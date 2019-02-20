#include "memoryManager.h"

#define SIZE 64 // max size of our memory
#define PAGE_SIZE 16 // pages are sized at 16 bytes
#define PAGE_FRAMES 4 // thus, we will have 4 page frames

// bits to help us keep track of info in our page table
#define VPN_BIT 1
#define PFN_BIT 2
#define VAL_BIT 3

unsigned char memory[SIZE]; // our memory array
int freeList[PAGE_FRAMES] = {0, 0, 0, 0}; // 0 if the page frame is free, 1 if occupied
int hardwareReg[4] = {-1, -1, -1, -1}; // keeps track of where the page table is in memory

int main(int argc, char *argv[]) {

    char input[16]; // store the user input
    char *delim = ","; // users input is separated by a comma

    while (1) {
        // prompt user for input
        printf("Instruction? ");
        fgets(input, sizeof(input), stdin); // read in input
        input[strcspn(input, "\n")] = '\0'; // remove the trailing '\n' character

        // parse user input
        int pid = atoi(strtok(input, delim));
        char *instruction = strtok(NULL, delim);
        int virtualAddress = atoi(strtok(NULL, delim));
        int value = atoi(strtok(NULL, delim));

        // verify input
        verifyInput(pid, instruction, virtualAddress, value);

        // run the instruction
        if (strcmp(instruction, "map") == 0) {
            map(pid, virtualAddress, value);
        } else if (strcmp(instruction, "store") == 0) {
            store(pid, virtualAddress, value);
        } else if (strcmp(instruction, "load") == 0) {
            load(pid, virtualAddress);
        }
    }
}

/**
 * tells the memory manager to allocate a physical page, i.e., it creates a mapping
 * in the page table between a virtual and physical address. The manager must determine
 * the appropriate virtual page number using the virtual address
 * @param pid
 * @param virtualAddress
 * @param value if 1, the page is writable and readable. if 0, the page is only readable
 * @return
 */
int map(int pid, int virtualAddress, int value) {
    int vpn = virtualAddress / PAGE_SIZE; // our virtual page number
    int frameAddr; // address of the frame we are interested in

    // todo handling when we try to enter a pid twice

    if (hardwareReg[pid] != -1) { // that page table exists already
        int pte = getFreePFN();// the frame of our page table entry

        if (pte == -1) {
            // todo handling the case where there is no free page table entry
            printf("error: that page table exists, but no more room\n");
            return -1;
        }

        frameAddr = hardwareReg[pid] * PAGE_SIZE;
        int pageTableOffset = vpn * PAGE_FRAMES;

        // store page table info
        memory[frameAddr + pageTableOffset] = pid;
        memory[frameAddr + pageTableOffset + VPN_BIT] = vpn;
        memory[frameAddr + pageTableOffset + PFN_BIT] = pte;
        memory[frameAddr + pageTableOffset + VAL_BIT] = value;

        printf("Mapped virtual address %d (page %d) into physical frame %d\n", virtualAddress, vpn, pte);

        return 0;
    } else { // creating a new page table
        int pte = getFreePFN();// the frame of our page table entry
        int pfn = getFreePFN(); // the frame we map our virtual address to

        if (pte == -1) {
            // todo handling the case where there is no free page table entry
            printf("error: trying to create a new page table, no room\n");
            return -1;
        }
        if (pfn == -1) {
            // todo handling the case where there is no free page table entry
            printf("error: trying to create a new page table, no room\n");
            return -1;
        }

        hardwareReg[pid] = pte; // keep track of where the pte for this pid is
        frameAddr = hardwareReg[pid] * PAGE_SIZE; // get our frame value

        // put the page table in memory
        memory[frameAddr] = pid;
        memory[frameAddr + VPN_BIT] = vpn;
        memory[frameAddr + PFN_BIT] = pfn;
        memory[frameAddr + VAL_BIT] = value;

        printf("Put page table for PID %d into physical frame %d\n", pid, pte);
        printf("Mapped virtual address %d (page %d) into physical frame %d\n", virtualAddress, vpn, pfn);

        return 0;
    }
}

/**
 * instructs the memory manager to write the supplied value into the physical memory location
 * associated with the provided virtual address, performing translation and page swapping
 * as necessary.
 * @param pid
 * @param virtualAddress
 * @param value
 * @return
 */
int store(int pid, int virtualAddress, int value) {
    int vpn = virtualAddress / PAGE_SIZE;
    int offset = virtualAddress - vpn * PAGE_SIZE;

    int needToSwap = 0;
    if(needToSwap == 1) { // no space, we must swap
        // todo handling the case where there is no free page table entry

    } else { // there is space, so store the value
        int frameAddr = hardwareReg[pid] * PAGE_SIZE;
        int pageTableOffset = vpn * PAGE_FRAMES;
        int isWritable = memory[frameAddr + pageTableOffset + VAL_BIT]; // use the val (rw in this case) bit to determine if it is writable

        if(isWritable) {
            int pfn = memory[frameAddr + pageTableOffset + PFN_BIT];
            int physicalAddress = pfn * PAGE_SIZE + offset;

            memory[physicalAddress] = value;
            printf("Stored value %d at virtual address %d (physical address %d)\n", value, virtualAddress, physicalAddress);
        } else {
            printf("Error, cannot write to this page as it is read only\n");
        }
    }

    return 0;
}

/**
 * instructs the memory manager to return the byte stored at the memory location specified
 * by virtual address. Like the store instruction, it is the memory manager’s responsibility
 * to translate and swap pages as needed.
 * @param pid
 * @param virtualAddress
 * @return
 */
int load(int pid, int virtualAddress) {
    int vpn = virtualAddress / PAGE_SIZE;
    int offset = virtualAddress - vpn * PAGE_SIZE;

    int needToSwap = 0;
    if(needToSwap == 1) { // no space, we must swap
        // todo handling the case where there is no free page table entry

    } else {
        int frameAddr = hardwareReg[pid] * PAGE_SIZE;
        int pageTableOffset = vpn * 4;

        int pfn = memory[frameAddr + pageTableOffset + PFN_BIT];
        int physicalAddress = pfn * PAGE_SIZE + offset;

        int value = memory[physicalAddress];
        printf("The value %d is virtual address %d  (physical address %d)\n", value, virtualAddress, physicalAddress);
    }

    return 0;
}

/**
 * tries to find a free page frame number, marks it as occupied, and returns the page frame number
 * @return the page frame number, or -1 if there is no free page frame
 */
int getFreePFN() {
    for (int i = 0; i < PAGE_FRAMES; i++) {
        if (freeList[i] == 0) { // found a free page frame
            freeList[i] = 1; // now occupied
            return i;
        }
    }
    return -1;
}

/**
 * verify the users input
 */
void verifyInput(int pid, char *instruction, int virtualAddress, int value) {
    if (pid < 0 || pid > 3) {
        printf("Please enter a process_id in the range [0, 3]\n");
        exit(1);
    }
    if (!(strcmp(instruction, "map") == 0 || strcmp(instruction, "store") == 0 || strcmp(instruction, "load") == 0)) {
        printf("Please enter either map, store, or load as an instruction\n");
        exit(1);
    }
    if (virtualAddress < 0 || virtualAddress > 63) {
        printf("Please enter a virtual_address in the range [0, 63]\n");
        exit(1);
    }
    if (value < 0 || value > 255) {
        printf("Please enter a value in the range [0, 255]\n");
        exit(1);
    }
}
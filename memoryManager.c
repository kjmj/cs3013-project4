#include "memoryManager.h"

#define SIZE 64 // max size of our memory
#define PAGE_SIZE 16 // pages are sized at 16 bytes
#define PAGE_FRAMES 4 // thus, we will have 4 page frames

// bits to help us keep track of info in our page table
#define VPN_BIT 1
#define PFN_BIT 2
#define VAL_BIT 3
#define UNMAPPED -1
#define MAPPED 1
#define ONDISK 4

unsigned char memory[SIZE]; // our memory array
int freeList[PAGE_FRAMES] = {0, 0, 0, 0}; // 0 if the page frame is free, 1 if occupied
int hardwareReg[4] = {UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED}; // keeps track of where the page table is in memory
int virPagMapInfo[16] = {UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED,
                         UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED, UNMAPPED,
                         UNMAPPED}; //Document whether a virtual page is mapped or not, four virtual pages for four processes

int main(int argc, char *argv[]) {

    char input[16]; // store the user input
    char *delim = ","; // users input is separated by a comma

    // open the disk file
    FILE *file = fopen("disk.txt", "w+");

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
            map(pid, virtualAddress, value, file);
        } else if (strcmp(instruction, "store") == 0) {
            store(pid, virtualAddress, value, file);
        } else if (strcmp(instruction, "load") == 0) {
            load(pid, virtualAddress, file);
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
int map(int pid, int virtualAddress, int value, FILE *disk) {
    int vpn = virtualAddress / PAGE_SIZE; // our virtual page number
    int frameAddr; // address of the frame we are interested in

    if (hardwareReg[pid] != -1) { // that page table exists already for this process
        frameAddr = hardwareReg[pid] * PAGE_SIZE;
        int pageTableOffset = vpn * PAGE_FRAMES;
        //check if that virtual page is mapped
        if (virPagMapInfo[pid * PAGE_FRAMES + vpn] == MAPPED) {
            // virtual page mapped already
            if (value == memory[frameAddr + pageTableOffset + VAL_BIT]) {
                printf("Error: virtual page %d is already mapped with rw_bit=%d\n", vpn, value);
                return 0;
            } else {
                memory[frameAddr + pageTableOffset + VAL_BIT] = value;
                printf("Updating permissions for virtual page %d (frame %d)\n", vpn,
                       memory[frameAddr + pageTableOffset + PFN_BIT]);
                return 0;
            }
        } else {
            // page table exists for that process but this virtual page unmapped
            printf("page table exists for that process and mapping this virtual page\n");
            int pfn = getFreePFN(); // new physical frame
            if (pfn == -1) {
                // no free page table entry and swap for new space
                printf("Swapping!!!!\n");
                // pick random a page to evict.
                int pageEvict = rand() % (PAGE_FRAMES); //random number from 0 to 3
                /*
                 * write the evicted page to disk
                */
                int pagTabEvictPID = -1; // process id 's page table ejected
                // check if the evicted page is a page table
                for (int i = 0; i < PAGE_FRAMES; i++) {
                    if (hardwareReg[i] == pageEvict) {
                        // page table evicted
                        pagTabEvictPID = i;
                    }
                }
                printf("check if evicted page %d is a page table\n", pageEvict);
                if (pagTabEvictPID == -1) {
                    printf("evicted page %d is not a page table\n", pageEvict);
                    //evicted not page table
                    int vpn = -1; //store the vpn for evicted physical frame
                    int pid = -1; //pid for the evicted page
                    int pageTableEvictFrame = -1;
                    // find which process this physical frame belong to
                    for (int i = 0; i < PAGE_FRAMES; i++) {
                        if (hardwareReg[i] != UNMAPPED && hardwareReg[i] != ONDISK) {
                            // page table for pid i is on board and check if that maps the evicted
                            for (int j = 0; j < PAGE_FRAMES; j++) {
                                int frameAddrEv = hardwareReg[i] * PAGE_SIZE;
                                if (memory[frameAddrEv + j * PAGE_FRAMES + PFN_BIT] == pageEvict) {
                                    // found the page table on memory
                                    pageTableEvictFrame = hardwareReg[i]; //where the page table is on the memory
                                    vpn = memory[frameAddrEv + j * PAGE_FRAMES + VPN_BIT];
                                    pid = memory[frameAddrEv + j * PAGE_FRAMES];
                                    memory[frameAddrEv + j * PAGE_FRAMES +
                                           PFN_BIT] = ONDISK; //update the page table indicate swapped frame is in disk now
                                    virPagMapInfo[pid * PAGE_FRAMES + vpn] = ONDISK; // record that page table is on the
                                    printf("found page table on board vpn %d, pid %d\n", vpn, pid);
                                    break;
                                }
                            }
                        }
                    }
                    if (vpn != -1 && pid != -1) {
                        //found the page table on memory
                        unsigned char memoryBuff[PAGE_SIZE];
                        //save the memory frame to a temporary buffer
                        for (int i = 0; i < PAGE_SIZE; i++) {
                            memoryBuff[i] = memory[pageEvict * PAGE_SIZE + i];
                        }
                        //write to disk at appropriate location
                        fseek(disk, (pid * PAGE_FRAMES + vpn) * PAGE_SIZE, SEEK_SET);
                        fwrite(memoryBuff, 1, sizeof(memoryBuff), disk);
                        // empty the memory physical frame evicted
                        for (int i = 0; i < PAGE_SIZE; i++) {
                            memory[pageEvict * PAGE_SIZE + i] = 0;
                        }
                        printf("Swapped frame %d to disk at swap slot %d\n", pageEvict, swapSlot);
                        swapSlot++;
                        // now there is free physical frame
                        // map it in the page table updated
                        memory[frameAddr + pageTableOffset] = pid;
                        memory[frameAddr + pageTableOffset + VPN_BIT] = vpn;
                        memory[frameAddr + pageTableOffset + PFN_BIT] = pageEvict;
                        memory[frameAddr + pageTableOffset + VAL_BIT] = value;
                        //record the virtual map address is mapped now
                        virPagMapInfo[pid * PAGE_FRAMES + vpn] = MAPPED;
                        printf("Mapped virtual address %d (page %d) into physical frame %d\n", virtualAddress, vpn,
                               pageEvict);
                        return 0;
                    }
                } else {
                    // evicted page is page table for process pagTabEvictPID
                    printf("evicted page %d is page table for process %d", pageEvict, pagTabEvictPID);
                    return -1;
                }
            } else {
                memory[frameAddr + pageTableOffset] = pid;
                memory[frameAddr + pageTableOffset + VPN_BIT] = vpn;
                memory[frameAddr + pageTableOffset + PFN_BIT] = pfn;
                memory[frameAddr + pageTableOffset + VAL_BIT] = value;
                //record the virtual map address is mapped already
                virPagMapInfo[pid * PAGE_FRAMES + vpn] = MAPPED;
                printf("Mapped virtual address %d (page %d) into physical frame %d\n", virtualAddress, vpn, pfn);
                return 0;
            }
        }
    } else { // creating a new page table
        int pte = getFreePFN();// the frame of our page table entry
        int pfn = getFreePFN(); // the frame we map our virtual address to

        if (pte == -1) {
            // no space for page table
            printf("Swapping!!!!\n");
            // pick random a page to evict.
            int pageEvict = rand() % (PAGE_FRAMES); //random number from 0 to 3
            /*
             * write the evicted page to disk
            */
            int pagTabEvictPID = -1; // process id 's page table ejected
            // check if the evicted page is a page table
            for (int i = 0; i < PAGE_FRAMES; i++) {
                if (hardwareReg[i] == pageEvict) {
                    // page table evicted
                    pagTabEvictPID = i;
                }
            }
            printf("check if evicted page %d is a page table\n", pageEvict);
            if (pagTabEvictPID == -1) {
                printf("evicted page %d is not a page table\n", pageEvict);
                //evicted not page table
                int vpn = -1; //store the vpn for evicted physical frame
                int pid = -1; //pid for the evicted page
                int pageTableEvictFrame = -1;
                // find which process this physical frame belong to
                for (int i = 0; i < PAGE_FRAMES; i++) {
                    if (hardwareReg[i] != UNMAPPED && hardwareReg[i] != ONDISK) {
                        // page table for pid i is on board and check if that maps the evicted
                        for (int j = 0; j < PAGE_FRAMES; j++) {
                            int frameAddrEv = hardwareReg[i] * PAGE_SIZE;
                            if (memory[frameAddrEv + j * PAGE_FRAMES + PFN_BIT] == pageEvict) {
                                // found the page table on memory
                                pageTableEvictFrame = hardwareReg[i]; //where the page table is on the memory
                                vpn = memory[frameAddrEv + j * PAGE_FRAMES + VPN_BIT];
                                pid = memory[frameAddrEv + j * PAGE_FRAMES];
                                memory[frameAddrEv + j * PAGE_FRAMES +
                                       PFN_BIT] = ONDISK; //update the page table indicate swapped frame is in disk now
                                virPagMapInfo[pid * PAGE_FRAMES + vpn] = ONDISK; // record that page table is on the
                                printf("found page table on board vpn %d, pid %d\n", vpn, pid);
                                break;
                            }
                        }
                    }
                }
                if (vpn != -1 && pid != -1) {
                    //found the page table on memory
                    unsigned char memoryBuff[PAGE_SIZE];
                    //save the memory frame to a temporary buffer
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        memoryBuff[i] = memory[pageEvict * PAGE_SIZE + i];
                    }
                    //write to disk at appropriate location
                    fseek(disk, (pid * PAGE_FRAMES + vpn) * PAGE_SIZE, SEEK_SET);
                    fwrite(memoryBuff, 1, sizeof(memoryBuff), disk);
                    // empty the memory physical frame evicted
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        memory[pageEvict * PAGE_SIZE + i] = 0;
                    }
                    printf("Swapped frame %d to disk at swap slot %d\n", pageEvict, swapSlot);
                    swapSlot++;
                    // now there is free physical frame
                    // map it in the page table updated
                    pte = pageEvict;
                }
            } else {
                // evicted page is page table for process pagTabEvictPID
                printf("evicted page %d is page table for process %d", pageEvict, pagTabEvictPID);
                return -1;
            }
        }
        if (pfn == -1) {
            printf("Swapping!!!!\n");
            // pick random a page to evict.
            int pageEvict = rand() % (PAGE_FRAMES); //random number from 0 to 3
            /*
             * write the evicted page to disk
            */
            int pagTabEvictPID = -1; // process id 's page table ejected
            // check if the evicted page is a page table
            for (int i = 0; i < PAGE_FRAMES; i++) {
                if (hardwareReg[i] == pageEvict) {
                    // page table evicted
                    pagTabEvictPID = i;
                }
            }
            printf("check if evicted page %d is a page table\n", pageEvict);
            if (pagTabEvictPID == -1) {
                printf("evicted page %d is not a page table\n", pageEvict);
                //evicted not page table
                int vpn = -1; //store the vpn for evicted physical frame
                int pid = -1; //pid for the evicted page
                int pageTableEvictFrame = -1;
                // find which process this physical frame belong to
                for (int i = 0; i < PAGE_FRAMES; i++) {
                    if (hardwareReg[i] != UNMAPPED && hardwareReg[i] != ONDISK) {
                        // page table for pid i is on board and check if that maps the evicted
                        for (int j = 0; j < PAGE_FRAMES; j++) {
                            int frameAddrEv = hardwareReg[i] * PAGE_SIZE;
                            if (memory[frameAddrEv + j * PAGE_FRAMES + PFN_BIT] == pageEvict) {
                                // found the page table on memory
                                pageTableEvictFrame = hardwareReg[i]; //where the page table is on the memory
                                vpn = memory[frameAddrEv + j * PAGE_FRAMES + VPN_BIT];
                                pid = memory[frameAddrEv + j * PAGE_FRAMES];
                                memory[frameAddrEv + j * PAGE_FRAMES +
                                       PFN_BIT] = ONDISK; //update the page table indicate swapped frame is in disk now
                                virPagMapInfo[pid * PAGE_FRAMES + vpn] = ONDISK; // record that page table is on the
                                printf("found page table on board vpn %d, pid %d\n", vpn, pid);
                                break;
                            }
                        }
                    }
                }
                if (vpn != -1 && pid != -1) {
                    //found the page table on memory
                    unsigned char memoryBuff[PAGE_SIZE];
                    //save the memory frame to a temporary buffer
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        memoryBuff[i] = memory[pageEvict * PAGE_SIZE + i];
                    }
                    //write to disk at appropriate location
                    fseek(disk, (pid * PAGE_FRAMES + vpn) * PAGE_SIZE, SEEK_SET);
                    fwrite(memoryBuff, 1, sizeof(memoryBuff), disk);
                    // empty the memory physical frame evicted
                    for (int i = 0; i < PAGE_SIZE; i++) {
                        memory[pageEvict * PAGE_SIZE + i] = 0;
                    }
                    printf("Swapped frame %d to disk at swap slot %d\n", pageEvict, swapSlot);
                    swapSlot++;
                    // now there is free physical frame
                    // map it in the page table updated
                    pfn = pageEvict;
                }
            } else {
                // evicted page is page table for process pagTabEvictPID
                printf("evicted page %d is page table for process %d", pageEvict, pagTabEvictPID);
                return -1;
            }
        }
        hardwareReg[pid] = pte; // keep track of where the pte for this pid is
        frameAddr = hardwareReg[pid] * PAGE_SIZE; // get our frame address at the start of page frame
        int pageTableOffset = vpn * PAGE_FRAMES;

        // map the virtual page to physical frame
        memory[frameAddr + pageTableOffset] = pid; //process id
        memory[frameAddr + pageTableOffset + VPN_BIT] = vpn; //virtual page number
        memory[frameAddr + pageTableOffset + PFN_BIT] = pfn; //physical frame number
        memory[frameAddr + pageTableOffset + VAL_BIT] = value; //valid bit

        // record that it is mapped table
        virPagMapInfo[pid * PAGE_FRAMES + vpn] = MAPPED;
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
int store(int pid, int virtualAddress, int value, FILE *disk) {
    int vpn = virtualAddress / PAGE_SIZE;  // Extract the VPN from the virtual address
    if (virPagMapInfo[pid * PAGE_FRAMES + vpn] == MAPPED) {
        int offset = virtualAddress - vpn * PAGE_SIZE;
        int frameAddr = hardwareReg[pid] * PAGE_SIZE;
        int pageTableOffset = vpn * PAGE_FRAMES;
        int isWritable = memory[frameAddr + pageTableOffset +
                                VAL_BIT]; // use the val (rw in this case) bit to determine if it is writable

        if (isWritable) {
            int pfn = memory[frameAddr + pageTableOffset + PFN_BIT];
            int physicalAddress = pfn * PAGE_SIZE + offset;

            memory[physicalAddress] = value;
            printf("Stored value %d at virtual address %d (physical address %d)\n", value, virtualAddress,
                   physicalAddress);
        } else {
            printf("Error, cannot write to this page as it is read only\n");
        }
    }
    else if (virPagMapInfo[pid * PAGE_FRAMES + vpn] == ONDISK){
        printf("Swapping!!!!\n");
        // pick random a page to evict.
        int pageEvict = rand() % (PAGE_FRAMES); //random number from 0 to 3
        /*
         * write the evicted page to disk
        */
        int pagTabEvictPID = -1; // process id 's page table ejected
        // check if the evicted page is a page table
        for (int i = 0; i < PAGE_FRAMES; i++) {
            if (hardwareReg[i] == pageEvict) {
                // page table evicted
                pagTabEvictPID = i;
            }
        }
        printf("check if evicted page %d is a page table\n", pageEvict);
        if (pagTabEvictPID == -1) {
            printf("evicted page %d is not a page table\n", pageEvict);
            //evicted not page table
            int vpn = -1; //store the vpn for evicted physical frame
            int pid = -1; //pid for the evicted page
            int pageTableEvictFrame = -1;
            // find which process this physical frame belong to
            for (int i = 0; i < PAGE_FRAMES; i++) {
                if (hardwareReg[i] != UNMAPPED && hardwareReg[i] != ONDISK) {
                    // page table for pid i is on board and check if that maps the evicted
                    for (int j = 0; j < PAGE_FRAMES; j++) {
                        int frameAddrEv = hardwareReg[i] * PAGE_SIZE;
                        if (memory[frameAddrEv + j * PAGE_FRAMES + PFN_BIT] == pageEvict) {
                            // found the page table on memory
                            pageTableEvictFrame = hardwareReg[i]; //where the page table is on the memory
                            vpn = memory[frameAddrEv + j * PAGE_FRAMES + VPN_BIT];
                            pid = memory[frameAddrEv + j * PAGE_FRAMES];
                            memory[frameAddrEv + j * PAGE_FRAMES +
                                   PFN_BIT] = ONDISK; //update the page table indicate swapped frame is in disk now
                            virPagMapInfo[pid * PAGE_FRAMES + vpn] = ONDISK; // record that page table is on the
                            printf("found page table on board vpn %d, pid %d\n", vpn, pid);
                            break;
                        }
                    }
                }
            }
            if (vpn != -1 && pid != -1) {
                //found the page table on memory
                unsigned char memoryBuff[PAGE_SIZE];
                //save the memory frame to a temporary buffer
                for (int i = 0; i < PAGE_SIZE; i++) {
                    memoryBuff[i] = memory[pageEvict * PAGE_SIZE + i];
                }
                //write to disk at appropriate location
                fseek(disk, (pid * PAGE_FRAMES + vpn) * PAGE_SIZE, SEEK_SET);
                fwrite(memoryBuff, 1, sizeof(memoryBuff), disk);
                // empty the memory physical frame evicted
                for (int i = 0; i < PAGE_SIZE; i++) {
                    memory[pageEvict * PAGE_SIZE + i] = 0;
                }
                printf("Swapped frame %d to disk at swap slot %d\n", pageEvict, swapSlot);
                swapSlot++;
                // now there is free physical frame

            }
        } else {
            // evicted page is page table for process pagTabEvictPID
            printf("evicted page %d is page table for process %d", pageEvict, pagTabEvictPID);
            return -1;
        }
        // swap data
        // read from disk
        unsigned char memoryBuffRead[PAGE_SIZE];
        fseek(disk, (pid * PAGE_FRAMES + vpn) * PAGE_SIZE, SEEK_SET);
        fread(memoryBuffRead, sizeof(memoryBuffRead), 1, disk);
        // load on to memory
        for(int i = 0; i < 16; i++){
            memory[pageEvict * PAGE_SIZE + i] = memoryBuffRead[i];
        }
        //modify the memory
        int offset = virtualAddress - vpn * PAGE_SIZE;
        int frameAddr = hardwareReg[pid] * PAGE_SIZE;
        int pageTableOffset = vpn * PAGE_FRAMES;
        int isWritable = memory[frameAddr + pageTableOffset +
                                VAL_BIT]; // use the val (rw in this case) bit to determine if it is writable

        if (isWritable) {
            int pfn = memory[frameAddr + pageTableOffset + PFN_BIT];
            int physicalAddress = pfn * PAGE_SIZE + offset;

            memory[physicalAddress] = value;
            printf("Stored value %d at virtual address %d (physical address %d)\n", value, virtualAddress,
                   physicalAddress);
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
int load(int pid, int virtualAddress, FILE *disk) {
    int vpn = virtualAddress / PAGE_SIZE;
    int offset = virtualAddress - vpn * PAGE_SIZE;

    int needToSwap = 0;
    if (needToSwap == 1) { // no space, we must swap
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

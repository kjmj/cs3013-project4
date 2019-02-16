#include "memoryManager.h"

int main(int argc, char *argv[]) {

    char input[16]; // store the user input
    char *delim = ","; // users input is separated by a comma

    while(1) {

        // prompt user for input
        printf("Instruction? ");
        fgets(input, sizeof(input), stdin); // read in input
        input[strcspn(input, "\n")] = '\0'; // remove the trailing '\n' character

        // parse user input
        int pid = atoi(strtok(input, delim));
        char *instruction = strtok(NULL, delim);
        int virtualAddress = atoi(strtok(NULL, delim));
        int meaning = atoi(strtok(NULL, delim));

        // verify input
        verifyInput(pid, instruction, virtualAddress, meaning);

        // run the instruction
        if(strcmp(instruction, "map")) {
            map(pid, virtualAddress, meaning);
        } else if(strcmp(instruction, "store")) {
            store(pid, virtualAddress, meaning);
        } else if(strcmp(instruction, "load")) {
            load(pid, virtualAddress);
        }
    }
}

int map(int pid, int virtualAddress, int meaning) {

    return 0;
}


int store(int pid, int virtualAddress, int meaning) {

    return 0;
}


int load(int pid, int virtualAddress) {

    return 0;
}
/**
 * verify the users input
 */
void verifyInput(int pid, char *instruction, int virtualAddress, int meaning) {
    if(pid < 0 || pid > 3) {
        printf("Please enter a process_id in the range [0, 3]\n");
        exit(1);
    }
    if(!(strcmp(instruction, "map") == 0 || strcmp(instruction, "store") == 0 || strcmp(instruction, "load") == 0)) {
        printf("Please enter either map, store, or load as an instruction\n");
        exit(1);
    }
    if(virtualAddress < 0 || virtualAddress > 63) {
        printf("Please enter a virtual_address in the range [0, 63]\n");
        exit(1);
    }
    if(meaning < 0 || meaning > 255) {
        printf("Please enter a meaning in the range [0, 255]\n");
        exit(1);
    }
}
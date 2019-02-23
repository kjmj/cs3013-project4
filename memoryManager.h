#ifndef memoryManager
#define memoryManager

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int swapSlot = 0;

int map(int pid, int virtualAddress, int value, FILE *disk);

int store(int pid, int virtualAddress, int value, FILE *disk);

int load(int pid, int virtualAddress, FILE *disk);

void verifyInput(int pid, char *instruction, int virtualAddress, int value);

int getFreePFN();

#endif
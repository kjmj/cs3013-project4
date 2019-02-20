#ifndef memoryManager
#define memoryManager

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int map(int pid, int virtualAddress, int value);
int store(int pid, int virtualAddress, int value);
int load(int pid, int virtualAddress);
void verifyInput(int pid, char *instruction, int virtualAddress, int value);
int getFreePFN();

#endif
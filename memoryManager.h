#ifndef memoryManager
#define memoryManager

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int *handleInput();
int map(int pid, int virtualAddress, int meaning);
int store(int pid, int virtualAddress, int meaning);
int load(int pid, int virtualAddress);
void verifyInput(int pid, char *instruction, int virtualAddress, int meaning);

#endif
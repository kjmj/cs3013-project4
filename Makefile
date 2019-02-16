all: memoryManager.c
	gcc -g -Wall -o memoryManager memoryManager.c

clean: 
	$(RM) memoryManager

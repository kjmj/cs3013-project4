----- Overview -----
This project simulates virtual memory using paging and swapping.


----- Usage -----
To build and run:
    make all
    ./memoryManager

Input:
    Instructions must be supplied to the memory manager in the format described below

    (process_id,instruction_type,virtual_address,value)

    - process_id: an integer in the range [0, 3] used to simulate instructions from different processes
    - instruction_type: the desired memory operation — see the instruction list below
    - virtual_address: an integer in the range [0, 63] specifying the virtual memory location for given process.
    - value: depends on the instruction, but must be an integer value in the range [0, 255]

Instructions:
    The memory manager supports the following instructions:
    - map: tells the memory manager to allocate a physical page by creating a mapping in the page table
           between a virtual and physical address. The manager must determine the appropriate virtual
           page number using the virtual address. For example, virtual address of 3 corresponds to
           virtual page 0. value argument represents the write permission for the page. If value=1,
           then the page is writeable and readable. If value=0, then the page is only readable, i.e., all
           mapped pages are readable. These permissions can be modified by using a second map instruction
           for the target page.
    - store: instructs the memory manager to write the supplied value into the physical memory location associated
             with the provided virtual address, performing translation and page swapping as necessary
    - load: instructs the memory manager to return the byte stored at the memory location specified by
            virtual address. Like the store instruction, it is the memory manager’s responsibility to
            translate and swap pages as needed. Note, the value parameter is not used for this instruction,
            but a dummy value (e.g., 0) should always be provided.

To clean:
    make clean


----- Testing -----
Here is a basic test:
    kenneth@kenneth-VirtualBox:~/Documents/cs3103/cs3013-project4$ ./memoryManager
    Instruction? 0,map,0,1
    Put page table for PID 0 into physical frame 0
    Mapped virtual address 0 (page 0) into physical frame 1
    Instruction? 0,store,12,24
    Stored value 24 at virtual address 12 (physical address 28)
    Instruction? 0,load,12,0
    The value 24 is virtual address 12  (physical address 28)
    Instruction? ^C

Here is a test with multiple processes:
    kenneth@kenneth-VirtualBox:~/Documents/cs3103/cs3013-project4$ ./memoryManager
    Instruction? 0,map,0,1
    Put page table for PID 0 into physical frame 0
    Mapped virtual address 0 (page 0) into physical frame 1
    Instruction? 0,store,12,24
    Stored value 24 at virtual address 12 (physical address 28)
    Instruction? 1,map,16,1
    Put page table for PID 1 into physical frame 2
    Mapped virtual address 16 (page 1) into physical frame 3
    Instruction? 1,store,5,6
    Stored value 6 at virtual address 5 (physical address 53)
    Instruction? 0,load,12,0
    The value 24 is virtual address 12  (physical address 28)
    Instruction? 1,load,5,0
    The value 6 is virtual address 5  (physical address 53)
    Instruction? ^C

Here is a test with mapping and swapping to disk
    Instruction? 0,map,0,1
    Put page table for PID 0 into physical frame 0
    Mapped virtual address 0 (page 0) into physical frame 1
    Instruction? 0,map,16,1
    page table exists for that process and mapping this virtual page
    Mapped virtual address 16 (page 1) into physical frame 2
    Instruction? 0,map,32,1
    page table exists for that process and mapping this virtual page
    Mapped virtual address 32 (page 2) into physical frame 3
    Instruction? 0,map,48,1
    page table exists for that process and mapping this virtual page
    Swapping!!!!
    check if evicted page 3 is a page table
    evicted page 3 is not a page table
    found page table on board vpn 2, pid 0
    Swapped frame 3 to disk at swap slot 0
    Mapped virtual address 48 (page 2) into physical frame 3

Here is a test with mapping and swapping to disk
    Instruction? 0,map,0,1
    Put page table for PID 0 into physical frame 0
    Mapped virtual address 0 (page 0) into physical frame 1
    Instruction? 0,map,16,1
    page table exists for that process and mapping this virtual page
    Mapped virtual address 16 (page 1) into physical frame 2
    Instruction? 0,map,32,1
    page table exists for that process and mapping this virtual page
    Mapped virtual address 32 (page 2) into physical frame 3
    Instruction? 1,map,0,1
    Swapping!!!!
    check if evicted page 3 is a page table
    evicted page 3 is not a page table
    found page table on board vpn 2, pid 0
    Swapped frame 3 to disk at swap slot 0
    Swapping!!!!
    check if evicted page 1 is a page table
    evicted page 1 is not a page table
    found page table on board vpn 0, pid 0
    Swapped frame 1 to disk at swap slot 1
    Put page table for PID 1 into physical frame 3
    Mapped virtual address 0 (page 0) into physical frame 1

Here is a test with illegal instructions
    Instruction? 0,map,0,0
    Put page table for PID 0 into physical frame 0
    Mapped virtual address 0 (page 0) into physical frame 1
    Instruction? 0,store,7,255
    Error, cannot write to this page as it is read only


Here is a test with updating writing permission
    Instruction? 0,map,0,0
    Put page table for PID 0 into physical frame 0
    Mapped virtual address 0 (page 0) into physical frame 1
    Instruction? 0,store,7,255
    Error, cannot write to this page as it is read only
    Instruction? 0,map,0,1
    Updating permissions for virtual page 0 (frame 1)

TODO: testing for swapping to disk and some edge cases/illegal instructions

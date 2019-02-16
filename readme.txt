This project is in process and will change drastically over the next few days.

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

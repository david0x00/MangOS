#include "systemcalls.h"
#include "file_sys.h"
#include "pcb.h"
#include "rtc.h"
#include "terminal.h"
#include "x86_desc.h"
#include "sched.h"

uint32_t vidmap_term0[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
uint32_t vidmap_term1[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
uint32_t vidmap_term2[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
uint32_t* vidmap_page_table_array[NUM_TERMS] = {vidmap_term0, vidmap_term1, vidmap_term2};

/*
 * void switch_to_user_mode(uint32_t esp_new, uint32_t eip_new)
 *   DESCRIPTION: prepares for and switched from kernel mode to user mode
 *   INPUTS: esp_new - the new stack pointer
 *  		-- eip_new - the new instruction pointer
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: switches to user mode
 */
void switch_to_user_mode(uint32_t esp_new, uint32_t eip_new) {
   // Set up a stack structure for switching to user mode.
   asm volatile(
		"cli;"
		//set up ds register
		"movw $0x2B, %%ax;"
		"movw %%ax, %%ds;"
		"movw %%ax, %%es;"
		"movw %%ax, %%fs;"
		"movw %%ax, %%gs;"
		// "movw %%ax, %%ss;"

		//push DS
		"pushl $0x2B;"
		//push ESP
		"pushl %0;"

		//need to set the IF flag because we cli'd
		"pushfl;"
		"popl %%eax;"
		"orl $0x200, %%eax;"
		//push EFLAGS
		"pushl %%eax;"

		//push CS
		"pushl $0x23;"

		//push EIP
		"pushl %1;"
		"iret;"
		:
		: "m" (esp_new), "m" (eip_new)
		: "eax"
	);
}

/*
 * int32_t validate_file(const uint8_t* filename, uint32_t* file_length, uint32_t *inode)
 *   DESCRIPTION: Checks if the file exists as one of the directory entries
 *   INPUTS: filename - the file name
 *				-- file_length - the length of the file we find
 *				-- inode - the inode of the file we want to find
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful, ERROR if failed
 *   SIDE EFFECTS: none
 */
int32_t validate_file(const uint8_t* filename, uint32_t* file_length, int32_t *inode) {
	uint8_t buf[EXEC_MAGIC_NO];
	uint8_t exec_magic_no[EXEC_MAGIC_NO] = {EXEC_0, EXEC_1, EXEC_2, EXEC_3};
	uint32_t i;

	// Gets the inode number
	*inode = get_inode_from_name(filename);

	if(*inode == ERROR)
		return ERROR;

	// Check if file exists
	// We are only reading the first 4 bytes because they contain metadata on the d_entry
	if(read_data(*inode, 0, buf, 4) == ERROR)
		return ERROR;

	// Check if file is an executable
	// We just check the first 4 bytes that we put into our buffer
	for(i = 0; i < 3; ++i) {
		if(buf[i] != exec_magic_no[i])
			return ERROR;
	}

	*file_length = boot_block.inodes[*inode].length;

	return 0;
}

/*
 * int32_t get_file_name(const uint8_t* command, uint8_t* filename, uint32_t* filename_end)
 *   DESCRIPTION: Gets the filename based on the command being executed
 *   INPUTS: command - the command we are executing
 * 				-- filename - the file name
 *				-- filename_end - the index of the filename_end
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful, ERROR if failed
 *   SIDE EFFECTS: none
 */
int32_t get_file_name(const uint8_t* command, uint8_t* filename, uint32_t* filename_end) {
	uint32_t i, start_index = 0;

	while(command[start_index] == ' ') {
		if(command[start_index] == '\0')
			return ERROR;
		start_index++;
	}

	//read the filename
	for(i = 0; i <= MAX_STRING_LEN; i++) {
		if (command[start_index + i] == ' ' || command[start_index + i] == '\0') {
			*filename_end = start_index + i;
			filename[i] = '\0';
			break;
		} else
			filename[i] = command[start_index + i];
	}

	return 0;
}

/*
 * int32_t get_entry_point(uint32_t inode)
 *   DESCRIPTION: Gets the virtual address from the file
 *   INPUTS: inode - the inode
 *   OUTPUTS: none
 *   RETURN VALUE: returns the entry point virtual address
 *   SIDE EFFECTS: none
 */
int32_t get_entry_point(uint32_t inode) {
	uint32_t entry_point = 0;
	uint8_t entry_point_buf[4];

	// We only want to covery bytes 24-47 (inclusive) because they contain the virtual address
	if(read_data(inode, 24, entry_point_buf, 4) == ERROR)
		return ERROR;

	// We shift the bits down (respectively) to construct the virtual address
	entry_point = (entry_point_buf[3] << 24) | (entry_point_buf[2] << 16) |
				  (entry_point_buf[1] << 8)  |  entry_point_buf[0];
	return entry_point;
}

/*
 * void flush_tlb()
 *   DESCRIPTION: Flushes the tlb
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void flush_tlb() {
	asm volatile(
        // load page directory into cr3
        "movl %%cr3, %%eax;"
        "movl %%eax, %%cr3;"
        :
        :
        : "eax"
    );
}


/*
 * int32_t add_args_to_buf(uint8_t* buf, const uint8_t* command, const uint32_t filename_end)
 *   DESCRIPTION: Adds the arguments to a buffer
 *   INPUTS: buf - The buffer we will fill
 *           command - The shell command
 *           filename_end - The index of where the argument begins
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Fills in the buffer
 */
int32_t add_args_to_buf(uint8_t* buf, const uint8_t* command, const uint32_t filename_end) {
	uint32_t i, j;
	i = filename_end;
	j = 0;

    if(buf == NULL)
        return ERROR;

	if(filename_end == ERROR)
		return ERROR;

	// removes all leading whitespace
	while(command[i] == ' ')
		i++;

	while(command[i] != '\0')
		buf[j++] = command[i++];

	buf[j] = '\0';

	return 0;

}


/*
 * execute(const uint8_t* command)
 *   DESCRIPTION: Attempt to load and execute new program
 *   INPUTS: command to execute
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful, ERROR if failed
 *   SIDE EFFECTS: Sets up kernel stack and creates a new PCB
 */
int32_t execute(const uint8_t* command) {
	uint8_t filename[BUF_SIZE];
	uint8_t args_buf[BUF_SIZE];
	int32_t inode;
	uint32_t filename_end, file_length, pid, stack_pointer, entry_point, esp, ebp, ret = 0, i = 0;
	pcb_t* pcb_addr;

	// fail if invalid command
	if (command == NULL)
        return ERROR;

	// fail if cannot parse file name
	if(get_file_name(command, filename, &filename_end) != 0)
		return ERROR;

	if(add_args_to_buf(args_buf, command, filename_end) != 0)
		return ERROR;

	if(validate_file(filename, &file_length, &inode) == ERROR)
		return ERROR;

	if((pid = get_available_pid()) == ERROR)
	 	return ERROR;

	page_directory[EXEC_PG_DIR_OFFSET] = (ALIGNED_8MB + ALIGNED_4MB * pid) | EXEC_PG_DIR_FLAGS;
	flush_tlb();

	if(read_data(inode, 0, (void*)LOAD_ADDR, file_length) == ERROR)
		return ERROR;

	if((entry_point = get_entry_point(inode)) == ERROR)
		return ERROR;

	pcb_addr = (pcb_t*)(ALIGNED_8MB - ((pid+1) * ALIGNED_8KB));

	if(init_pcb(pcb_addr) == ERROR)
		return ERROR;
	
	pcb_addr->pid = pid;
	if(set_pid(pid) == ERROR)
		return ERROR;

	for(i = 0; i < strlen((const int8_t*)args_buf); ++i)
		pcb_addr->args[i] = args_buf[i];

	stack_pointer = ALIGNED_132MB - ALIGNED_4B; //132MB - 1 address

	tss.ss0 = KERNEL_DS;
	if (num_processes > 0)
		tss.esp0 = ALIGNED_8MB - (pid * ALIGNED_8KB) - ALIGNED_4B;


	asm volatile (
		"movl %%esp, %0;"
		"movl %%ebp, %1;"
		: "=m" (esp), "=m" (ebp)
		:
	);

	pcb_addr->esp = esp;
	pcb_addr->ebp = ebp;

	asm volatile(
		"cli;"
		//set up ds register
		"movw $0x2B, %%ax;"
		"movw %%ax, %%ds;"
		"movw %%ax, %%es;"
		"movw %%ax, %%fs;"
		"movw %%ax, %%gs;"
		// "movw %%ax, %%ss;"

		//push DS
		"pushl $0x2B;"
		//push ESP
		"pushl %0;"

		//need to set the IF flag because we cli'd
		"pushfl;"
		"popl %%eax;"
		"orl $0x200, %%eax;"
		//push EFLAGS
		"pushl %%eax;"

		//push CS
		"pushl $0x23;"

		//push EIP
		"pushl %1;"
		"iret;"
		:
		: "m" (stack_pointer), "m" (entry_point)
	);

	asm volatile (
		// load page directory into cr3
        "halt_called:;"
        "movl %%edi, %0;"
		: "=r" (ret)
        :
        : "edi"
	);


	//if program dies by exception, return 256

	//if program executes halt system call, return value 0-255
	//this will be the value returned by the halt sys call.

	return ret;
}


/*
 * int32_t halt(uint8_t status)
 *   DESCRIPTION: Terminates the process
 *   INPUTS: status
 *   OUTPUTS: none
 *   RETURN VALUE: status or ERROR on failure
 *   SIDE EFFECTS: Makes current PCB the parent PCB of the current process and moves kernel stack pointer
 */
int32_t halt(uint8_t status) {
	//drop a label in inline assembly in execute function.
	//jump to that label in this function.
	//save the status in a register. and use that register.
	//this function is trippy
	int32_t ret;
	uint32_t esp, ebp, i;
	ret = (int32_t)status;

	pcb_t* finished_pcb = pcb_term[scheduler.curr_process->tid];

	if (finished_pcb == NULL)
		ret = ERROR;

	esp = finished_pcb->esp;
	ebp = finished_pcb->ebp;

    // close all fds in curr pcb
    for(i = 0; i < FD_ARRAY_MAX; i++){
        close(i);
    }

	pcb_term[scheduler.curr_process->tid] = finished_pcb->parent;

	free_pid(finished_pcb->pid);
	remove_process_from_runqueue(&scheduler, finished_pcb);

    finished_pcb = finished_pcb->parent;
	--num_processes;

	if (finished_pcb == NULL)
        execute((const uint8_t*)"shell");
	else {
		page_directory[EXEC_PG_DIR_OFFSET] = (ALIGNED_8MB + ALIGNED_4MB * finished_pcb->pid) | EXEC_PG_DIR_FLAGS;
		flush_tlb();
		tss.ss0 = KERNEL_DS;
		tss.esp0 = ALIGNED_8MB - (finished_pcb->pid * ALIGNED_8KB) - ALIGNED_4B;
	}

	asm volatile (
		"movl %0, %%edi;"
		"movl %1, %%esp;"
		"movl %2, %%ebp;"
		"jmp halt_called;"
		:
		: "m" (ret), "m" (esp), "m" (ebp)
	);

	return 0;
}

/*
 * int32_t read(int32_t fd, void* buf, int32_t nbytes)
 *   DESCRIPTION: Read data from keyboard, file, RTC, or directory
 *   INPUTS: fd - file descriptor to operate on, buf - store read value here,
 * nbytes - how many bytes we should read
 *   OUTPUTS: buf - values read go here
 *   RETURN VALUE: how many bytes we actually read
 *   SIDE EFFECTS: Updates file position field in PCB
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
	// fd is out-of-bounds
	if(fd < 0 || fd >= FD_ARRAY_MAX)
		return ERROR;

	// buf should be a valid pointer
	if (buf == NULL)
		return ERROR;

	// check if this fd has not been opened yet
	if((scheduler.curr_process->file_desc_array[fd].flags & IN_USE) == UNUSED)
		return ERROR;

	return scheduler.curr_process->file_desc_array[fd].file_op_table_ptr->read(fd, buf, nbytes);
}

/*
 * int32_t write(int32_t fd, const void* buf, int32_t nbytes)
 *   DESCRIPTION: Write data to terminal or RTC
 *   INPUTS: fd - file descriptor to operate on, buf - write from here,
 * nbytes - how many bytes we should write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes actually written
 *   SIDE EFFECTS: none
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
	// fd is out-of-bounds
	if(fd < 0 || fd >= FD_ARRAY_MAX)
		return ERROR;

	// buf should be a valid pointer
	if(buf == NULL)
		return ERROR;

	file_desc_t file_desc = scheduler.curr_process->file_desc_array[fd];

	// check if this fd has not been opened yet
	if((file_desc.flags & IN_USE) == UNUSED)
		return ERROR;

	return file_desc.file_op_table_ptr->write(fd, buf, nbytes);
}

/*
 * int32_t open(const uint8_t* filename)
 *   DESCRIPTION: allocate new file descriptor and put it in the PCB
 *   INPUTS: filename - name of the file to input
 *   OUTPUTS: none
 *   RETURN VALUE: ERROR on error : 0 on success
 *   SIDE EFFECTS: modifies the current pcb
 */
int32_t open(const uint8_t* filename) {
	// fill out fields of the file_desc struct and initialize upon filling required information
	fops_table_t * file_op_table_ptr;
	uint32_t inode;
	uint32_t file_position;
	uint32_t flags;
	int32_t pos;

	//check if the filename is valid
	if (filename == NULL) return ERROR;
	//check if there is room in the current PCB
	pos = find_open_idx(scheduler.curr_process);
	if (pos == ERROR) return ERROR;


	// fill out fops table based on file type
	dentry_t dentry;

	// check for valid name of file
	if(read_dentry_by_name (filename, &dentry) == ERROR)
		return ERROR;

	// RTC file
	if(dentry.file_type == RTC_FILE_TYPE){
		file_op_table_ptr = &rtc_fops_table;
		inode = 0;
	}

	// directory file
	else if(dentry.file_type == DIR_FILE_TYPE){
		file_op_table_ptr = &dir_fops_table;
		inode = 0;
	}

	// regular file
	else if(dentry.file_type == REG_FILE_TYPE){
		file_op_table_ptr = &reg_fops_table;
		inode = get_inode_from_name(filename);
	}

	// unknown file type
	else
		return ERROR;

	// set file in-use bit
	flags |= IN_USE;

	// initialize the file descriptor
	file_position = 0;
	file_desc_t fd = {file_op_table_ptr, inode, file_position, flags};

	//add the file descriptor to the array
	scheduler.curr_process->file_desc_array[pos] = fd;
	scheduler.curr_process->capacity += 1;
	fd.file_op_table_ptr->open(filename);
	return pos;
}


/*
 * int32_t close(int32_t fd)
 *   DESCRIPTION: Closes the specified file descriptor
 *   INPUTS: fd - desired file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, ERROR on failure
 *   SIDE EFFECTS: Removes file descriptor from current PCB's file array
 */
int32_t close(int32_t fd) {
	// fd is out-of-bounds
	if(fd < 0 || fd >= FD_ARRAY_MAX)
		return ERROR;

	// do not let user close STDIN or STDOUT
	if(fd == STDIN_FD || fd == STDOUT_FD)
		return ERROR;

	file_desc_t file_desc = scheduler.curr_process->file_desc_array[fd];

	// check if this fd has not been opened yet
	if((file_desc.flags & IN_USE) == UNUSED)
		return ERROR;

	// set the fd as not in use
	scheduler.curr_process->file_desc_array[fd].flags &= ~IN_USE;

	scheduler.curr_process->capacity--;

	return file_desc.file_op_table_ptr->close(fd);
}

/*
 * int32_t getargs(uint8_t* buf, int32_t nbytes)
 *   DESCRIPTION: Reads command line args into user-level buffer
 *   INPUTS: buf -- the user level buffer to compy into
 * 			 nbytes -- number of bytes to be copied
 *   OUTPUTS: none
 *   RETURN VALUE: ERROR error : 0 success
 *   SIDE EFFECTS: writes to buf
 */
int32_t getargs(uint8_t* buf, int32_t nbytes) {
    uint32_t length;
    
    length = strlen(scheduler.curr_process->args);

    if(buf == NULL)
        return ERROR;

	if(length + 1 > nbytes || length == 0)
		return ERROR;

	strcpy((int8_t*)buf, (const int8_t*)scheduler.curr_process->args);

	return 0;
}

/*
 * int32_t vidmap(uint8_t** screen_start)
 *   DESCRIPTION: Maps the text-mode video memory into userspace at screen_start
 *   INPUTS: screen_start - address of pointer to start of video mem base address in user space
 *   OUTPUTS: memory of screen_start is updated with start of video memory in kernel space
 *   RETURN VALUE: virtual address of video memory
 *   SIDE EFFECTS: Sets up proper paging for video
 */
int32_t vidmap(uint8_t** screen_start) {
	if(screen_start == NULL)
		return ERROR;
    if((uint32_t)screen_start < ALIGNED_128MB || (uint32_t)screen_start >= ALIGNED_132MB)
        return ERROR;

    page_directory[VIDMAP_PG_DIR_OFFSET] = ((uint32_t)vidmap_page_table_array[curr_term_idx]) | USER_SUPERVISOR | READ_WRITE | PRESENT;
    vidmap_page_table_array[curr_term_idx][0] = VID_MEM | READ_WRITE | USER_SUPERVISOR | PRESENT;

    *screen_start = (uint8_t*)ALIGNED_132MB;

    return ALIGNED_132MB;
}

/*
 * int32_t set_handler(int32_t signum, void* handler_address)
 *   DESCRIPTION: ...
 *   INPUTS: ...
 *   OUTPUTS: ...
 *   RETURN VALUE: ...
 *   SIDE EFFECTS: ...
 */
int32_t set_handler(int32_t signum, void* handler_address) {
	return ERROR;
}

/*
 * int32_t sigreturn(void)
 *   DESCRIPTION: ...
 *   INPUTS: ...
 *   OUTPUTS: ...
 *   RETURN VALUE: ...
 *   SIDE EFFECTS: ...
 */
int32_t sigreturn(void) {
	return ERROR;
}

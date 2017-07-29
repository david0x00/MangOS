#include "file_sys.h"
#include "pcb.h"
#include "terminal.h"
#include "sched.h"

int32_t curr_term_idx;
pcb_t* pcb_term[NUM_TERMS];
uint32_t pid_arr[MAX_PROG_NUM];


/* All the different types of fops tables we will need are found below */
fops_table_t std_fops_table = {terminal_open, terminal_read, terminal_write, terminal_close};
fops_table_t rtc_fops_table = {rtc_open, rtc_read, rtc_write, rtc_close};
fops_table_t dir_fops_table = {open_directory, read_directory, write_directory, close_directory};
fops_table_t reg_fops_table = {open_file, read_file, write_file, close_file};

/*
 * void init_pcb_one_time()
 *   DESCRIPTION: initializes PCB setup for the OS.  Call on bootup
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: sets up some pcb things
 */
void init_pcb_one_time() {
	uint32_t i;
	for(i = 0; i < NUM_TERMS; i++) {
		pcb_term[i] = NULL;
	}
	curr_term_idx = -1;
	num_processes = 0;
	for(i = 0; i < MAX_PROG_NUM; i++) {
		pid_arr[i] = PID_AVAILABLE;
	}
}

/*
 * uint32_t get_available_pid()
 *   DESCRIPTION: Gets the earliest available PID
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: returns the first avaiable PID possible, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t get_available_pid() {
	uint32_t i;
	for(i = 0; i < MAX_PROG_NUM; i++)
		if(pid_arr[i] == PID_AVAILABLE) {
			return i;
		}
	return ERROR;
}

/*
 * uint32_t set_pid()
 *   DESCRIPTION: Sets the index in the PCB array
 *   INPUTS: pid - the pid we want to set
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t set_pid(uint32_t pid) {
	if(pid_arr[pid] == PID_USED)
		return ERROR;
	pid_arr[pid] = PID_USED;
	return 0;
}

/*
 * uint32_t free_pid()
 *   DESCRIPTION: Frees the index in the PCB array
 *   INPUTS: pid - the pid we want to free
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t free_pid(uint32_t pid) {
	if(pid_arr[pid] == PID_AVAILABLE)
		return ERROR;
	pid_arr[pid] = PID_AVAILABLE;
	return 0;
}

/*
 * uint32_t init_pcb(pcb_t * it)
 *   DESCRIPTION: initializes a given PCB.
 *   INPUTS: it - this is a pointer to a pcb
 *   OUTPUTS: it
 *   RETURN VALUE: -1 if full or error : 0 on success
 *   SIDE EFFECTS: modifies it.
 */
int32_t init_pcb(pcb_t* it) {
	uint32_t i;
	int fd;
	if (it == NULL) return ERROR;

	// create file descriptor for stdin and stdout (they're the same)
	file_desc_t fd_std;
	fd_std.file_op_table_ptr = &std_fops_table;
	fd_std.inode = 0;
	fd_std.file_position = 0;
	fd_std.flags = IN_USE;

	// add in file descriptor into PCB
	it->file_desc_array[STDIN_FD] = fd_std;
	it->file_desc_array[STDOUT_FD] = fd_std;

	//initialize args to empty string
	for(i = 0; i < BUF_SIZE; ++i) {
		it->args[i] = '\0';
	}

	// init all other file descriptors to not in use
	for(fd = 2; fd < FD_ARRAY_MAX; fd++){
		file_desc_t fd_unused;
		fd_unused.flags = UNUSED;
		it->file_desc_array[fd] = fd_unused;
	}

	//update current pcb
	num_processes++;
	// Start at the bottom of the 8KB block
	it->curr_esp = (uint32_t)it + ALIGNED_8KB - 4;
	it->curr_ebp = (uint32_t)it + ALIGNED_8KB - 4;

	curr_term_idx = curr_term->tid;
	it->tid = curr_term_idx;
	it->parent = pcb_term[curr_term_idx];
	pcb_term[curr_term_idx] = it;


	//update capacity
	it->capacity = 2;

	//update scheduler
	add_process_to_runqueue(&scheduler, it);

	return 0;
}

/*
 * uint32_t find_open_idx(pcb_t * par)
 *   DESCRIPTION: finds open index in file descriptor array
 *   INPUTS: pcb - a pcb to check
 *   OUTPUTS: none
 *   RETURN VALUE: -1 if full or error : open index on success
 *   SIDE EFFECTS: none
 */
int32_t find_open_idx(pcb_t* pcb) {
	uint32_t i, curr_flags;
	if (pcb == NULL) return ERROR;
	if (pcb->capacity >= FD_ARRAY_MAX) return ERROR;

	// skip over STDIN_FD and STDOUT_FD
	for (i = 2; i < FD_ARRAY_MAX; i++) {
		// check if ith fd is in use
		curr_flags = pcb->file_desc_array[i].flags;
		if ((curr_flags & IN_USE) == UNUSED)
			return i;
	}
	return ERROR;
}

#ifndef _PCB_H
#define _PCB_H

#include "file_sys.h"
#include "keyboard.h"
#include "terminal.h"
#include "rtc.h"

#define FD_ARRAY_MAX 		8

#define MAX_PROG_NUM		6

#define STDIN_FD			0
#define STDOUT_FD			1
#define RTC_FD				2
#define IN_USE				0x00000001
#define UNUSED				0x00000000

#define PID_AVAILABLE		0
#define PID_USED			1

#define ALIGNED_8KB 		0x2000

#define ERROR				-1

uint32_t num_processes;

/* general struct for a file descriptor*/
typedef struct fops_table_t
{
	int32_t (*open) (const uint8_t* filename);
	int32_t (*read) (int32_t fd, void* buf, int32_t nbytes);
	int32_t (*write) (int32_t fd, const void* buf, int32_t nbytes);
	int32_t (*close) (int32_t fd);
} fops_table_t;

typedef struct file_desc_t
{
	fops_table_t* file_op_table_ptr; // pointer to file operations jump table.
	uint32_t inode;	//inode position
	uint32_t file_position;	//position of file in file_desc_array
	uint32_t flags; //needs to indicate "in-use", ....
} file_desc_t;

/* general struct for a PCB */
typedef struct pcb_t {
	uint32_t tid;			//terminal id [0,2]
	uint32_t pid;			//process control block id [0,5]
	uint32_t capacity;		//amount of files currently in array
	uint32_t esp;			//parent's esp to return to in halt
	uint32_t ebp;			//parent's ebp to return to in halt
	uint32_t curr_esp;		//current process's esp to return to when context switching
	uint32_t curr_ebp;		//current process's ebp to return to when context switching
	struct pcb_t* parent;	//pointer to parent pcb
	char args[BUF_SIZE];	//stores arguments as array of chars
	file_desc_t file_desc_array[FD_ARRAY_MAX];	// array of file descriptors
	struct pcb_t * next;	//next pcb for the scheduler
	struct pcb_t * prev;	//previous pcb for the scheduler
	uint32_t state;			//task is excecuting currently or waiting to execute
							//one of {TASK_RUNNING, TASK_SLEEPING,
							//		  TASK_INTERRUPTIBLE, TASK_UNINTERRUPTIBLE,
							//		  TASK_STOPPED, TASK_ZOMBIE}
							//full description in sched.h
} pcb_t;


void init_pcb_one_time();

int32_t get_available_pid();

int32_t set_pid(uint32_t pid);

int32_t free_pid(uint32_t pid);

int32_t init_pcb(pcb_t* it);

int32_t find_open_idx(pcb_t* pcb);

extern uint32_t pid_arr[MAX_PROG_NUM];

//variable that will house the current PCB.
extern int32_t curr_term_idx;
extern pcb_t* pcb_term[NUM_TERMS];

// file operations table for all types of files
extern fops_table_t std_fops_table;
extern fops_table_t rtc_fops_table;
extern fops_table_t dir_fops_table;
extern fops_table_t reg_fops_table;


#endif

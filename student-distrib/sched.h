#ifndef _IDT_H
#define _IDT_H


#include "pcb.h"

//task is excecuting currently or waiting to execute
//task is in a run queue on some processor
#define TASK_RUNNING 0

//task is still running, but not currently being executed
#define TASK_SLEEPING 5

//task is sleeping on a semaphore/condition/signal
//task is in a wait queue
//can be made runnable by delivery of signal
#define TASK_INTERRUPTIBLE 1

//task is busy with something that can’t be stopped
//cannot be made runnable by delivery of signal
//e.g., a device that will stay in unrecoverable state without further task interaction
#define TASK_UNINTERRUPTIBLE 2

//task is stopped
//task is not in a queue; must be woken by signal
#define TASK_STOPPED 3

//task has terminated
//task state retained until parent collects exit status information
//task is not in a queue
#define TASK_ZOMBIE 4

#define SLICE 		10

#define ERROR		-1

/*genreral struct for our scheduler*/
typedef struct runqueue_t
{
	uint32_t size;
	uint32_t max_size;
	struct pcb_t * curr_process;
	struct pcb_t * head;
	struct pcb_t * tail;
} runqueue_t;

/*this is the main scheduler for our OS*/
runqueue_t scheduler;

/*call once to set up the scheduler*/
extern void sched_init();

/*add a process to the scheduler*/
extern int32_t add_process_to_runqueue(runqueue_t * rq, struct pcb_t * new_p);

/*remove a process from the scheduler*/
extern int32_t remove_process_from_runqueue(runqueue_t * rq, struct pcb_t * old_p);

/*rotate the runqueue*/
int32_t runqueue_rotate();

/*queue pop method*/
pcb_t * pop(runqueue_t * rq, pcb_t * old_p);

/*queue push method*/
int32_t push(runqueue_t * rq, pcb_t * new_p);

#endif

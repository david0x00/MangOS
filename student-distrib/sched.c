#include "sched.h"
#include "x86_desc.h"
#include "pcb.h"
#include "systemcalls.h"
#include "paging_init.h"


/*
 * void sched_init()
 *   DESCRIPTION: initializes the scheduler
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initializes the scheduler
 */
void sched_init() {
	scheduler.size = 0;
	scheduler.max_size = MAX_PROG_NUM;
	scheduler.curr_process = NULL;
	scheduler.head = NULL;
	scheduler.tail = NULL;
}


/*
 * uint32_t add_process_to_runqueue()
 *   DESCRIPTION: adds a process to the run queue
 *   INPUTS: rq - a run queue to change
 			 new_p - the new process to add
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - success, ERROR - failure
 *   SIDE EFFECTS: changes the runqueue
 */
int32_t add_process_to_runqueue(runqueue_t * rq, pcb_t * new_p) {
	//make sure the new_p is valid
	if (new_p == NULL || rq == NULL) return ERROR;
	//make sure a process can be added
	if (rq->size == rq->max_size) return ERROR;
	//if new process has a parent, then put the parent to sleep
	if (new_p->parent != NULL) {
		new_p->parent->state = TASK_SLEEPING;
	}
	//push, and update.
	push(rq, new_p);
	new_p->state = TASK_RUNNING;
	rq->curr_process = new_p;
	return 0;
}


/*
 * uint32_t remove_process_from_runqueue()
 *   DESCRIPTION: removes a process from the run queue
 *   INPUTS: rq - a run queue to change
 			 new_p - the process to remove
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - success, ERROR - failure
 *   SIDE EFFECTS: changes the runqueue
 */
int32_t remove_process_from_runqueue(runqueue_t * rq, pcb_t * old_p) {
	//check for NULL
	if (rq == NULL || old_p == NULL) return ERROR;
	//check if head is NUll
	if(rq->head == NULL){
		return ERROR;
	}

	//make old_p a temp variable
	pcb_t * current = old_p;

	//examine each case and remove the pcb respectively
	if (current->next == NULL && current->prev == NULL) {
		rq->head = NULL;
		rq->tail = NULL;
	} else if (current->next == NULL && current->prev != NULL) {
		current->prev->next = NULL;
		rq->tail = current->prev;
	} else if (current->next != NULL && current->prev == NULL) {
		current->next->prev = NULL;
		rq->head = current->next;
	} else {
		current->next->prev = current->prev;
		current->prev->next = current->next;
	}

	//decrement the old state and make old_p a Zombie
	old_p->state = TASK_ZOMBIE;
	rq->size -= 1;

	//if old_p's parent is not null, make it the curr_process
	if (old_p->parent != NULL){
		old_p->parent->state = TASK_RUNNING;
		rq->curr_process = old_p->parent;
	}
	//if the parent is null, but there are still processes on the queue
	else if(rq->head != NULL){
		rq->curr_process = rq->head;
	}
	//if everything is gone, curr_process is NULL
	else{
		rq->curr_process = NULL;
	}
	return 0;
}


/*
 * uint32_t remove_process_from_runqueue()
 *   DESCRIPTION: pops the head from the runqueue and pushes it to the tail
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - success, ERROR - failure
 *   SIDE EFFECTS: changes the runqueue
 */
int32_t runqueue_rotate() {
	//check for null
	if (scheduler.head == NULL || scheduler.tail == NULL) return ERROR;
	//update the linked list putting the head at the back of the queue
	if (scheduler.head != scheduler.tail) {
		//here I do not use push and pop because this is less lines.
		scheduler.tail->next = scheduler.head;
		scheduler.head->prev = scheduler.tail;
		scheduler.head = scheduler.head->next;
		scheduler.tail = scheduler.tail->next;
		scheduler.tail->next = NULL;
		scheduler.head->prev = NULL;
	}
	return 0;
}


/*
 * uint32_t push()
 *   DESCRIPTION: pushes to the end of the queue
 *   INPUTS: rq - a runqueue
 * 			 new_p - a pcb to be pushed
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - success, ERROR - failure
 *   SIDE EFFECTS: changes the runqueue
 */
int32_t push(runqueue_t * rq, pcb_t * new_p) {
	//check for NULL
	if (rq == NULL || new_p == NULL) return ERROR;
	//check if size condition is met
	if (rq->size >= rq->max_size) return ERROR;
	//if this is the first element
	if (rq->size == 0) {
		rq->head = new_p;
		rq->head->prev = NULL;
		rq->tail = new_p;
		rq->tail->next = NULL;
	}
	//if this is not the first
	else {
		rq->tail->next = new_p;
		new_p->prev = rq->tail;
		new_p->next = NULL;
		rq->tail = new_p;
	}
	//increment size
	rq->size += 1;
	return 0;
}

/*
 * pcb_t * pop()
 *   DESCRIPTION: pops from the front of the queue
 *   INPUTS: rq - a runqueue
 * 			 old_p - a pcb to be pushed
 *   OUTPUTS: none
 *   RETURN VALUE: ret - success, NULL - failure
 *   SIDE EFFECTS: changes the runqueue
 */
pcb_t * pop(runqueue_t * rq, pcb_t * old_p) {
	//the return value
	pcb_t * ret;
	//check for null
	if (rq == NULL || old_p == NULL) return NULL;
	//check for in bounds
	if (rq->size <= 0) return NULL;
	ret = rq->head;
	//if there is only 1 element in queue
	if (rq->size == 1) {
		rq->head = NULL;
		rq->tail = NULL;
	}
	//if there is more than 1 element in queue
	else {
		rq->head = rq->head->next;
		rq->head->prev = NULL;
	}
	//decrement size
	// ret->prev == NULL;
	// ret->next == NULL;
	rq->size -= 1;
	return ret;
}

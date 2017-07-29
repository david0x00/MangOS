/* pit.c - Functions to interact with the RTC
 * vim:ts=4 noexpandtab
 */
#include "sched.h"
#include "x86_desc.h"
#include "pcb.h"
#include "systemcalls.h"
#include "paging_init.h"
#include "i8259.h"
#include "pit.h"
#include "lib.h"
#include "terminal.h"

/*
 * void pit_init
 *   DESCRIPTION: Initializes PIT interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Turns on IRQ 0
 *  Source 1: http://proquest.safaribooksonline.com/book/operating-systems-and-server-administration/
 *   linux/0596005652/6dot-timing-measurements/understandlk-chp-6-sect-1
 *  Source 2: http://wiki.osdev.org/Programmable_Interval_Timer
 */
void
pit_init(void)
{


    outb(RATE_GEN_MODE, COMMAND_REG);
    outb(LATCH & 0xFF, CHANNEL_0_PORT);
    outb(LATCH >> 8, CHANNEL_0_PORT);


    // enable associated interrupt on PIC
    enable_irq(PIT_IRQ);
}


/*
 * void pit_int_handler
 *   DESCRIPTION: Handles PIT interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Signals that interrupt has been handled to PIT_IRQ
 *
 */
void
pit_int_handler()
{
    cli();

    // if(terminals[1].active == INACTIVE) {
    //     switch_term(1);
    //     curr_term->active = ACTIVE;
    //     send_eoi(PIT_IRQ);
    //     // asm volatile (
    //     //     "popfl;"
    //     //     "popal;"
    //     //     :
    //     //     :
    //     // );
    //     execute((const uint8_t*)"shell");
    // }
    // if(terminals[0].active == INACTIVE) {
    //     switch_term(0);
    //     curr_term->active = ACTIVE;
    //     send_eoi(PIT_IRQ);
    //     // asm volatile (
    //     //     "popfl;"
    //     //     "popal;"
    //     //     :
    //     //     :
    //     // );
    //     execute((const uint8_t*)"shell");
    // }


    //upnext is the next process to run
    pcb_t * up_next;
    //current is the current process running
    pcb_t * current;

    //handle if scheduler is empty.  nothing to schedule
    if (scheduler.head == NULL || scheduler.tail == NULL) return;

    //geth the current process temporarily.
    if (scheduler.curr_process != NULL) {
        current = scheduler.curr_process;
    } else {
        return;
    }

    //rotate the queue
    if (runqueue_rotate() == ERROR) return;

    //rotate the queue until we find a process that is running
    while (scheduler.head->state != TASK_RUNNING) {
        runqueue_rotate();
    }

    //update the current process and store it temporarily.
    up_next = scheduler.head;
    scheduler.curr_process = up_next;
    //prepare for context switch
    page_directory[EXEC_PG_DIR_OFFSET] = (ALIGNED_8MB + ALIGNED_4MB * up_next->pid) | EXEC_PG_DIR_FLAGS;

    if(up_next->tid == curr_term_idx)
        vidmap_page_table_array[current->tid][0] = (uint32_t)(terminals[current->tid].pte) | READ_WRITE | USER_SUPERVISOR | PRESENT;
    else
        vidmap_page_table_array[up_next->tid][0] = (uint32_t)(terminals[up_next->tid].pte) | READ_WRITE | USER_SUPERVISOR | PRESENT;

    page_directory[VIDMAP_PG_DIR_OFFSET] = (uint32_t)(vidmap_page_table_array[up_next->tid]) | USER_SUPERVISOR | READ_WRITE | PRESENT;
    vidmap_page_table_array[curr_term_idx][0] = VID_MEM | READ_WRITE | USER_SUPERVISOR | PRESENT;

    flush_tlb();
    //switch the stack
    tss.ss0 = KERNEL_DS;
    tss.esp0 = ALIGNED_8MB - (up_next->pid * ALIGNED_8KB) - ALIGNED_4B;
    //store the Stack registers for the old process
    asm volatile (
        "movl %%esp, %0;"
        "movl %%ebp, %1;"
        : "=m" (current->curr_esp), "=m" (current->curr_ebp)
        :
    );
    //load the stack registers for the new process.
    asm volatile (
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        :
        : "m" (up_next->curr_esp), "m" (up_next->curr_ebp)
    );

    //send the interupt end signal
    send_eoi(PIT_IRQ);  // signal PIC
    sti();
}

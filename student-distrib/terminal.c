/* terminal.c - Functions to interact with the terminal
 * vim:ts=4 noexpandtab
 */

#include "terminal.h"
#include "keyboard.h"
#include "rtc.h"
#include "lib.h"
#include "paging_init.h"
#include "pcb.h"
#include "sched.h"
#include "systemcalls.h"


terminal_t terminals[NUM_TERMS];
terminal_t* curr_term;

/*
 * void clear_terminal
 *   DESCRIPTION: Resets the display screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Resets cursor back to top and acts like a "new" terminal screen
 */
void
clear_terminal(){
	clear();
	set_screen_x(0);
	set_screen_y(0);
	puts_mod(USER_LINE);
}

/*
 * int32_t terminal_open
 *   DESCRIPTION: Open syscall for terminal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 *
 */
int32_t
terminal_open(const uint8_t* filename) {
    return 0;
}

/*
 * int32_t terminal_close
 *   DESCRIPTION: Close syscall for terminal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: ERROR
 *   SIDE EFFECTS: Not allowed to close stdout
 *
 */
int32_t
terminal_close(int32_t fd){
    return -1;
}

/*
 * void temrinal_write_char()
 *   DESCRIPTION: writes a character to the terminal
 *   INPUTS:
 *      -- key - the char to be written
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  writes a char to screen
 */
void terminal_write_char(char key) {
    if(key != 0){
        if(key == '\b'){
            handle_backspace();
        }

        if (key == '\n') {
            handle_newline();
        }

        // don't write anymore characters if buf is full
        else if(curr_term->buf_idx < BUF_SIZE){
            curr_term->buf[curr_term->buf_idx] = key;
            curr_term->buf_idx++;
            putc_mod(key);
        }
    }
}


/*
 * int32_t terminal_write(const char * in_buf, int32_t nbytes) {
 *   DESCRIPTION: writes the contents of a buffer to the screen
 *   INPUTS:
 *      -- in_buf - the buffer to write to the screen
 *      -- nbytes - the number of bytes to write to the screen
 * 			make sure you know the size of in_buf
 *   OUTPUTS: none
 *   RETURN VALUE: returns the number of bytes actually written to the screen
 *   SIDE EFFECTS: None
 */
int32_t terminal_write(int32_t fd, const void* in_buf, int32_t nbytes) {
    int32_t i, ret = 0;

    // if stdin is calling it, call should fail
    if(fd == STDIN_FD){
        return ERROR;
    }

    // recast from void pointer
    char* char_in_buf = (char*) in_buf;
    for (i = 0; i < nbytes; i++) {
        // terminal_write_char(char_in_buf[i]);

		if(curr_term_idx == scheduler.curr_process->tid)
        	putc_mod(char_in_buf[i]);
		else{
			/* write to proper backup buffer for
			terminals[scheduler.head->tid].pte */
            putc_page(char_in_buf[i], &(terminals[scheduler.curr_process->tid]));
		}
        ret++;
    }
	update_cursor(get_screen_y(), get_screen_x());

    return ret;
}

/*
 * void switch_term
 *   DESCRIPTION: Allows Alt + F1/F2/F3 functionality
 *   INPUTS: id - terminal to switch to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Switches to the terminal with same id
 */
void
switch_term(uint32_t id){

	// save vidmem to backup terminal
	if(curr_term != NULL) {
		memcpy(curr_term->pte, (char*) VID_MEM, ALIGNED_4KB);
	}

	// restore backup term vid mem to vid mem
	vidmap_page_table_array[curr_term_idx][0] = (uint32_t)(curr_term->pte) | READ_WRITE | USER_SUPERVISOR | PRESENT;
	curr_term = &(terminals[id]);
	curr_term_idx = id;

	page_directory[VIDMAP_PG_DIR_OFFSET] = ((uint32_t)vidmap_page_table_array[curr_term_idx]) | USER_SUPERVISOR | READ_WRITE | PRESENT;
	vidmap_page_table_array[curr_term_idx][0] = VID_MEM | READ_WRITE | USER_SUPERVISOR | PRESENT;
	flush_tlb();

	memcpy((char*) VID_MEM, curr_term->pte, ALIGNED_4KB);
	set_screen_x(curr_term->x_loc);
	set_screen_y(curr_term->y_loc);
	update_cursor(curr_term->y_loc, curr_term->x_loc);
}


/*
 * void init_terms
 *   DESCRIPTION: Inits the terminals
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Creates terminal structs
 */
void
init_terms(){
    int i;
    for(i = 0; i < NUM_TERMS; i++)
	    init_terminal(i);

	// curr_term = &(terminals[2]);
	curr_term = &(terminals[0]);
	curr_term->active = ACTIVE;
	clear();
    set_screen_x(0);
    set_screen_y(0);
}


/*
 * init_terminal
 *   DESCRIPTION: Inits the terminal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Creates a display for the terminal
 */
void
init_terminal(uint32_t id){
    int i;

	terminal_t* term = &(terminals[id]);
    term->enter_flag = 0;
	term->active = INACTIVE;
	term->tid = id;
	term->pte = (char*) term_pte[id];
	term->x_loc = 0;
	term->y_loc = 0;
    term->auto_comp_index = 0;
    term->cmd_hist_idx = 0;
    for(i = 0; i < HISTORY_SIZE; i++){
        strcpy(term->cmd_history[i], "no cmd here");
    }
	term->buf_idx = 0;
    memset(term->buf, '\0', BUF_SIZE);
    memset(term->return_buffer, '\0', RET_BUF_SIZE);
}

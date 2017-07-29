#ifndef _TERM_H
#define _TERM_H

#include "types.h"
#include "keyboard.h"

#define USER_LINE	    ""

#define STDIN_FD	    0  

#define NUM_TERMS       3
#define INACTIVE        0
#define ACTIVE          1
#define NUM_PROG_TERM   4


/* Inits terminal */
void init_terms();
void init_terminal(uint32_t id);
/* Clear the terminal screen */
extern void clear_terminal();
/* System calls for terminal */
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
int32_t terminal_write(int32_t fd, const void* in_buf, int32_t nbytes);
void terminal_write_char(char key);	// helper for keyboard_write syscall


/* Switch between 3 terminals */
void switch_term(uint32_t id);


/* general struct for a file descriptor*/
typedef struct terminal_t
{
    uint32_t tid;
    char* pte;
	int32_t x_loc, y_loc;
    uint32_t active;
    int auto_comp_index;
    int cmd_hist_idx;
    char cmd_history[HISTORY_SIZE][BUF_SIZE];
    int32_t buf_idx;
    uint32_t enter_flag;
    char buf[BUF_SIZE];
    char return_buffer[RET_BUF_SIZE];
} terminal_t;

extern terminal_t terminals[NUM_TERMS];
extern terminal_t* curr_term;

#endif /* _TERM_H */

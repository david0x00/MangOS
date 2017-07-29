/* keyboard.h - Defines used in interactions with the keyboard
 * vim:ts=4 noexpandtab
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

#define KEYBOARD_PORT	0x60
#define KEYBOARD_IRQ	1
#define NUM_KEYS		128
#define BUF_SIZE		128
#define RET_BUF_SIZE	129

#define LOW_PORT        0x0F
#define HIGH_PORT       0x0E
#define CURSOR_START    0x3D5
#define VGA_IND_REG     0x3D4

// scan code for caps lock
#define CAPS_LOCK_SC	0x3A

// shift keypresses
#define LSHIFT_PRESSED	0x2A
#define LSHIFT_RELEASED	0xAA
#define RSHIFT_PRESSED	0x36
#define RSHIFT_RELEASED 0xB6

// ctrl keypresses
#define CTRL_PRESSED	0x1D
#define CTRL_RELEASED	0x9D

// alt keypresses
#define ALT_PRESSED		0x38
#define ALT_RELEASED	0xB8

// function keys
#define F1_SC			0x3B
#define F2_SC			0x3C
#define F3_SC			0x3D

// arrow keys
#define UP_ARROW		0x48
#define DOWN_ARROW		0x50

#define NUM_COLS        80

// difference between an uppercase and lowercase letter in ASCII
#define LOWER_UPPER_DIFF	32

#define STDOUT_FD			1

#define ERROR				-1

#define HISTORY_SIZE		5

/* Externally-visible functions */

/* Initialize the keyboard */
extern void keyboard_init(void);
/* Reads scan code from keyboard */
extern char get_scan_code();
/* Returns the character represented by the scancode */
extern char get_char();
/* Interrupt handler for keyboard */
extern void keyboard_int_handler();
/* Delete from buffer and update display */
void handle_backspace();
/* Handles enter key input */
void handle_newline();
/* Handles tab key input with autocomplete */
void handle_tab();
/* Writes characters in buffer to terminal */
void write_buffer_to_screen();
/* Erases characters from screen */
void erase_n_chars(int n);
/* System calls for keyboard */
int32_t terminal_read(int32_t fd, void* in_buf, int32_t nbytes);
/* Cursor moves with text */
void update_cursor(int row, int col);
/* Helper function for autocomplete */
int32_t find_autocomp_index();
/* Creates instance of a terminal */
void start_terminal();


//buffer identifiers
extern char return_buffer[RET_BUF_SIZE];
// extern char buf[BUF_SIZE];
// extern int buf_idx;
//if the newline has been pressed
extern int enter_flag;


#endif /* _KEYBOARD_H */

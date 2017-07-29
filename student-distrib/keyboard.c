/* keyboard.c - Functions to interact with the keyboard
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "keyboard.h"
#include "lib.h"
#include "terminal.h"
#include "file_sys.h"
#include "pcb.h"
#include "systemcalls.h"
#include "sched.h"


// active high flag for caps lock
int caps_lock_on = 0;
// active high flag for shifts
int lshift_pressed = 0;
int rshift_pressed = 0;
// active high flag for ctrl
int ctrl_pressed = 0;
// active high flag for alt
int alt_pressed = 0;


/*
 * keyboard_init
 *   DESCRIPTION: Initializes keyboard interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Turns on IRQ 8 (periodic interrupts of 2 Hz)
 *
 * Inspiration drawn from: http://wiki.osdev.org/RTC
 */
void
keyboard_init(void)
{

    // enable associated interrupt on PIC
    enable_irq(KEYBOARD_IRQ);
}


/*
 * get_scan_code
 *   DESCRIPTION: Reads the scan code from the keyboard
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: The scan code from the keyboard
 *   SIDE EFFECTS: none
 *
 * Inspiration drawn from: http://wiki.osdev.org/PS/2_Keyboard
 */
char
get_scan_code()
{
    return inb(KEYBOARD_PORT);
}

/*
 *  char get_char
 *   DESCRIPTION: Looks up scan code in the scancode2char array
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: The character represented by the scan code
 *   SIDE EFFECTS: none
 *
 * Inspiration drawn from: http://wiki.osdev.org/PS/2_Keyboard
 */
char
get_char()
{

    /* Source: http://www.osdever.net/bkerndev/Docs/keyboard.htm */
    static unsigned char scancode2char[NUM_KEYS] =
    {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
      '9', '0', '-', '=', '\b', /* Backspace */
      '\t',         /* Tab */
      'q', 'w', 'e', 'r',   /* 19 */
      't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
        0,          /* 29   - Control */
      'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
     '\'', '`',   0,        /* Left shift */
     '\\', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
      'm', ',', '.', '/',   0,              /* Right shift */
      '*',
        0,  /* Alt */
      ' ',  /* Space bar */
        0,  /* Caps lock */
        0,  /* 59 - F1 key ... > */
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  /* < ... F10 */
        0,  /* 69 - Num lock*/
        0,  /* Scroll Lock */
        0,  /* Home key */
        0,  /* Up Arrow */
        0,  /* Page Up */
      '-',
        0,  /* Left Arrow */
        0,
        0,  /* Right Arrow */
      '+',
        0,  /* 79 - End key*/
        0,  /* Down Arrow */
        0,  /* Page Down */
        0,  /* Insert Key */
        0,  /* Delete Key */
        0,   0,   0,
        0,  /* F11 Key */
        0,  /* F12 Key */
        0,  /* All other keys are undefined */
    };

    // shift for non-alphabetic characters (alphabetic characters are dealt with separately)
    static unsigned char shiftChars[NUM_KEYS] =
    {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*', /* 9 */
      '(', ')', '_', '+', '\b', /* Backspace */
      '\t',         /* Tab */
      'q', 'w', 'e', 'r',   /* 19 */
      't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n', /* Enter key */
        0,          /* 29   - Control */
      'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', /* 39 */
     '\"', '~',   0,        /* Left shift */
     '|', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
      'm', '<', '>', '\?',   0,              /* Right shift */
      '*',
        0,  /* Alt */
      ' ',  /* Space bar */
        0,  /* Caps lock */
        0,  /* 59 - F1 key ... > */
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  /* < ... F10 */
        0,  /* 69 - Num lock*/
        0,  /* Scroll Lock */
        0,  /* Home key */
        0,  /* Up Arrow */
        0,  /* Page Up */
      '-',
        0,  /* Left Arrow */
        0,
        0,  /* Right Arrow */
      '+',
        0,  /* 79 - End key*/
        0,  /* Down Arrow */
        0,  /* Page Down */
        0,  /* Insert Key */
        0,  /* Delete Key */
        0,   0,   0,
        0,  /* F11 Key */
        0,  /* F12 Key */
        0,  /* All other keys are undefined */
    };

    unsigned char c = get_scan_code();

    /* "Edge" cases (non-printable characters) */

    // enable caps lock if it is disabled
    if(c == CAPS_LOCK_SC){
        if(caps_lock_on == 0)
            caps_lock_on = 1;
        else{
            caps_lock_on = 0;
        }
        return 0;
    }

    // set shift, ctrl, and alt flags
    switch(c){
        case LSHIFT_PRESSED:    lshift_pressed = 1; return 0;
        case LSHIFT_RELEASED:   lshift_pressed = 0; return 0;
        case RSHIFT_PRESSED:    rshift_pressed = 1; return 0;
        case RSHIFT_RELEASED:   rshift_pressed = 0; return 0;
        case CTRL_PRESSED:      ctrl_pressed = 1;   return 0;
        case CTRL_RELEASED:     ctrl_pressed = 0;   return 0;
        case ALT_PRESSED:      alt_pressed = 1;    return 0;
        case ALT_RELEASED:      alt_pressed = 0;    return 0;
    }

    // arrow key handling (make sure to not extend past bounds of cmd_hist array)
    if(c == UP_ARROW){
        if(curr_term->cmd_hist_idx < HISTORY_SIZE - 1){
            curr_term->cmd_hist_idx++;

            // erase whats on the screen
            int prev_buf_idx = (strlen(curr_term->buf) > BUF_SIZE) ? BUF_SIZE : strlen(curr_term->buf);
            erase_n_chars(prev_buf_idx);

            // place historic command in buf
            strncpy(curr_term->buf, curr_term->cmd_history[curr_term->cmd_hist_idx], BUF_SIZE);
            puts_mod(curr_term->buf);
            curr_term->buf_idx = (strlen(curr_term->buf) > BUF_SIZE) ? BUF_SIZE : strlen(curr_term->buf);
            curr_term->auto_comp_index = find_autocomp_index();
        }
        return 0;
    }

    else if(c == DOWN_ARROW){
        if(curr_term->cmd_hist_idx > 0){
            curr_term->cmd_hist_idx--;

            // erase whats on the screen
            int prev_buf_idx = strlen(curr_term->buf);
            prev_buf_idx = (prev_buf_idx > BUF_SIZE) ? BUF_SIZE : prev_buf_idx;
            erase_n_chars(prev_buf_idx);

            // place historic command in buf
            strncpy(curr_term->buf, curr_term->cmd_history[curr_term->cmd_hist_idx], BUF_SIZE);
            puts_mod(curr_term->buf);
            curr_term->buf_idx = strlen(curr_term->buf);
            curr_term->buf_idx = (curr_term->buf_idx > BUF_SIZE) ? BUF_SIZE : curr_term->buf_idx;
            curr_term->auto_comp_index = find_autocomp_index();
        }

        return 0;
    }


    /* Switch terminals combination*/
    if(alt_pressed == 1){
        switch(c){
            case F1_SC:
                switch_term(0);
                return 0;
            case F2_SC:
                if(scheduler.size >= MAX_PROG_NUM && terminals[1].active == INACTIVE)
                    return 0;
                switch_term(1);
                if(curr_term->active == INACTIVE) {
                    curr_term->active = ACTIVE;
                    send_eoi(KEYBOARD_IRQ);  // signal PIC
                    clear();
                    if(execute((const uint8_t*)"shell") == ERROR)
                        curr_term->active = INACTIVE;
                }
                return 0;
            case F3_SC:
                if(scheduler.size >= MAX_PROG_NUM && terminals[2].active == INACTIVE)
                    return 0;
                switch_term(2);
                if(curr_term->active == INACTIVE) {
                    curr_term->active = ACTIVE;
                    send_eoi(KEYBOARD_IRQ);  // signal PIC
                    clear();
                    if(execute((const uint8_t*)"shell") == ERROR)
                        curr_term->active = INACTIVE;
                }
                return 0;
        }
    }



    /* General case (printable characters) */
    if(c < NUM_KEYS){

        char key = scancode2char[c];

        // "ctrl + l" clears the screen
        if(ctrl_pressed == 1 && (key == 'l')){
            clear_terminal();
            puts_mod(curr_term->buf);
            return 0;
        }

        // deal with alphabetic chars separately from all other characters
        else if( key >= 'a' && key <= 'z'){
            int shiftPressed = rshift_pressed | lshift_pressed;
            int capital = shiftPressed ^ caps_lock_on;
            // if capital letter, reassign the proper capital ASCII value
            key = (capital == 1) ? (key - LOWER_UPPER_DIFF) : key;
        }

        // only look at shift key for all other characters
        else{
            int shiftPressed = rshift_pressed | lshift_pressed;
            key = (shiftPressed == 1) ? shiftChars[c] : key;
        }

        return key;
      }
    else
        return 0;
}


/*
 * int32_t find_autocomp_index
 *   DESCRIPTION: Finds last space in buf
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: proper index for autocomplete using up/down arrows
 *   SIDE EFFECTS: none
 */
int32_t
find_autocomp_index(){
    int i = curr_term->buf_idx;

    // find last space in the buf
    while(i >= 0 && curr_term->buf[i] != ' '){
        i--;
    }
    return i + 1;
}


/*
 * void erase_n_chars
 *   DESCRIPTION: Erases the past n characters on screen
 *   INPUTS: n - number of characters to erase
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Updates screen by erasing characters
 */
void
erase_n_chars(int n){
    int32_t i;

    set_screen_x(get_screen_x() - n);
    for(i = 0; i < n; i++) {
        putc_mod(0);
    }
    set_screen_x(get_screen_x() - n);
}



/*
 * void keyboard_int_handler
 *   DESCRIPTION: Handles keyboard interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Signals that interrupt has been handled to KEYBOARD_IRQ and echos key back to screen
 */
void
keyboard_int_handler()
{
    cli();

    char key = 0;

    key = get_char();
    // only print a valid character to screen
    if(key != 0){

        if(key == '\n')
            handle_newline();

        else if(key == '\b'){
            handle_backspace();
        }

        else if(key == '\t'){
            handle_tab();
        }

        // don't write anymore characters if buf is full
        else if(curr_term->buf_idx < BUF_SIZE){
            curr_term->buf[curr_term->buf_idx++] = key;
            // update cursor for autocomplete
            if(key == ' '){
                curr_term->auto_comp_index = curr_term->buf_idx;
            }
            putc_mod(key);

             // save updated buffer as most recent cmd in history
            if(curr_term->cmd_hist_idx == 0)
                strncpy(curr_term->cmd_history[0], curr_term->buf, BUF_SIZE);
        }

    }

    // update location parameters
    curr_term->x_loc = get_screen_x();
    curr_term->y_loc = get_screen_y();
    update_cursor(get_screen_y(), get_screen_x());

    send_eoi(KEYBOARD_IRQ);  // signal PIC

    sti();
}


/*
 * void handle_newline
 *   DESCRIPTION: Deals with the enter key input
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Cursor is moved down a line, clears that line and buffer
 */
void
handle_newline(){
    //copy into return buffer
    int32_t i;

    for (i = 0; i < curr_term->buf_idx; i++) {
        curr_term->return_buffer[i] = curr_term->buf[i];
    }
    curr_term->return_buffer[i] = '\n';

    curr_term->auto_comp_index = 0;

    // save command in buffer
    strncpy(curr_term->cmd_history[0], curr_term->buf, BUF_SIZE);
    for(i = HISTORY_SIZE - 1; i >= 1; i--){
        strncpy(curr_term->cmd_history[i], curr_term->cmd_history[i - 1], BUF_SIZE);
    }
    curr_term->cmd_hist_idx = 0;

    // clear current buffer
    memset(curr_term->buf, '\0', BUF_SIZE);
    curr_term->buf_idx = 0;

    strncpy(curr_term->cmd_history[0], curr_term->buf, BUF_SIZE);

    putc_mod('\n');
    curr_term->enter_flag = 1;
}

/*
 * void handle_backspace
 *   DESCRIPTION: Erases character from buffer and updates display
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Cursor is moved left once and buffer is updated
 */
void
handle_backspace(){
    // boundary check
    if(curr_term->buf_idx > 0){
        // erase previous char in buffer
        curr_term->buf_idx--;

        /* Erasing space character special case */
        // reset the autocomplete index to the previous word
        if(curr_term->buf[curr_term->buf_idx] == ' ' && curr_term->buf[curr_term->buf_idx - 1] != ' '){
            curr_term->auto_comp_index = curr_term->buf_idx;
            while((curr_term->auto_comp_index > 0) && (curr_term->buf[curr_term->auto_comp_index - 1] != ' ')){
                curr_term->auto_comp_index--;
            }
        }

        // multiple spaces between args
        else if(curr_term->buf[curr_term->buf_idx] == ' ' && curr_term->buf[curr_term->buf_idx - 1] == ' '){
            curr_term->auto_comp_index--;
        }

        curr_term->buf[curr_term->buf_idx] = '\0';
        erase_n_chars(1);

    } else {
        curr_term->auto_comp_index = 0;
    }
}

/*
 * void handle_tab
 *   DESCRIPTION: Autocomplete functionality
 *   INPUTS: none
 *   OUTPUTS: Writes to terminal buf with most common suffix
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Searches file list for matching autocompletes
 */
void
handle_tab(){

    int8_t fname[MAX_STRING_LEN];
    int8_t most_common_suffix[MAX_STRING_LEN];
    int32_t i, j, size_query, size_new_buf, matches = 0;
    
    size_query = curr_term->buf_idx - curr_term->auto_comp_index;

    // don't attempt to autocomplete if impossible
    if((size_query >= MAX_STRING_LEN) || (curr_term->auto_comp_index >= curr_term->buf_idx)){
        return;
    }

    memset(most_common_suffix, '\0', MAX_STRING_LEN);
    // linear search through all the files for possible results
    for (i = 0; i < MAX_DENTRIES; i++) {
        strncpy(fname, all_files[i].file_name, MAX_STRING_LEN);
        // check if this filename is a candidate for completion
        if(strncmp(fname, &(curr_term->buf[curr_term->auto_comp_index]), size_query) == 0){
            matches++;
            if(matches == 1){
                strncpy(most_common_suffix, &fname[size_query], MAX_STRING_LEN - size_query);
            }

            // determine the most common suffix (or relative prefix?)
            else{
                for(j = size_query; j < MAX_STRING_LEN; j++){
                    if(fname[j] != most_common_suffix[j - size_query]){
                        memset(&most_common_suffix[j - size_query], '\0', MAX_STRING_LEN - j);
                        break;
                    }
                }
            }
        }
    }

    // check out of bounds for buffer first
    size_new_buf = strlen(most_common_suffix) + curr_term->buf_idx;

    if(size_new_buf > BUF_SIZE) {
        return;
    }

    puts_mod(most_common_suffix);
    strncpy(&(curr_term->buf[curr_term->buf_idx]), most_common_suffix, MAX_STRING_LEN - size_query);
    curr_term->buf_idx = strlen(curr_term->buf);
}

/*
 * int32_t terminal_read
 *   DESCRIPTION: reads a line that was entered into a given buffer
 *   INPUTS:
 *      -- in_buf - the buffer to read data into
 *      -- nbytes - the size of in_buf
 *   OUTPUTS: none
 *   RETURN VALUE: returns the number of bytes read into the buffer.
 *   SIDE EFFECTS: writes into the passed buffer
 */
int32_t terminal_read(int32_t fd, void* in_buf, int32_t nbytes) {
    uint32_t i;
    int32_t ret = 0;
    pcb_t * temp;
    curr_term->enter_flag = 0;
    char* char_in_buf = (char*) in_buf;

    // if stdout is calling it, call should fail
    if(fd == STDOUT_FD){
        return ERROR;
    }

    temp = scheduler.curr_process;

    sti();
    while (!(terminals[temp->tid].enter_flag));
    curr_term->enter_flag = 0;

    while(scheduler.curr_process->pid != temp->pid);
    cli();


    // scheduler.curr_process = temp;

    //add the buf into the in_buf
    for (i = 0; i < nbytes; i++) {
        //make sure the index is not out of any bounds
        if (nbytes >= ret && curr_term->return_buffer[i] != '\n') {
            char_in_buf[i] = curr_term->return_buffer[i];
            ret++;
        } else {
            if(curr_term->return_buffer[i] == '\n'){
                char_in_buf[i] = curr_term->return_buffer[i];
                ret++;
            }
            break;
        }
    }
    //return number of bytes read.
    return ret;
}

/*
 * void update_cursor
 *   DESCRIPTION: Updates the cursor
 *   INPUTS: (row, col) - desired row and col for cursor
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Moves cursor on the screen
 *   SOURCE: http://wiki.osdev.org/Text_Mode_Cursor
 */
void
update_cursor(int row, int col) {

    unsigned short position=(row*NUM_COLS) + col;

    // cursor LOW port to vga INDEX register

    outb(LOW_PORT, VGA_IND_REG);
    outb((unsigned char) (position & 0xFF), CURSOR_START);
    // cursor HIGH port to vga INDEX register
    outb(HIGH_PORT, VGA_IND_REG);
    outb((unsigned char) ((position >> 8) & 0xFF), CURSOR_START);
}

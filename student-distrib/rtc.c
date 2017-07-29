/* rtc.c - Functions to interact with the RTC
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "rtc.h"
#include "lib.h"
#include "sched.h"

/*
 * void rtc_init
 *   DESCRIPTION: Initializes RTC interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Turns on IRQ 8 (periodic interrupts of 2 Hz)
 *
 * Inspiration drawn from: http://wiki.osdev.org/RTC
 */
void
rtc_init(void)
{

    outb(STATUS_REG_B, RTC_ADDR_PORT);
    char prev = inb(RTC_DATA_PORT); // save value of this reg
    read_flag = 0;
    outb(STATUS_REG_B, RTC_ADDR_PORT);  // reading reset the index
    outb(prev | 0x40, RTC_DATA_PORT);   // sets bit 6 in reg B

    // currently the interrupt rate is set to default 1024 Hz, we need 2 Hz (detailed in set_interrupt rate)
    // we don't call set_interrupt_rate because cli() and sti() get messed up
    unsigned char rate = 0x0F;                      // frequency = 32768 >> (rate - 1)
    outb(STATUS_REG_A, RTC_ADDR_PORT);               // set index to register A, disable NMI
    prev = inb(RTC_DATA_PORT);                  // get initial value of register A
    outb(STATUS_REG_A, RTC_ADDR_PORT);               // reset index to A
    outb((prev & 0xF0) | rate, RTC_DATA_PORT);   // write only our rate to A. Note, rate is the bottom 4 bits.

    // enable associated interrupt on PIC
    enable_irq(RTC_IRQ);
}

/*
 * void set_interrupt_rate
 *   DESCRIPTION: Enables the specified IRQ
 *   INPUTS: rate - number in [6,15] to divide frequency by
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets interrupt freq to 32768/2^(rate - 1)
 *
 * Inspiration drawn from: http://wiki.osdev.org/RTC
 */
void
set_interrupt_rate(unsigned char rate)
{

    // check to see if frequency will be between 2 and 1024 Hz (6 <= rate <= 15)
    if(rate < 6 || rate > 15)
        return;

    cli();
    outb(STATUS_REG_A, RTC_ADDR_PORT);               // set index to register A, disable NMI
    char prev = inb(RTC_DATA_PORT);                  // get initial value of register A
    outb(STATUS_REG_A, RTC_ADDR_PORT);               // reset index to A
    outb((prev & 0xF0) | rate, RTC_DATA_PORT);   // write only our rate to A. Note, rate is the bottom 4 bits.
    sti();
}


/*
 * int32_t rtc_read
 *   DESCRIPTION: reads when there is an interrupt from rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE:
 *      -- 0 only after interrupt has occured
 *   SIDE EFFECTS: None
 *
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    read_flag = 0;  //initialize read flag
    sti();
    while (!read_flag); //wait for handler to change read flag
    cli();
    return 0;   //always return 0
}

/*
 * int32_t rtc_write
 *   DESCRIPTION: writes new value to interrupt rate.
 *   INPUTS: ir - the new interrupt rate.
 *   OUTPUTS: none
 *   RETURN VALUE:
 *      -- 0 on success
 *      -- ERROR if the frequency is out of bounds
 *   SIDE EFFECTS: Sets the interrupt rate of the RTC.
 *
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    int32_t temp, ir, rate = 1;   //variable to be passed into set_interrupt

    if(buf == NULL){
        return ERROR;
    }

    ir = *((int32_t*) buf);

    temp = MAX_SHORT / ir;    // used to take log
    if (ir < FREQ_LOW || ir > FREQ_HIGH)    //check if parametor is out of bounds
        return ERROR;
    //take the log base 2 of temp
    //we do this to correct for the offset in set_interrupt_rate
    while (temp != 1) {
        //reduce by a factor of 2
        if ((temp /= 2) == 0)
            return ERROR;
        else
            rate++;
    }
    set_interrupt_rate(rate); //checking on ir is done inside set_interrrupt_rate
    return 0;
}

/*
 * int32_t rtc_open
 *   DESCRIPTION: Open syscall for RTC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: Sets up RTC
 *
 */
int32_t rtc_open(const uint8_t* filename) {
    return 0;
}

/*
 * int32_t rtc_close
 *   DESCRIPTION: Close syscall for RTC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 *
 */
int32_t rtc_close(int32_t fd) {
    read_flag = 0;
    return 0;
}


/*
 * void rtc_int_handler
 *   DESCRIPTION: Handles RTC interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Signals that interrupt has been handled to RTC_IRQ
 *
 * Inspiration drawn from: http://wiki.osdev.org/RTC
 */
void
rtc_int_handler()
{
    cli();
    // make sure to read from reg C so that we can receive a new interrupt
    outb(STATUS_REG_C, RTC_ADDR_PORT);   // select register C
    inb(RTC_DATA_PORT);      // just throw away contents
    // test_interrupts();
    // Set the read_flag to enabled
    read_flag = 1;
    send_eoi(RTC_IRQ);  // signal PIC
    sti();
}




/*
 * void test_rtc_rw
 *   DESCRIPTION: tests the functionality of the rtc_read and rtc_write functions
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: writes to the screen repeatedly
 *
 */
void
test_rtc_rw() {
    int i, j, times;
    //reset the screen
    clear();
    set_screen_x(0);
    set_screen_y(0);
    //times is the number of 1s written to the screen
    times = 5;
    //loop through acceptable frequencies
    for (i = FREQ_LOW; i <= FREQ_HIGH; i*=2) {
        //change the interrupt rate
        rtc_write(-1, &i, -1);
        //write the characters
        for(j = 0; j < times; j++) {
            if (!rtc_read(-1, NULL, -1))
                putc_mod('1');
        }
        //reset the screen
        clear();
        set_screen_x(0);
        set_screen_y(0);
        //increment the times the char prints
        times *= 2;
    }
}

/* rtc.h - Defines used in interactions with the RTC
 * vim:ts=4 noexpandtab
 */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"

/* RTCS Ports */
#define RTC_ADDR_PORT	0x70
#define RTC_DATA_PORT 	0x71

#define STATUS_REG_A 	0x8A
#define STATUS_REG_B	0x8B
#define STATUS_REG_C	0x8C

#define RTC_IRQ			8

#define FREQ_LOW	2
#define FREQ_HIGH 	1024
#define MAX_SHORT	32768

/* Initialize the RTC */
extern void rtc_init(void);
/* Set a new interrupt rate for RTC */
extern void set_interrupt_rate(unsigned char rate);
/* Interrupt handler for RTC */
extern void rtc_int_handler();
/* System calls for RTC */
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
	
// set high whenever interrupt occurs
volatile int read_flag;

void test_rtc_rw();


#endif /* _RTC_H */

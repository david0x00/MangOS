#ifndef _PIT_H
#define _PIT_H

#define PIT_IRQ				0


/* PIT Ports */
#define CHANNEL_0_PORT	0x40
#define CHANNEL_1_PORT 	0x41
#define CHANNEL_2_PORT 	0x42
#define COMMAND_REG		0x43

#define RATE_GEN_MODE	0x34

#define HZ				50
#define CLOCK_TICK_RATE	1193182
#define LATCH			CLOCK_TICK_RATE/HZ

#define ERROR           -1

/* Initialize the RTC */
extern void pit_init(void);

extern void pit_int_handler();


#endif /* _PIT_H */

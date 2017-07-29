/* idt.h - Defines for IDT
 * vim:ts=4 noexpandtab
 */

#ifndef _IDT_H
#define _IDT_H

#include "types.h"
#include "lib.h"
#include "terminal.h"
#include "custom_images.h"

//blue screen of death with custom picture
#define DEFINE_EXCEPTION(handle_exception, message)	\
void handle_exception() {							\
	uint32_t temp;									\
	cli();											\
	clear();								\
	printf("%s\n", #message);						\
	asm ("movl %%cr2, %0" : "=r" (temp));			\
	printf("Linear Address at Fault: %x\n", &temp);	\
	while(1);										\
}

#define PIT_ENTRY		0x20
#define KEYBOARD_ENTRY	0x21
#define RTC_ENTRY		0x28
#define SYS_CALL_ENTRY	0x80

/* Initialize the IDT with each of the interrupt and exception handlers */
extern void idt_init();


#endif /* _IDT_H */

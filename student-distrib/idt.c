//contains IDT initialization functions.
#include "x86_desc.h"
#include "rtc.h"
#include "keyboard.h"
#include "idt.h"


// create exception handlers (prints error)
DEFINE_EXCEPTION(DE, "Divide-by-zero");
DEFINE_EXCEPTION(DB, "Debug");
DEFINE_EXCEPTION(NMI, "Non-maskable Interrupt");
DEFINE_EXCEPTION(BP, "Breakpoint");
DEFINE_EXCEPTION(OF, "Overflow");
DEFINE_EXCEPTION(BR, "Bound Range Exceeded");
DEFINE_EXCEPTION(UD, "Invalid Opcode");
DEFINE_EXCEPTION(NM, "Device Not Available");
DEFINE_EXCEPTION(DF, "Double Fault");
DEFINE_EXCEPTION(OLD_MF, "Coprocessor Segment Overrun");
DEFINE_EXCEPTION(TS, "Invalid TSS");
DEFINE_EXCEPTION(NP, "Segment Not Present");
DEFINE_EXCEPTION(SS, "Stack Segment Fault");
DEFINE_EXCEPTION(GP, "General Protection Fault");
DEFINE_EXCEPTION(PF, "Page Fault");
DEFINE_EXCEPTION(SPURIONS, "Spurious Interrupt");
DEFINE_EXCEPTION(MF, "x87 Floating-Point Exception");
DEFINE_EXCEPTION(AC, "Alignment Check");
DEFINE_EXCEPTION(MC, "Machine Check");
DEFINE_EXCEPTION(XF, "SIMD Floating-Point Exception");



/*
 * idt_init
 *   DESCRIPTION: Initializes IDT
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Installs the interrupt and exception handlers into the IDT
 */
void
idt_init() {

	int i;

	for (i = 0; i < NUM_VEC; i++) {

		// 0x80 is for the user to perform system calls (DPL = 3)
		// everything else should be kernel privilege (DPL = 0)
		if (i != SYS_CALL_ENTRY) {
			idt[i].dpl = 0;
		} else {
			idt[i].dpl = 3;
		}

		//enable the reserved3 for exception interrupts
		if (i < 32) {
			idt[i].reserved3 = 1;
		} else {
			idt[i].reserved3 = 0;
		}


		idt[i].size = 1;

		//unused: have to be 0
		idt[i].reserved0 = 0;
		//these are default values.
		idt[i].reserved1 = 1;
		idt[i].reserved2 = 1;
		idt[i].reserved4 = 0;

		// mark interrupt handler present
		idt[i].present = 1;
		idt[i].seg_selector = KERNEL_CS;

	}

	// install all interrupt handlers
	SET_IDT_ENTRY(idt[0], DE);
	SET_IDT_ENTRY(idt[1], DB);
	SET_IDT_ENTRY(idt[2], NMI);
	SET_IDT_ENTRY(idt[3], BP);
	SET_IDT_ENTRY(idt[4], OF);
	SET_IDT_ENTRY(idt[5], BR);
	SET_IDT_ENTRY(idt[6], UD);
	SET_IDT_ENTRY(idt[7], NM);
	SET_IDT_ENTRY(idt[8], DF);
	SET_IDT_ENTRY(idt[9], OLD_MF);
	SET_IDT_ENTRY(idt[10], TS);
	SET_IDT_ENTRY(idt[11], NP);
	SET_IDT_ENTRY(idt[12], SS);
	SET_IDT_ENTRY(idt[13], GP);
	SET_IDT_ENTRY(idt[14], PF);
	SET_IDT_ENTRY(idt[15], SPURIONS);
	SET_IDT_ENTRY(idt[16], MF);
	SET_IDT_ENTRY(idt[17], AC);
	SET_IDT_ENTRY(idt[18], MC);
	SET_IDT_ENTRY(idt[19], XF);

	//load the device handlers onto the IDT
	SET_IDT_ENTRY(idt[PIT_ENTRY], pit_interrupt);
	SET_IDT_ENTRY(idt[KEYBOARD_ENTRY], keyboard_interrupt);
	SET_IDT_ENTRY(idt[RTC_ENTRY], rtc_interrupt);
	//load the syscall handler onto the IDT
	SET_IDT_ENTRY(idt[SYS_CALL_ENTRY], syscall_interrupt);

	//Load the interrupt descripter table register.
	lidt(idt_desc_ptr);

}

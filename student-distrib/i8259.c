/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */



/*
 * i8259_init
 *   DESCRIPTION: Initializes PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Writes the proper ICWs to the PIC
 *
 * Inspiration drawn from Linux source code:
 * https://github.com/torvalds/linux/blob/master/drivers/irqchip/irq-i8259.c
 */
void
i8259_init(void)
{

	// write out the ICWS
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

	outb(ICW2_MASTER, MASTER_8259_IMR);
	outb(ICW2_SLAVE, SLAVE_8259_IMR);

	outb(ICW3_MASTER, MASTER_8259_IMR);
	outb(ICW3_SLAVE, SLAVE_8259_IMR);

	outb(ICW4, MASTER_8259_IMR);
	outb(ICW4, SLAVE_8259_IMR);

    outb(0xff, MASTER_8259_IMR);    /* mask all of 8259A-1 */
    outb(0xff, SLAVE_8259_IMR); /* mask all of 8259A-2 */

    enable_irq(SLAVE_IRQ);
}

/*
 * enable_irq
 *   DESCRIPTION: Enables the specified IRQ
 *   INPUTS: irq_num - IRQ to be enabled
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Writes a 0 to correct location in the IRQ mask on the proper chip
 *
 * Inspiration drawn from: http://wiki.osdev.org/8259_PIC
 */
void
enable_irq(uint32_t irq_num)
{
    uint16_t port;
    uint8_t value;

    // figure out if the IRQ is slave or master
    if(irq_num < PIC_LINES) {
        port = MASTER_8259_IMR;
    } else {
        port = SLAVE_8259_IMR;
        irq_num -= PIC_LINES;   // if slave, scale down IRQ to get to proper pin on slave
    }

    // enable that line on the specified PIC
    value = inb(port) & ~(1 << irq_num);
    outb(value, port);
}

/*
 * disable_irq
 *   DESCRIPTION: Disables the specified IRQ
 *   INPUTS: irq_num - IRQ to be disabled
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Writes a 1 to correct location in the IRQ mask on the proper chip
 *
 * Inspiration drawn from: http://wiki.osdev.org/8259_PIC
 */
void
disable_irq(uint32_t irq_num)
{
	uint16_t port;
    uint8_t value;

 	// figure out if the IRQ is slave or master
    if(irq_num < PIC_LINES) {
        port = MASTER_8259_IMR;
    } else {
        port = SLAVE_8259_IMR;
        irq_num -= PIC_LINES;   // if slave, scale down IRQ to get to proper pin on slave
    }

    // disable that line on the specified PIC
    value = inb(port) | (1 << irq_num);
    outb(value, port);
}

/*
 * send_eoi
 *   DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 *   INPUTS: irq_num - IRQ we need to send EOI to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: If IRQ < PIC_LINES, sends EOI to master at irq_num
 *   Otherwise, send EOI to the master for the IRQ where the slave is
 *   and send the EOI to the correct IRQ on the slave
 */
void
send_eoi(uint32_t irq_num)
{
    // slave IRQ, send EOI to both master and slave
	if(irq_num >= PIC_LINES && irq_num <= SLAVE_PIC) {
        irq_num -= PIC_LINES;   // if slave, scale down IRQ to get to proper pin on slave
		outb(EOI | irq_num, SLAVE_8259_PORT);
        outb(EOI | SLAVE_IRQ, MASTER_8259_PORT);
    }

    // master IRQ, only send EOI to master
    else{
        outb(EOI | irq_num, MASTER_8259_PORT);
    }

}

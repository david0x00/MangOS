
.globl rtc_interrupt, keyboard_interrupt, pit_interrupt, save_regs, restore_regs, syscall_interrupt

# rtc_interrupt()
# Description: Saves all registers in preparation for
# call of RTC interrupt handler, and then restores them
rtc_interrupt:
	pushal
	pushfl
	call rtc_int_handler
	popfl
	popal
	iret

# keyboard_interrupt()
# Description: Saves all registers in preparation for
# call of keyboard interrupt handler, and then restores them
# Side effects: Reads from keyboard port 0x60
keyboard_interrupt:
	pushal
	pushfl
	call keyboard_int_handler
	popfl
	popal
	iret

# keyboard_interrupt()
# Description: Saves all registers in preparation for
# call of keyboard interrupt handler, and then restores them
# Side effects: Reads from keyboard port 0x60
pit_interrupt:
	pushal
	pushfl
	call pit_int_handler
	popfl
	popal
	iret


# save_regs()
# Description: Saves all registers (make sure to call restore_regs after)
save_regs:
	pushal
	pushfl
	iret

# restore_regs()
# Description: Restores all registers (make sure restore_regs is called before)
restore_regs:
	popfl
	popal
	iret

# syscall_interrupt()
# description: sets up the stack for a system call, and then calls that function
# in c through the syscall_jump jump table
syscall_interrupt:
	#set up the stack
	pushfl
	# the push order is important. push %ebx last
	# since this is the 1st parameter that the user
	# identifies.  to understand this further, see
	# syscalls/ece391syscall.S
	pushl %edx #push the args
	pushl %ecx
	pushl %ebx
	pushl %eax
	movw $0x18, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	popl %eax
	#value in EAX should be in range from 1-10
	#check which sys call to execute based on number in EAX
	cmpl $0, %eax
	jbe syscall_error
	cmpl $10, %eax
	ja syscall_error
	#execute the correct system call
	#make eax start at 0 for jump table
	addl $-1, %eax
	call *syscall_jump(, %eax, 4)
	jmp syscall_return
	#return -1 on error.
	syscall_error:
		movl $-1, %eax
	syscall_return:
		#tear down the stack
		popl %ebx
		popl %ecx
		popl %edx
		popfl
		iret

#jump table for system calls
syscall_jump:
.long halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn


#ifndef _SYS_H
#define _SYS_H

#include "types.h"
#include "sched.h"
#include "paging_init.h"

#define EXEC_0 0x7F
#define EXEC_1 0x45
#define EXEC_2 0x4C
#define EXEC_3 0x46

#define EXEC_MAGIC_NO 4

#define ALIGNED_4MB  	0x00400000
#define ALIGNED_8MB		0x00800000
#define ALIGNED_12MB 	0x00C00000
#define ALIGNED_16MB	0x01000000
#define ALIGNED_128MB	0x08000000
#define ALIGNED_132MB	0x08400000
#define ALIGNED_136MB	0x08800000
#define ALIGNED_4B		0x4

#define ALIGNED_8KB  0x2000
#define VIDMAP_MASK     0xFFFFE000

#define EXEC_PG_DIR_FLAGS		 	(PAGE_SIZE_4MB | USER_SUPERVISOR | READ_WRITE | PRESENT)
#define FIRST_PROG_PG_DIR_ENTRY		(ALIGNED_8MB | EXEC_PG_DIR_FLAGS)
#define SECOND_PROG_PG_DIR_ENTRY 	(0x00C00000 | EXEC_PG_DIR_FLAGS)
#define EXEC_PG_DIR_OFFSET 			32
#define VIDMAP_PG_DIR_OFFSET 		33
#define EXEC_PG_OFFSET 				0x00048000
#define LOAD_ADDR 					(0x08000000 | EXEC_PG_OFFSET)

#define FIRST_PROG_PCB_ADDR         (ALIGNED_8MB - (1 * ALIGNED_8KB))
#define SECOND_PROG_PCB_ADDR        (ALIGNED_8MB - (2 * ALIGNED_8KB))

#define ERROR						-1

// extern uint32_t vidmap_page_table[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));

// ================== OFFICIAL SYSTEM CALLS ===============================
/* Terminates the process */
int32_t halt(uint8_t status);
/* Attempt to load and execute new program */
int32_t execute(const uint8_t* command);
/* Read data from keyboard, file, RTC, or directory */
int32_t read(int32_t fd, void* buf, int32_t nbytes);
/* Write data to terminal or RTC */
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
/* Provide access to file system */
int32_t open(const uint8_t* filename);
/* Closes the specified file descriptor */
int32_t close(int32_t fd);
/* Reads command line args into user-level buffer */
int32_t getargs(uint8_t* buf, int32_t nbytes);
/* Maps the text-mode video memory into userspace at screen_start */
int32_t vidmap(uint8_t** screen_start);
/*discussed in signal section*/
int32_t set_handler(int32_t signum, void* handler_address);
/*discussed in signal section*/
int32_t sigreturn(void);
// ================== OFFICIAL SYSTEM CALLS ===============================

int32_t validate_file(const uint8_t* filename, uint32_t* file_length, int32_t *inode);
int32_t get_file_name(const uint8_t* command, uint8_t* filename, uint32_t* filename_end);
int32_t get_entry_point(uint32_t inode);
int32_t add_args_to_buf(uint8_t* buf, const uint8_t* command, const uint32_t filename_end);
void flush_tlb();

extern uint32_t vidmap_term0[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
extern uint32_t vidmap_term1[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
extern uint32_t vidmap_term2[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
extern uint32_t* vidmap_page_table_array[NUM_TERMS];



#endif /* _SYS_H */

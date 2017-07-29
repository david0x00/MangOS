
#ifndef _PAGING_INIT_H
#define _PAGING_INIT_H

#include "types.h"
#include "terminal.h"

#define ALIGNED_4KB     0x1000
#define HIGH_20_MASK    0xFFFFF000
#define LOW_22_MASK     0x003FFFFF
#define PRESENT         0x1
#define READ_WRITE      0x2
#define USER_SUPERVISOR 0x4
#define PAGE_SIZE_4MB   0x80
#define PG_DIR_TAB_SIZE 1024

#define KERNEL_PG_DIR_ENTRY (0x00400000 | PAGE_SIZE_4MB | PRESENT)
#define VID_MEM             0xB8000
#define VID_TERM0           0xB9000
#define VID_TERM1           0xBA000
#define VID_TERM2           0xBB000

#define LOWER_12_BITS       12


void intialize_paging();

extern uint32_t page_directory[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
extern uint32_t page_table[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
extern uint32_t term_pte[NUM_TERMS];

#endif

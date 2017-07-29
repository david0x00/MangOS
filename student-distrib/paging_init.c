#include "paging_init.h"


uint32_t page_directory[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
uint32_t page_table[PG_DIR_TAB_SIZE] __attribute__((aligned (ALIGNED_4KB)));
uint32_t term_pte[NUM_TERMS] = {VID_TERM0, VID_TERM1, VID_TERM2};

/*
 * intialize_paging
 *   DESCRIPTION: Initializes Paging and creates page directory and page table
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Writes the proper entries in page directory and page table
 */
void intialize_paging() {
    int i;

    // clear all present bits in page dir and page table
    for(i = 0; i < PG_DIR_TAB_SIZE; ++i) {
        page_directory[i] = 0;
        page_table[i] = 0;
    }

    // set vid mem pg table and kernel entries in page directory
    page_directory[0] = ((unsigned int) page_table) | PRESENT;
    page_directory[1] = KERNEL_PG_DIR_ENTRY;

    // set vid mem pg in pg table
    page_table[VID_MEM   >> LOWER_12_BITS] = VID_MEM   | PRESENT;
    page_table[VID_TERM0 >> LOWER_12_BITS] = VID_TERM0 | PRESENT;
    page_table[VID_TERM1 >> LOWER_12_BITS] = VID_TERM1 | PRESENT;
    page_table[VID_TERM2 >> LOWER_12_BITS] = VID_TERM2 | PRESENT;


    // intialize paging
    asm volatile(
        // bitmask to enable paging and protection bits in cr0
        "CR0_PG_PE: .long 0x80000001;"

        // bitmask to enable page size extension in cr4
        "CR4_PSE: .long 0x10;"

        // load page directory into cr3
        "movl %0,    %%eax;"
        "movl %%eax, %%cr3;"

        // enable page size extension bit for 4MiB pages in cr4
        "movl %%cr4, %%eax;"
        "orl CR4_PSE, %%eax;"
        "movl %%eax, %%cr4;"

    	"xorl %%eax, %%eax;"

        // enable paging bit and protected mode in cr0
        "movl %%cr0, %%eax;"
        "orl  CR0_PG_PE, %%eax;"
        "movl %%eax, %%cr0;"
        :
        : "r" (page_directory)
        : "eax"
    );
}

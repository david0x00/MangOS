#ifndef _FILE_SYS_H
#define _FILE_SYS_H

#include "types.h"
#include "lib.h"

#define DENTRY_OFFSET	16
#define DENTRY_SIZE		64
#define INODE_SIZE		128
#define BLOCK_SIZE		4096
#define EOF				-1
#define MAX_DENTRIES	63
#define MAX_STRING_LEN	32
#define MAX_DBLOCKS		127
#define INODE_OFFSET	0x1000

#define RTC_FILE_TYPE	0
#define DIR_FILE_TYPE	1
#define REG_FILE_TYPE	2

#define ERROR			-1

typedef struct inode_t
{
	uint32_t length;
	uint32_t dblock_indices[MAX_DBLOCKS];
} __attribute__((aligned(BLOCK_SIZE))) inode_t;


typedef struct dentry_t
{
	int8_t file_name[MAX_STRING_LEN];
	uint32_t file_type;
	uint32_t inode_index;
	// uint32_t file_size;
} __attribute__((aligned(DENTRY_SIZE))) dentry_t;


typedef struct dblock_t
{
	uint8_t data[BLOCK_SIZE];
} __attribute__((aligned(BLOCK_SIZE))) dblock_t;


typedef struct boot_block_t
{
	uint32_t num_dentries;
	uint32_t num_inodes;
	uint32_t num_dblocks;
	dentry_t* dentries;
	inode_t* inodes;
	dblock_t* dblocks;
} boot_block_t;

void init_file_sys (unsigned int mod_start, unsigned int mod_end);
int32_t get_inode_from_name(const uint8_t* fname);
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t fname, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int32_t find_dentry_index (uint32_t inode);
void print_dentry (dentry_t* dentry);

extern boot_block_t boot_block;

// File functions
int32_t read_file(int32_t fd, void* buf, int32_t nbytes);
int32_t write_file(int32_t fd, const void* buf, int32_t nbytes);
int32_t open_file(const uint8_t* filename);
int32_t close_file(int32_t fd);


// Directory functions
int32_t read_directory(int32_t fd, void* buf, int32_t nbytes);
int32_t write_directory(int32_t fd, const void* buf, int32_t nbytes);
int32_t open_directory(const uint8_t* filename);
int32_t close_directory(int32_t fd);

extern dentry_t all_files[MAX_DENTRIES];

#endif

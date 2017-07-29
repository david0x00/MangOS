#include "file_sys.h"
#include "pcb.h"

// Global variable for the boot block
boot_block_t boot_block;
dentry_t all_files[MAX_DENTRIES];
int total_dentries = 0;
inode_t temp_inode;
dblock_t dblock;

/*
*	void init_file_sys (unsigned int mod_start, unsigned int mod_end)
*   Inputs: unsigned int mod_start = Address of where the boot block begins
*			unsigned int mod_end   = Address of where the file system ends
*   Return Value: NONE
*	Function: Initializes the file system
*/
void
init_file_sys (unsigned int mod_start, unsigned int mod_end)
{
	dentry_t d;
	int i, j = 0;

	uint32_t* boot_block_ptr = (uint32_t*)mod_start;
	boot_block.num_dentries  = (uint32_t)boot_block_ptr[0];
	boot_block.num_inodes    = (uint32_t)boot_block_ptr[1];
	boot_block.num_dblocks   = (uint32_t)boot_block_ptr[2];

	boot_block.dentries = (dentry_t*) &(boot_block_ptr[DENTRY_OFFSET]);
	boot_block.inodes   = (inode_t*)   ((uint32_t)boot_block_ptr + INODE_OFFSET);
	boot_block.dblocks  = (dblock_t*)  ((uint32_t)boot_block.inodes + (BLOCK_SIZE * boot_block.num_inodes));

	for(i = 0; i < MAX_DENTRIES; ++i) {
		d = boot_block.dentries[i];
		if(strlen(d.file_name) != 0) {
			all_files[j++] = d;
		}
	}
	total_dentries = j;
	clear();



/*
	// this is ls method for our file system bc there is only one directory
	// LISTS ALL FILES IN DIRECTORY
	// CTRL + 1
	for(i = 0; i < MAX_DENTRIES; ++i) {
		d = boot_block.dentries[i];
		if(strlen(d.file_name) != 0)
			print_dentry(&d);
	}
*/

/*
	// READS FILE BY NAME
	// CTRL + 2
	int offset;
	uint8_t c;
	dentry_t d;
	offset = 0;
	if(read_dentry_by_name((const uint8_t*)("ls"), &d) == 0) {
		while(offset < boot_block.inodes[d.inode_index].length) {
			read_data (d.inode_index, offset, &c, 1);
			putc_mod(c);
			offset++;
		}
	}
	putc_mod('\n');
	print_dentry(&d);
*/

/*
	// READ FILE BY INDEX
	// CTRL + 3
	// Example: i = 6 corresponds to grep
	int i;
	dentry_t d;
	i = 6;
	if(read_dentry_by_index(i, &d) == 0) {
		print_dentry(&d);
	}
*/


//	CTRL + 4 is RTC - not in this file!



}


/*
*	int32_t get_inode_from_name(const uint8_t* fname)
*   Inputs: const uint8_t* fname = The filename of the dentry
*   Return Value: inode pointer of dentry for success | ERROR for failure
*	Function: Reads a directory entry by name
*/
int32_t
get_inode_from_name(const uint8_t* fname)
{
	int i;
	uint32_t temp_len, fname_len;
	dentry_t temp;

	if (fname == NULL) return NULL;
	for (i = 0; i < MAX_DENTRIES; i++) {
		temp = boot_block.dentries[i];
		fname_len = strlen((const char*)fname);
		temp_len = strlen_mod(temp.file_name);
		if(temp_len == fname_len) {
			if (strncmp((int8_t*)temp.file_name, (int8_t*)fname, fname_len) == 0) {
				return temp.inode_index;
			}
		}
	}
	return ERROR;
}

/*
*	int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
*   Inputs: const uint8_t* fname = The filename of the dentry
*			dentry_t* dentry 	 = A pointer to a directory entry
*   Return Value: 0 for success | ERROR for failure
*	Function: Reads a directory entry by name
*/
int32_t
read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
	dentry_t temp;
	uint32_t i, temp_len, fname_len/*, max_len*/;

	if (dentry == NULL) return ERROR;
	if (fname == NULL) return ERROR;

	for (i = 0; i < MAX_DENTRIES; ++i)
	{
		// copy temp dentry into param if file names match, return success
		temp = boot_block.dentries[i];
		fname_len = strlen((const char*)fname);

		if(fname_len == 0)
			return ERROR;

		temp_len = strlen_mod(temp.file_name);
		// max_len = (temp_len > fname_len) ? temp_len : fname_len;
		// max_len = (max_len > MAX_STRING_LEN) ? MAX_STRING_LEN : max_len;

		if(temp_len == fname_len) {
			if (strncmp((int8_t*)temp.file_name, (int8_t*)fname, fname_len) == 0) {
				memcpy(dentry, &temp, DENTRY_SIZE);
				return 0;
			}
		}
	}

	// couldn't find dentry with fname, return failure
	// printf("Couldn't find dentry with name: %s\n", fname);
	return ERROR;
}


/*
*	int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry)
*   Inputs: uint32_t index 	 = The inode index
*			dentry_t* dentry = A pointer to a directory entry
*   Return Value: 0 for success | ERROR for failure
*	Function: Reads a directory entry by an inode
*/
int32_t
read_dentry_by_index (uint32_t index, dentry_t* dentry)
{
	int32_t i;

	// check for valid index, return failure
	if (index >= boot_block.num_inodes) return ERROR;
	if (dentry == NULL) return ERROR;

	i = find_dentry_index(index);
	if (i == ERROR) {
		// printf("Couldn't find dentry with inode index: %d\n", index);
		return ERROR;
	}

	memcpy(dentry, &(boot_block.dentries[i]), DENTRY_SIZE);
	return 0;
}


/*
*	int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
*   Inputs: uint32_t inode 	= The inode index
*			uint32_t offset = The offset from
*			uint8_t* buf 	= A pointer to a buffer
*			uint32_t length = Number of bytes to read into buffer
*   Return Value: 0 for success | ERROR for failure
*	Function: Reads data from the file system image
*/


int32_t
read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{

	uint32_t i, file_len, dblock_count, dblock_offset, data_offset, dblock_index, iterations, bytes_read = 0;

	// check for valid index, return failure

	if (inode < 0 || inode >= boot_block.num_inodes) return ERROR;
	if (buf == NULL) return ERROR;
	if (length == 0) return 0;


	// create inode to use for file and get file length
	temp_inode = boot_block.inodes[inode];
	file_len = temp_inode.length;

	// get number of data blocks used by file
	dblock_count = file_len / BLOCK_SIZE;
	if(file_len % BLOCK_SIZE != 0) dblock_count++;

	// calculate start point of data block and data offset
	dblock_offset = offset / BLOCK_SIZE;
	data_offset = offset % BLOCK_SIZE;

	// get data block
	dblock_index = temp_inode.dblock_indices[dblock_offset];
	dblock = boot_block.dblocks[dblock_index];

	// check and make sure we don't go overboard
	if (length >= file_len - offset)
		iterations = file_len - offset;
	else
		iterations = length;

	for(i = 0; i < iterations; i++) {
		// read data into buffer and increment indices
		if(data_offset >= file_len)
			// return bytes_read;
			break;
		// buf[i] = dblock.data[data_offset++];
		memcpy(&(buf[i]), &(dblock.data[data_offset++]), 1);
		bytes_read++;
		if(data_offset >= file_len)
			break;
		// reset pointers to start of next data block
		if(data_offset >= BLOCK_SIZE) {
			data_offset = 0;
			if(dblock_offset >= dblock_count)
				// return bytes_read;
				break;
			dblock = boot_block.dblocks[temp_inode.dblock_indices[++dblock_offset]];
		}
	}
	return bytes_read;
}


/*
*	int32_t find_dentry_index (uint32_t inode)
*   Inputs: uint32_t inode 	= The inode index
*   Return Value: i for the index of the dentry | ERROR for failure
*	Function: Finds the dentry based on the inode
*/
int32_t
find_dentry_index (uint32_t inode)
{
	dentry_t temp;
	int32_t i;

	for (i = 0; i < MAX_DENTRIES; ++i)
	{
		// copy temp dentry into param if file names match, return success
		temp = boot_block.dentries[i];
		// if(strlen(temp.file_name) != 0 && temp.inode_index == inode) {
			// print_dentry(&temp);
		// }
		if (temp.inode_index == inode)
			return i;
	}
	return ERROR;
}


/*
*	void print_dentry (dentry_t* dentry)
*   Inputs: dentry_t* dentry = A pointer to a directory entry
*   Return Value: NONE
*	Function: Prints the directory entry
*/
void
print_dentry (dentry_t* dentry)
{
	uint8_t copy_buf[MAX_STRING_LEN + 1];
	uint8_t print_buf[MAX_STRING_LEN + 1];
	uint32_t i, str_length;

	memcpy(copy_buf, dentry->file_name, MAX_STRING_LEN);
	memset(print_buf, ' ', MAX_STRING_LEN);

	str_length = strlen((const char*)copy_buf);

	print_buf[MAX_STRING_LEN] = '\0';

	for(i = 0; i < str_length; i++) {
		print_buf[MAX_STRING_LEN - i - 1] = copy_buf[str_length - i - 1];
	}

	// printf("file_name: %s | file_type: %d | file_size: %d", print_buf, dentry->file_type, boot_block.inodes[dentry->inode_index].length);
	putc_mod('\n');
}


/*
*	int32_t read_file(int32_t fd, void* buf, int32_t nbytes)
*   Inputs: int32_t fd = A file descriptor
*			void* buf  = A pointer to a buffer
*			int32_t nbytes = The number of bytes to copy into the buffer
*   Return Value: 0 for success | ERROR for failure
*	Function: Read data from file
*/
int32_t read_file(int32_t fd, void* buf, int32_t nbytes)
{
	file_desc_t file_desc = pcb_term[curr_term_idx]->file_desc_array[fd];
	// read data
	int32_t ret = read_data (file_desc.inode, file_desc.file_position, (uint8_t*) buf, nbytes);

	// if error, no bytes are read
	if(ret == ERROR) {
		return 0;
	}

	// update file position
	pcb_term[curr_term_idx]->file_desc_array[fd].file_position += ret;
	return ret;
}

/*
*	int32_t write_file(int32_t fd, const void* buf, int32_t nbytes)
*   Inputs: int32_t fd = A file descriptor
*			void* buf  = A pointer to a buffer
*			int32_t nbytes = no function
*   Return Value: ERROR for failure
*	Function: none
*/
int32_t write_file(int32_t fd, const void* buf, int32_t nbytes)
{
	return ERROR;
}

/*
*	int32_t open_file(const uint8_t* filename)
*   Inputs: const uint8_t* filename = A char pointer to a file name
*   Return Value: 0 for success | ERROR for failure
*	Function: Opens the passed in file
*/
int32_t open_file(const uint8_t* filename)
{
	return 0;
}

/*
*	int32_t close_file(int32_t fd)
*   Inputs: int32_t fd = A file descriptor
*   Return Value: 0 for success | ERROR for failure
*	Function: Closes the file
*/
int32_t close_file(int32_t fd)
{
	return 0;
}

/*
*	int32_t read_directory(int32_t fd, void* buf, int32_t nbytes)
*   Inputs: int32_t fd = A file descriptor
*			void* buf  = A pointer to a buffer
*			int32_t nbytes = The number of bytes to copy into the buffer
*   Return Value: 0 for success | ERROR for failure
*	Function: Read data from directory
*/
int32_t read_directory(int32_t fd, void* buf, int32_t nbytes)
{
	dentry_t d;
	// Gets the file index - used to index the global array of all the dentries
	file_desc_t file_desc = pcb_term[curr_term_idx]->file_desc_array[fd];
	int32_t file_index = file_desc.file_position;

	// check if end of file
	if(file_index >= total_dentries) {
		return 0;
	}

	// Gets the current dentry and copies the file name into the buffer
	d = all_files[file_index];
	strncpy((int8_t*)buf, d.file_name, MAX_STRING_LEN);

	// Increment the index
	pcb_term[curr_term_idx]->file_desc_array[fd].file_position++;
	return strlen_mod((const int8_t*)buf);

}

/*
*	int32_t write_directory(int32_t fd, void* buf, int32_t nbytes)
*   Inputs: int32_t fd = A file descriptor
*			void* buf  = A pointer to a buffer
*			int32_t nbytes = no function
*   Return Value: 0 for success | ERROR for failure
*	Function: none
*/
int32_t write_directory(int32_t fd, const void* buf, int32_t nbytes)
{
	return ERROR;
}

/*
*	int32_t open_file(const uint8_t* filename)
*   Inputs: const uint8_t* filename = A char pointer to a file name
*   Return Value: 0 for success | ERROR for failure
*	Function: Opens the passed in directory
*/
int32_t open_directory(const uint8_t* filename)
{
	return 0;
}

/*
*	int32_t close_directory(int32_t fd)
*   Inputs: int32_t fd = A file descriptor
*   Return Value: 0 for success | ERROR for failure
*	Function: Closes the directory
*/
int32_t close_directory(int32_t fd)
{
	return 0;
}

/* fat32_access.c
 *
 * Authors: Yinyi Luo, Guixing Lin, Jinghagn Wang
 * Based on the fat32
 *
 * Contains the lowest level methods for interacting with an fat322
 * filesystem image.
 *
 * Built for adding fat32 functionality to the Nautilus OS, Northwestern University
 */

#include "fat32fs.h"
#include "fat32_types.h"
#include "fat32fs.h"

#define FLOOR_DIV(x,y) ((x)/(y))
#define CEIL_DIV(x,y)  (((x)/(y)) + !!((x)%(y)))
#define DIVIDES(x,y) (((x)%(y))==0)
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define MIN(x,y) ((x)<(y) ? (x) : (y))

static char *rw[2] = { "read", "write" };

int read_write_bootrecord(struct fat32_state *fs, int write){
	write &= 0x1;

	DEBUG("size of boot record %d\n",sizeof(fs->bootrecord));
	if (sizeof(fs->bootrecord)!=BOOTRECORD_SIZE) { 
		ERROR("Cannot %s bootrecord as the code is not internally consistent\n",rw[write]);
	return -1;
    }

    if (!DIVIDES(BOOTRECORD_SIZE,fs->chars.block_size) || 
		BOOTRECORD_SIZE < fs->chars.block_size) { 
		ERROR("Cannot %s bootrecord as it is too small or not a multiple of device blocks\n",rw[write]);
		return -1;
    }

	int rc = 0;
	if (write) { 
		//rc = nk_block_dev_write(fs->dev,dev_offset,dev_num,&fs->super,NK_DEV_REQ_BLOCKING); 
		// TODO: write shadow copies
    } else {
		rc = nk_block_dev_read(fs->dev,0,1,&fs->bootrecord,NK_DEV_REQ_BLOCKING);
    }
    
    if (rc) { 
		ERROR("Failed to %s boot record due to device error\n",rw[write]);
		return -1;
    }
    return 0;

}
#define read_bootrecord(fs)  read_write_bootrecord(fs,0)
#define write_bootrecord(fs) read_write_bootrecord(fs,1)


int read_FAT(struct fat32_state *fs){
	int rc = 0;
	uint32_t cluster_size = fs->bootrecord.cluster_size;
	uint32_t FAT32_size = fs->bootrecord.FAT32_size;

	fs->table_chars.cluster_size = cluster_size;
	fs->table_chars.FAT32_size = FAT32_size;
	fs->table_chars.FAT32_begin = malloc(FAT32_size * fs->chars.block_size);

	rc = nk_block_dev_read(fs->dev, fs->bootrecord.reservedblock_size, FAT32_size, fs->table_chars.FAT32_begin, NK_DEV_REQ_BLOCKING);
	if(rc) {
		ERROR("Failed to read FAT from disk");
		return -1;
	}
	return 0;
}

static uint32_t get_sector_size(struct fat32_state *fs)
{
	return fs->bootrecord.sector_size;

}

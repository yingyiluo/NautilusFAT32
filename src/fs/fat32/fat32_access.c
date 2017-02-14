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
//#include <string.h>
//#include <stdlib.h>

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
	fs->table_chars.data_start = fs->bootrecord.FAT_num * FAT32_size + fs->bootrecord.reservedblock_size;
	fs->table_chars.data_end = fs->bootrecord.total_sector_num;

	rc = nk_block_dev_read(fs->dev, fs->bootrecord.reservedblock_size, FAT32_size, fs->table_chars.FAT32_begin, NK_DEV_REQ_BLOCKING);
	if(rc) {
		ERROR("Failed to read FAT from disk");
		free(fs->table_chars.FAT32_begin);
		return -1;
	}
	return 0;
}

static uint32_t get_sector_size(struct fat32_state *fs)
{
	return fs->bootrecord.sector_size;

}

static long get_sector_num(uint32_t cluster_num, struct fat32_state* fs)
{
	uint32_t num = fs->table_chars.data_start + cluster_num*fs->table_chars.cluster_size;
	if(num < fs->table_chars.data_end)
		return num;
	return -1;
}

static void filename_parser(char* path, char* name, char* ext)
{
	int i = 1; 
	int ext_i = 0;
	//needs to handle directory(use split_path function)
	//validate file name(eg: length)
	//further optimization: handle long file name; handle relative path 
	while(path[i] != '.')
	{
		name[i-1] = path[i];
		i++;
	}
	name[i] = '\0';
	i++;
	while(path[i] != '\0')
	{
		ext[ext_i] = path[i];
		i++;
		ext_i++;
	}
	ext[ext_i] = '\0';
}

void debug_print_file(struct fat32_state* state, uint32_t cluster_num, uint32_t size){
	char * file = (char*)malloc(512);
	nk_block_dev_read(state->dev, get_sector_num(cluster_num, state), 1, file, NK_DEV_REQ_BLOCKING);
	char output[size+1];
	strncpy(output, file, size);
	output[size] = 0;
	DEBUG("file contents: %s\n", output);
}

static uint32_t path_lookup( struct fat32_state* state, char* path )
{
	uint32_t root_sector = get_sector_num(state->bootrecord.rootdir_cluster, state);
	DEBUG("table entry[2] = %x\n", state->table_chars.FAT32_begin[2]);
	DEBUG("root sector num is %d\n", root_sector);
	DEBUG("directory entry size %d\n", sizeof(dir_entry));
	dir_entry* root_data = (dir_entry*)malloc(state->bootrecord.sector_size);
	nk_block_dev_read(state->dev, root_sector, 1, root_data, NK_DEV_REQ_BLOCKING);

	//print out the entire block
	for(int i = 0; i < 512; i+=4){
		DEBUG("%d th 32 bit is %x\n", i, *((int*)root_data+i));
	}
	//char found = '0';
/*	while(){
		//if not found and more cluster
		call helper
	}*/
	char file_name[9];
	char file_ext[4];

	filename_parser(path, file_name, file_ext);
	DEBUG("read file name is %s, ext is %s\n", file_name, file_ext);
	int i;
	for(i = 0; i < state->bootrecord.sector_size/sizeof(dir_entry); i++){
		dir_entry data = root_data[i];
		DEBUG("i is %d, dir_entry name is %s, ext is %s\n", i, data.name, data.ext);
		if( strncmp(data.name, file_name, 8) == 0 && strncmp(data.ext, file_ext, 3) == 0){
			DEBUG("enter strncmp if statement\n");
			break;
		}
	}
	DEBUG("directory entry name: %s \n",root_data[i].name);

	char cluster_str[5];  
	strncpy(cluster_str, root_data[i].high_cluster, 2);
	strncpy(cluster_str+2, root_data[i].low_cluster, 2);
	cluster_str[4] = '\0';
	int cluster_num = atoi(cluster_str);
	DEBUG("cluster number of file %d\n", cluster_num);
	debug_print_file(state, cluster_num, root_data[i].size);
	return cluster_num;
}


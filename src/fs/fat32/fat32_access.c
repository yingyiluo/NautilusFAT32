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
#define DECODE_CLUSTER(h,l) ((((uint32_t)h)<<16) | ((uint32_t)l))

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
	fs->table_chars.data_end = fs->bootrecord.total_sector_num - 1; 

	rc = nk_block_dev_read(fs->dev, fs->bootrecord.reservedblock_size, FAT32_size, fs->table_chars.FAT32_begin, NK_DEV_REQ_BLOCKING);
	if(rc) {
		ERROR("Failed to read FAT from disk");
		free(fs->table_chars.FAT32_begin);
		return -1;
	}
	return 0;
}

static uint32_t get_cluster_size(struct fat32_state *fs)
{
	return (uint32_t)fs->bootrecord.sector_size * (uint32_t)fs->bootrecord.cluster_size;

}

static uint32_t get_sector_num(uint32_t cluster_num, struct fat32_state* fs)
{
	uint32_t num = fs->table_chars.data_start + (cluster_num - fs->bootrecord.rootdir_cluster) * fs->table_chars.cluster_size;
/*	if(num < fs->table_chars.data_end)
		return num;
	return -1;*/
	return num;
}

/* split_path
 *
 * returns an array of each part of a filepath
 * i.e. split_path("/a/b/c") = {"a", "b", "c"}
 * also puts the number of parts into the specified location for convenience
 */
static char** split_path(char *path, int *num_parts) 
{
    int num_slashes = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
	num_slashes++;
    }
	
    *num_parts = num_slashes;
    // Copy out each piece by advancing two pointers (piece_start and slash).
    char **parts = (char **)malloc(num_slashes*sizeof(char *));
    char *piece_start = path + 1;
    int i = 0;
    for (char *slash = strchr(path + 1, '/'); slash != NULL; slash = strchr(slash + 1, '/')) {
		int part_len = slash - piece_start;
		parts[i] = (char *) malloc((part_len + 1)*sizeof(char));
		strncpy(parts[i], piece_start, part_len);
		piece_start = slash + 1;
		i++;
    }
    // Get the last piece.
    parts[i] = (char *)malloc((strlen(piece_start) + 1)*sizeof(char));
    strcpy(parts[i], " ");
    strcpy(parts[i], piece_start);
    DEBUG("num_parts is %d\n", *num_parts);
    for(int n = 0; n <= i; n++){
    	DEBUG("parts[%d] is %s\n", n, parts[n]);
    }
   
    return parts;
}

static void free_split_path(char **list, int n)
{
    int i;
    for (i=0;i<n;i++) { 
		free(list[i]);
    }
    free(list);
}

static void filename_parser(char* path, char* name, char* ext, int* name_s, int* ext_s)
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
		
	*name_s = i-1;
	if (*name_s < 8) {
		name[(*name_s)++] = ' '; // append a space if name < 8 chars
	}
	i++;
	while(path[i] != '\0')
	{
		ext[ext_i] = path[i];
		i++;
		ext_i++;
	}
	*ext_s = ext_i; 
	if (*ext_s < 3) {
		ext[(*ext_s)++] = ' '; // append a space if ext < 3 chars
	}
}

void debug_print_file(struct fat32_state* state, uint32_t cluster_num, uint32_t size){
	char * file = (char*)malloc(512);
	nk_block_dev_read(state->dev, get_sector_num(cluster_num, state), 1, file, NK_DEV_REQ_BLOCKING);
	char output[size+1];
	strncpy(output, file, size);
	output[size] = 0;
	DEBUG("file contents: %s\n", output);
}

static int isLowerCase(char c) {
	return (c >= 'a' && c <= 'z');
}

static char* toUpperCase(char* s) {
	for (int i = 0; s[i] != '\0'; ++i) {
		if (isLowerCase(s[i])) {
			s[i] += ('A' - 'a');
		}
	}
	return s;
}

static int path_lookup( struct fat32_state* state, char* path, uint32_t * dir_cluster_num )
{
	int num_parts;
	char** parts = split_path(path, &num_parts);
	uint32_t dir_entry_num = FLOOR_DIV(state->bootrecord.sector_size, sizeof(dir_entry));
	
	uint32_t root_sector = get_sector_num(state->bootrecord.rootdir_cluster, state);
	*dir_cluster_num = state->bootrecord.rootdir_cluster;  
	//DEBUG("table entry[2] = %x\n", state->table_chars.FAT32_begin[2]);
	DEBUG("root sector num is %d\n", root_sector);
	DEBUG("directory entry size %d\n", sizeof(dir_entry));
	dir_entry* root_data = (dir_entry*)malloc(state->bootrecord.sector_size);
	nk_block_dev_read(state->dev, root_sector, 1, root_data, NK_DEV_REQ_BLOCKING);

	/*	
	while(){
		//if not found and more cluster
		call helper
	}*/
	char file_name[8];
	char file_ext[3];
	uint32_t cluster_num;
	int i, name_size, ext_size;
	filename_parser(toUpperCase(path), file_name, file_ext, &name_size, &ext_size);
	DEBUG("read file name is %s, ext is %s\n", file_name, file_ext);
	for(i = 0; i < dir_entry_num; i++){
		dir_entry data = root_data[i];
		DEBUG("i is %d, dir_entry name is %s, ext is %s\n", i, data.name, data.ext);	
		if( strncmp(data.name, file_name, name_size) == 0 && strncmp(data.ext, file_ext, ext_size) == 0){
			cluster_num = DECODE_CLUSTER(data.high_cluster,data.low_cluster);
			DEBUG("cluster num is %d\n", cluster_num);	
			//debug_print_file(state, cluster_num, root_data[i].size);
			//debug_print_file(state, cluster_num, 11);
			return i;			
		}
	}
	return -1; //cannot find file
}

int alloc_block(struct fat32_state* state, uint32_t cluster_entry, uint32_t num) {
	uint32_t * fat = state->table_chars.FAT32_begin;
	uint32_t size = state->table_chars.FAT32_size; 
	uint32_t start = state->bootrecord.rootdir_cluster;
	uint32_t cluster_entry_cpy = cluster_entry;
	uint32_t count = 0;
	for(uint32_t i = start; i < size; i++) {
		uint32_t tmp = fat[i];
		if( (tmp << 1) == FREE_CLUSTER ){
			//update FAT table
			DEBUG("ALLOC BLOCK: i is %u\n", i);
			fat[cluster_entry] = i;
			count++;
			cluster_entry = i;
		}
		if(count == num) break;
	}

	fat[cluster_entry] = EOC_MIN;
	if(count < num){
		//restore fat 
		while(cluster_entry_cpy != EOC_MIN){
			uint32_t next = fat[cluster_entry_cpy];
			fat[cluster_entry_cpy] = FREE_CLUSTER;
			cluster_entry_cpy = next;
		}
		//nk_block_dev_read(state->dev, state->bootrecord.reservedblock_size, size, state->table_chars.FAT32_begin, NK_DEV_REQ_BLOCKING);
		return -1;
	}

	nk_block_dev_write(state->dev, state->bootrecord.reservedblock_size, size, fat, NK_DEV_REQ_BLOCKING);
	nk_block_dev_write(state->dev, state->bootrecord.reservedblock_size+size, size, fat, NK_DEV_REQ_BLOCKING);

	return 0; 
}
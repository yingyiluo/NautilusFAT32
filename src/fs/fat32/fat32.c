/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xtack.sandia.gov/hobbes
 *
 * Copyright (c) 2016, Brady Lee and David Williams
 * Copyright (c) 2016, Peter Dinda
 * Copyright (c) 2016, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors:  Brady Lee <BradyLee2016@u.northwestern.edu>
 *           David Williams <davidwilliams2016@u.northwestern.edu>
 *           Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/dev.h>
#include <nautilus/blkdev.h>
#include <nautilus/fs.h>

#include <fs/fat32/fat32.h>

#include "fat32_types.h"
#include "fat32_access.c"
#include "fat32fs.h"


typedef union attributes
{
   struct
   {
       char readonly : 1;
       char hidden : 1;
       char system : 1;
       char volumeid : 1;
       char dir : 1;
       char archive : 1; 
   };
   char attris;
}attributes;

typedef struct dir_entry
{
    char[8] name;
    char[3] ext;
    attributes attri;  //attributs 
    char[8] reserved0;
    char[2] high_cluster;
    char[4] reserved1;
    char[2] low_cluster;
    uint size;
}dir_entry; 

static ssize_t fat32_read_write(void *state, void *file, void *srcdest, off_t offset, size_t num_bytes, int write)
{
	return -1;
}

static ssize_t fat32_read(void *state, void *file, void *srcdest, off_t offset, size_t num_bytes)
{
    return fat32_read_write(state,file,srcdest,offset,num_bytes,0);
}

static ssize_t fat32_write(void *state, void *file, void *srcdest, off_t offset, size_t num_bytes)
{
    return fat32_read_write(state,file,srcdest,offset,num_bytes,1);
}

static int fat32_stat_path(void *state, char *path, struct nk_fs_stat *st)
{
    struct fat32_state *fs = (struct fat32_state *)state;
    //uint32_t inum = get_inode_num_by_path(fs,path);
/*
    if (!inum) {
        ERROR("Nonexistent path %s during stat\n",path);
        return -1;
    }
*/
   // return ext2_stat(state,(void*)(uint64_t)inum,st);
   return -1;
}

static void *fat32_create(void *state, char *path, int dir)
{
	return;
}

static void *fat32_create_file(void *state, char *path)
{
    return ;
}

static int fat32_create_dir(void *state, char *path)
{
    void *f = fat32_create(state,path,1);

    if (!f) {
        return -1;
    } else {
        return 0;
    }
}

static int fat32_exists(void *state, char *path)
{
    struct fat32_state *fs = (struct fat32_state *)state;

    DEBUG("exists(%s,%s)?\n",fs->fs->name,path);
   // uint32_t inode_num = get_inode_num_by_path(fs, path);
   // return inode_num != 0;
    return -1;
}

int fat32_remove(void *state, char *path)
{
    return -1;
}

static void * fat32_open(void *state, char *path)
{
    return;
}

static int fat32_stat(void *state, void *file, struct nk_fs_stat *st)
{
    struct fat32_state *fs = (struct fat32_state *)state;
    return 0;
}

static int fat32_truncate(void *state, void *file, off_t len)
{
    return -1;
}
static void fat32_close(void *state, void *file)
{
    struct fat32_state *fs = (struct fat32_state *)state;

    DEBUG("closing inode %u\n",(uint32_t)(uint64_t)file);

    // ideally FS would track this here so that we can handle multiple
    // opens, locking, etc correctly, but that's outside of scope for now

}


static struct nk_fs_int fat32_inter = {
    .stat_path = fat32_stat_path,
    .create_file = fat32_create_file,
    .create_dir = fat32_create_dir,
    .exists = fat32_exists,
    .remove = fat32_remove,
    .open_file = fat32_open,
    .stat = fat32_stat,
    .trunc_file = fat32_truncate,
    .close_file = fat32_close,
    .read_file = fat32_read,
    .write_file = fat32_write,
};

int nk_fs_fat32_attach(char *devname, char *fsname, int readonly){
	struct nk_block_dev *dev = nk_block_dev_find(devname);
	uint64_t flags = readonly ? NK_FS_READONLY : 0;

	if (!dev) { 
		ERROR("Cannot find device %s\n",devname);
		return -1;
    }

    struct fat32_state *s = malloc(sizeof(*s));
    if (!s) { 
		ERROR("Cannot allocate space for fs %s\n", fsname);
		return -1;
    }

    memset(s,0,sizeof(*s));

    s->dev = dev;
    
    if (nk_block_dev_get_characteristics(dev,&s->chars)) { 
		ERROR("Cannot get characteristics of device %s\n", devname);
		free(s);
		return -1;
    }

    DEBUG("Device %s has block size %lu and numblocks %lu\n",dev->dev.name, s->chars.block_size, s->chars.num_blocks);
    
    //read reserved block(boot record)
    if (read_bootrecord(s)) {
		ERROR("Cannot read bootrecord for fs FAT32 %s on device %s\n", fsname, devname);
		free(s);
		return -1;
    }

    if (read_FAT(s)){
        ERROR("Cannot load FAT into memory");
        free(s);
        return -1;
    }

    /*
    //DEBUG("System ID \"%s\"\n", s->bootrecord.system_id);
	DEBUG("Media byte %x\n", s->bootrecord.media_type);
	DEBUG("%lu bytes per logical sector\n",s->bootrecord.sector_size);
	DEBUG("%lu bytes per cluster\n",s->bootrecord.cluster_size * BLOCK_SIZE);
	DEBUG("%lu reserved sectors\n",s->bootrecord.reservedblock_size);
	DEBUG("First FAT starts at sector %lu\n",s->bootrecord.reservedblock_size);
	DEBUG("%lu FATs\n",s->bootrecord.FAT_num);
	DEBUG("%lu sectors per FAT\n",s->bootrecord.FAT32_size);
	DEBUG("Root directory start at cluster %lu (arbitrary size)\n",s->bootrecord.rootdir_cluster);
	//DEBUG("Data area starts at byte 1049600 (sector 2050)\n",);
	//DEBUG("129022 data clusters (66059264 bytes)\n",);
	DEBUG("%lu sectors/track, %lu heads\n",s->bootrecord.track_size, s->bootrecord.head_num);
	DEBUG("%lu hidden sectors\n",s->bootrecord.hidden_sector_num);
	DEBUG("%lu sectors total\n",s->bootrecord.total_sector_num);

    */
    
    s->fs = nk_fs_register(fsname, flags, &fat32_inter, s);

    if (!s->fs) { 
	ERROR("Unable to register filesystem %s\n", fsname);
	free(s);
	return -1;
    }

    INFO("filesystem %s on device %s is attached (%s)\n", fsname, devname, readonly ?  "readonly" : "read/write");
    
	return 0;
}

int nk_fs_ext2_detach(char *fsname)
{
    struct nk_fs *fs = nk_fs_find(fsname);
    if (!fs) {
        return -1;
    } else {
        return nk_fs_unregister(fs);
    }
}

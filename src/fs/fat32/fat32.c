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

#include "fat32_access.c"
#include "fat32fs.h"
#include "fat32_types.h"

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


    /*
    // stash away superblock for later use
    // any modifier is responsible for writing it as well
    if (read_superblock(s)) {
	ERROR("Cannot read superblock for fs %s on device %s\n", fsname, devname);
	free(s);
	return -1;
    }
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

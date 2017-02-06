#ifndef __FAT32_TYPES_H__
#define __FAT32_TYPES_H__



#define INFO(fmt, args...)  INFO_PRINT("fat32: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("fat32: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("fat32: " fmt, ##args)

#ifndef NAUT_CONFIG_DEBUG_FAT32_FILESYSTEM_DRIVER
#undef DEBUG
#define DEBUG(fmt, args...)
#endif


#define BLOCK_SIZE 512

struct fat32_state {
    struct nk_block_dev_characteristics chars; 
    struct nk_block_dev *dev;
    struct nk_fs        *fs;
    struct fat32_bootrecord bootrecord;
};


#endif
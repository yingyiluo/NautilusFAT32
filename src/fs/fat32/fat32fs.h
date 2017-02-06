#ifndef __FAT32FS_H__
#define __FAT32FS_H__


#define BOOTRECORD_SIZE		512

struct fat32_bootrecord{
	//BIOS Parament Block
	uint8_t na[3];
	uint8_t OEM_identifier[8];
	uint16_t sector_size;			//number of bytes per sector
	uint8_t cluster_size; 			//number of sectors per cluster
	uint16_t reservedblock_size;	//number of reserved sectors, boot record is included
	uint8_t FAT_num;				//number of FATs
	uint16_t directory_entry_num;	//number of directory entries
	uint16_t logical_volume_size;	//total number of sectors in the logical volume
	uint8_t media_type;
	uint16_t FAT16_size;		//number of sectors per FAT12/16
	uint16_t track_size;			//number of sectors per track
	uint16_t head_num;			//number of heads/sides on the media
	uint32_t hidden_sector_num;	//number of hidden sectors;
	uint32_t total_sector_num;		//large number of sectors on media
	//end of BPB

	//Extended Boot Record
	uint32_t FAT32_size;			//number of sectors per FAT
	uint16_t flags;				//flags
	uint16_t version;				//FAT version number
	uint32_t rootdir_cluster;		//root directory cluster number, usually 2
	uint16_t FSInfo;				//sector number of FSInof Structure
	uint16_t backip_boot;			//sector number of backup boot sector
	uint8_t reserved[12];			//should be 0
	uint8_t drive_number;
	uint8_t flag_Windowns_NT;		//reserved
	uint8_t signature;				//must be 0x28 or 0x29
	uint32_t serial_num;			//Volumen serial number 
	char	volument_lbl_string[11];//padded with spaces	
	char	system_id[8];			//system identifier
	uint8_t boot_code[420];
	uint16_t partition_signature;	//bootable partition signature 0xAA55
	//end of Entended Boot Record
}__attribute__((packed));



#endif
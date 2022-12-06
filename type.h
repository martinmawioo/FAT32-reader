#ifndef _TYPE_H_
#define _TYPE_H_
#include <stdint.h>


// Ref: https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
/*struct FAT32
{
    uint8_t bootstrapJump[3];		// 0-2		Code to jump to bootstrap
    uint8_t oem[8];					// 3-10		OEM name/version
	uint8_t bytePerSector[2];		// 11-12	Byte per sector
    uint8_t sectorPerCluster;		// 13		Sector per cluster
    uint8_t reservedSector[2];		// 14-15	Number of reserved sectors
	uint8_t fatCopy;				// 16		Number of FAT copies
	uint8_t rdetEntry[2];			// 17-18	Number of root directory entries
	uint8_t sectorInFS[2];			// 19-20	Total number of sector in the filesystem
	uint8_t mediaType;				// 21		Media descriptor type
	uint8_t sectorPerFAT[2];		// 22-23	Sector per FAT, 0 for FAT32
	uint8_t sectorPerTrack[2];		// 24-25	Sector per track
    uint8_t head[2];				// 26-27	Number of heads
    uint8_t hiddenSector[2];        // 28-29	Number of hidden sectors
    uint8_t bootstrap[480];			// 30-509	Bootstrap code
    uint8_t signature[2];			// 510-511	Signature 0x55 0xaa
};*/

// Ref: https://en.wikipedia.org/wiki/GUID_Partition_Table
struct GPT
{
    uint8_t signature[8];				// Offset 0 (0x00)		Length 8 bytes	Signature ("EFI PART", 45h 46h 49h 20h 50h 41h 52h 54h or 0x5452415020494645ULL[a] on little-endian machines)
    uint8_t rev[4];						// Offset 8 (0x08)		Length 4 bytes	Revision (for GPT version 1.0 (through at least UEFI version 2.3.1), the value is 00h 00h 01h 00h)
	uint8_t header[4];					// Offset 12 (0x0C) 	Length 4 bytes	Header size in little endian (in bytes, usually 5Ch 00h 00h 00h or 92 bytes)
    uint8_t headerCRC32[4];				// Offset 16 (0x10)		Length 4 bytes	CRC32 of header (offset +0 up to header size), with this field zeroed during calculation
    uint8_t reserved1[4];				// Offset 20 (0x14)		Length 4 bytes	Reserved; must be zero
	uint8_t currentLBA[8];				// Offset 24 (0x18)		Length 8 bytes	Current LBA (location of this header copy)
	uint8_t backupLBA[8];				// Offset 32 (0x20)		Length 8 bytes	Backup LBA (location of the other header copy)
	uint8_t firstLBA[8];				// Offset 40 (0x28)		Length 8 bytes	First usable LBA for partitions (primary partition table last LBA + 1)
	uint8_t lastLBA[8];					// Offset 48 (0x30)		Length 8 bytes	Last usable LBA (secondary partition table first LBA - 1)
	uint32_t GUID[4];					// Offset 56 (0x38)		Length 16 bytes	Disk GUID (also referred as UUID on UNIXes)
	uint8_t partitionEntriesArray[8];	// Offset 72 (0x48)		Length 8 bytes	Starting LBA of array of partition entries (always 2 in primary copy)
	uint8_t countPartitionEntries[4];	// Offset 80 (0x50)		Length 4 bytes	Number of partition entries in array
    uint8_t sizePartitionEntries[4];	// Offset 84 (0x54)		Length 4 bytes	Size of a single partition entry (usually 80h or 128)
    uint8_t partitionArrayCRC32[4];		// Offset 88 (0x58)		Length 4 bytes	CRC32 of partition array
    uint8_t reserved2[420];				// Offset 92 (0x5C)		Length *		Reserved; must be zeroes for the rest of the block (420 bytes for a sector size of 512 bytes; but can be more with larger sector sizes)
};

struct PartitionEntryGPT
{
    uint8_t type[16];		// Partition type GUID
    uint8_t GUID[16];		// Unique partition GUID
    uint8_t firstLBA[8];	// First LBA (little endian)
    uint8_t lastLBA[8];	// Last LBA (inclusive, usually odd)
    uint8_t attribute[8];	// Attribute flags (e.g. bit 60 denotes read-only)
    uint8_t name[72];		// Partition name (36 UTF-16LE code units)
};




// Ref: https://en.wikipedia.org/wiki/Master_boot_record
struct MBR
{
    uint8_t bootstrap[446];			// Offset 0: 	Bootstrap code area
    uint8_t partitionEntry1[16];	// Offset 446:	(Primary) partition entry no 1
    uint8_t partitionEntry2[16];	// Offset 462:	(Primary) partition entry no 2
    uint8_t partitionEntry3[16];	// Offset 478:	(Primary) partition entry no 3
    uint8_t partitionEntry4[16];	// Offset 494:	(Primary) partition entry no 4
    uint8_t bootSingnature[2];		// Offset 510:	Boot singature 0x55 0xAA
};

struct PartitionEntryMBR
{
    uint8_t pysicalDrive;			// Status of physical drive (bit 7 set is for active or bootable, old MBRs only accept 80hex, 00hex means inactive, and 01hex–7Fhex stand for invalid)
    uint8_t firstAddressCHS[3];		// CHS address of first absolute sector in partition. The format is described by three bytes, see the next three rows.
    uint8_t partitionType;			// Partition type: https://en.wikipedia.org/wiki/Partition_type
    uint8_t lastAddressCHS[3];		// CHS address of last absolute sector in partition.[d] The format is described by 3 bytes, see the next 3 rows.
    uint8_t LBA[4];					// LBA of first absolute sector in the partition
    uint8_t countSector[4];			// Number of sectors in partition
};


#endif // _TYPE_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>

#include "type.h"

#define DEFAULT_BACKUP  "disk.dat"


int bit(uint64_t var,int pos);
uint64_t reverse64(uint64_t num);
void writeByte(uint8_t* byte, int count);

void show(char* disk);
void backup(char* arg);
void explore(char* arg);
void change(char* arg);
void extractInfo(char* arg, char* disk, unsigned long* first, unsigned long* last);
uint64_t reverseByte(uint8_t* byte, unsigned int count);

int main(int argc, char* argv[])
{
	char c;
    while ((c = getopt (argc, argv, "c:i:b:e:?:")) != -1)
        switch (c)
        {
			case 'i':
				show(optarg);
				break;
			case 'b':
				backup(optarg);
				break;
			case 'e':
				explore(optarg);
				break;
			case 'c':
				change(optarg);
				break;
			case '?':
				printf("Showing help\n");
				break;
			default:
				exit(1);
        }

    return 0;
}

void show(char* disk)
{
    printf("Infomation about disk %s\n", disk);

    uint8_t bootSector[1024];
    FILE *f;

    f = fopen(disk, "rb");
    if (f == NULL)
	{
		printf("Could not open device.\nCheck for your root permission to access block devices on Linux system.\n");
		return;
	}
    fread(&bootSector, sizeof(bootSector), 1, f);
    fclose(f);

    /* First, check for GPT partition table
    * Skip the first 512 bytes because GPT may use Protective MBR
    *
	* For limited backward compatibility, the space of the legacy MBR is still reserved
	* in the GPT specification, but it is now used in a way that prevents MBR-based disk
	* utilities from misrecognizing and possibly overwriting GPT disks. This is referred
	* to as a protective MBR.
    */

	if (memcmp(bootSector+512, "EFI PART", 8) == 0)
	{
        printf("Partition table: [GPT]\n");
        struct GPT *gpt = (struct GPT *)(bootSector+512);
        uint32_t headerSize = reverseByte(gpt->header, 4);
        uint64_t posPartitionEntriesArray = reverseByte(gpt->partitionEntriesArray, 8) * 512;
        uint32_t sizePartitionEntry = reverseByte(gpt->sizePartitionEntries, 4);

		printf("Signature:                 "); fwrite(gpt->signature, 8, 1, stdout); printf("\n");
		printf("Header size:               %d bytes\n", headerSize);
		printf("GUID:                      %08llx%08llx%08llx%08llx\n", (unsigned long long int)gpt->GUID[3], (unsigned long long int)gpt->GUID[2], (unsigned long long int)gpt->GUID[1], (unsigned long long int)gpt->GUID[0]);
		printf("Partition Entry Array at:  0x%08llx\n", (unsigned long long int)posPartitionEntriesArray);
		printf("Size of a partition entry: %ld bytes\n", (unsigned long)sizePartitionEntry);

        if (sizePartitionEntry != sizeof(struct PartitionEntryGPT))
		{
            printf("Caution: size of partition entry bigger than %ld is not supported by this program\n", (unsigned long)sizeof(struct PartitionEntryGPT));
		}

		f = fopen(disk, "rb");
		// Jump to the first partition entry
		fseek(f, posPartitionEntriesArray, SEEK_SET);
		struct PartitionEntryGPT entry;

        printf("\nList of partition entry:\n");
        int count = 0;
		while (count++ <= 127)
		{
			fseek(f, 0, SEEK_CUR+128);
            fread(&entry, sizePartitionEntry, 1, f);

            if ((*(uint64_t*)entry.type) == 0)
                continue;

            uint64_t firstSector = reverseByte(entry.firstLBA, 8);
            uint64_t lastSector = reverseByte(entry.lastLBA, 8);

            printf("------------------\nEntry %d\n", count);
			printf("Partition type:  %08llX%08llX\n", (unsigned long long int)reverseByte(entry.type+8, 8), (unsigned long long int)reverseByte(entry.type, 8));
			printf("GUID:            %08llX%08llX\n", (unsigned long long int)reverseByte(entry.GUID+8, 8), (unsigned long long int)reverseByte(entry.GUID, 8));
            printf("First sector:    %lld\n", (unsigned long long int)firstSector);
            printf("Last sector:     %lld\n", (unsigned long long int)lastSector);
            printf("Size:            %lldMB\n", (unsigned long long int)(lastSector - firstSector + 1)*512/1024/1024);
            printf("Label:           "); fwrite(entry.name, 72, 1, stdout); printf("\n");

            printf("Format:          ");

            uint8_t partitionBootSector[512];
            FILE* f_partition = fopen(disk, "rb");
            if (f_partition == NULL)
                exit(1);
            fseek(f_partition, firstSector*512, SEEK_SET);
            fread(partitionBootSector, 512, 1, f_partition);
			fclose(f_partition);

			if (strstr(partitionBootSector+3, "NTFS    "))
			{
                printf("NTFS\n");
			} else if (strstr(partitionBootSector+82, "FAT32   "))
			{
                printf("FAT32\n");
			} else if (strstr(partitionBootSector+54, "FAT12   "))
			{
                printf("FAT12\n");
			} else if (strstr(partitionBootSector+54, "FAT16   "))
			{
                printf("FAT16\n");
			} else if (strstr(partitionBootSector+54, "FAT     "))
			{
                printf("FAT\n");
			} else printf("Unknown\n");

            printf("Partition attributes: ");
            uint64_t attribute = *((uint64_t*)entry.attribute);
            if (bit(attribute, 0))
                printf("platform required, ");
            if (bit(attribute, 1))
                printf("EFI firmware should ignore this partition, ");
            if (bit(attribute, 2))
                printf("Legacy BIOS bootable, ");
			if (bit(attribute, 7))
                printf("Legacy BIOS bootable, ");
            if (bit(attribute, 60))
                printf("read only, ");
            if (bit(attribute, 61))
                printf("shadow copy, ");
            if (bit(attribute, 62))
                printf("hidden, ");
            if (bit(attribute, 63))
                printf("no drive letter, ");
			printf("\n");
		}

        fclose(f);

	} else if (memcmp(bootSector+510, "\x55\xaa", 2) == 0)		// Check for MBR
	{
        printf("Partition table: [MBR]\n");

        struct MBR *mbr = (struct MBR *)(bootSector);

        int count = 0;
        struct PartitionEntryMBR *entry;
        while (count++ < 4)
		{
			entry = (struct PartitionEntryMBR *)(mbr->partitionEntry1 + (count-1)*16);
			if (entry->partitionType == 0)
				continue;

            uint64_t firstSector = reverseByte(entry->LBA, 4);
            uint64_t countSector = reverseByte(entry->countSector, 4);
            uint64_t lastSector =  firstSector + countSector - 1;

            printf("---------------\nPartition entry %d\n", count);
            printf("First sector:      %ld\n", firstSector);
            printf("Last sector:       %ld\n", lastSector);
            printf("Number of sector:  %ld\n", countSector);
            printf("Size:              %ld MB\n", countSector*512/1024/1024);
            printf("Partition type:    0x%02x - ", entry->partitionType);

            // https://en.wikipedia.org/wiki/Partition_type
            switch (entry->partitionType)
            {
				case 0x0b:
                    printf("FAT32\n");
                    break;
				case 0x05:
					// https://en.wikipedia.org/wiki/Extended_boot_record
                    printf("Extended partiton of MBR\n");

					uint8_t extendedBootSector[512];
					FILE* f_partition = fopen(disk, "rb");
					if (f_partition == NULL)
						exit(1);
					fseek(f_partition, firstSector*512, SEEK_SET);
					fread(extendedBootSector, 512, 1, f_partition);
					fclose(f_partition);

					struct MBR *extendMBR = (struct MBR *)(extendedBootSector);
					int count1 = 0;
					struct PartitionEntryMBR *entry1;
					while (count1++ < 4)
					{
						entry1 = (struct PartitionEntryMBR *)(extendMBR->partitionEntry1 + (count1-1)*16);
						if (entry1->partitionType == 0)
							continue;

						firstSector = reverseByte(entry1->LBA, 4);
						countSector = reverseByte(entry1->countSector, 4);
						lastSector =  firstSector + countSector - 1;

						printf("---------------\nPartition entry %d of Extended partition\n", count1);
						printf("First sector:      %ld\n", firstSector);
						printf("Last sector:       %ld\n", lastSector);
						printf("Number of sector:  %ld\n", countSector);
						printf("Size:              %ld MB\n", countSector*512/1024/1024);
						printf("Partition type:    0x%02x - ", entry1->partitionType);

						switch (entry1->partitionType)
						{
							case 0x0b:
								printf("FAT32\n");
								break;
							case 0x0c:
								printf("FAT32\n");
								break;
							case 0x01:
								printf("FAT12\n");
								break;
							case 0x04:
								printf("FAT16\n");
								break;
							case 0x06:
								printf("FAT16B\n");
								break;
							case 0x0e:
								printf("FAT16B\n");
								break;
							case 0x07:
								printf("IFS/HPFS/NTFS/exFAT\n");
								break;
							case 0x27:
								printf("FAT32/NTFS recuse partition\n");
								break;
							default:
								printf("Unknown\n");
						}
					}
                    break;
				case 0x0c:
                    printf("FAT32\n");
                    break;
				case 0x01:
                    printf("FAT12\n");
                    break;
				case 0x04:
                    printf("FAT16\n");
                    break;
				case 0x06:
                    printf("FAT16B\n");
                    break;
				case 0x0e:
                    printf("FAT16B\n");
                    break;
				case 0x07:
                    printf("IFS/HPFS/NTFS/exFAT\n");
                    break;
				case 0x27:
                    printf("FAT32/NTFS recuse partition\n");
                    break;
				default:
					printf("Unknown\n");
            }
		}

	} else
        printf("Partition table: Unknown\n");

}

void backup(char* arg)
{
	char* disk = arg;
	disk[strstr(arg, ":") - disk] = '\0';
	char* backupDisk = disk + strlen(disk) + 1;

	printf("Backing up disk %s into %s\n", disk, backupDisk);
	char command[200];
    sprintf(command, "sudo dd if=%s of=%s bs=512", disk, backupDisk);
	FILE* f = popen(command, "r");

	while (fgets(command, 200, f) > 0)
        printf("%s", command);

	fclose(f);
}

void extractInfo(char* arg, char* disk, unsigned long* first, unsigned long* last)
{
	disk = arg;
	disk[strstr(disk, ":") - disk] = '\0';

    char* cfirst = disk + strlen(disk) + 1;
    cfirst[strstr(cfirst, ":") - cfirst] = '\0';

    char* clast = cfirst + strlen(cfirst) + 1;

    *first = strtol(cfirst, NULL, 16);
    *last = strtol(clast, NULL, 16);
}

void explore(char* arg)
{
    char* disk = arg;
    unsigned long first,last;

    extractInfo(arg, disk, &first, &last);

    printf("Exploring disk %s from byte 0x%04lx to 0x%04lx\n", disk, first, last);

    int size = last-first+1;
    uint8_t* byte = (uint8_t*)malloc(size);

    FILE* f = fopen(disk, "rb");
    if (f == NULL)
	{
        printf("Could not open %s\n", disk);
	} else
	{
        fread(byte, size, 1, f);
        fclose(f);

        int index = 0;
        int row = size / 16;
		int remain = size - row * 16;
		printf("---------------------------\n");
        for (int i = 0; i < row; i++)
		{
			printf("0x%04lx: ", first);
			first += 16;

			// colum 1
			for (int i = 0; i < 8; i++)
				printf("%02x ", byte[index++]);
            printf("  ");

            // colum 2
			for (int i = 0; i < 8; i++)
				printf("%02x ", byte[index++]);
			printf("\n");
		}

		if ((last - first + 1) > 0)
		{
			printf("0x%04lx: ", first);

			if ((last - first + 1) > 8)
			{
				// colum 1
				for (int i = 0; i < 8; i++)
					printf("%02x ", byte[index++]);
				printf("  ");
			}

            // colum 2
			for (int i = 0; i < (last-first+1)/8; i++)
				printf("%02x ", byte[index++]);
			printf("\n");
		}
	}
}

void change(char* arg)
{
    char* disk = arg;
    unsigned long pos,val;

    extractInfo(arg, disk, &pos, &val);

    printf("Changing byte at 0x%04lx to 0x%04lx from disk %s\n", pos, val, disk);
}


uint64_t reverseByte(uint8_t* byte, unsigned int count)
{
    uint64_t result = 0;
    for (int i = count - 1; i >= 0; i--)
		result = (result << 8) | byte[i];

    return result;
}

uint64_t reverse64(uint64_t num)
{
    uint64_t count = sizeof(num) * 8 - 1;
    uint64_t reverse_num = num;

    num >>= 1;
    while (num)
    {
       reverse_num <<= 1;
       reverse_num |= num & 1;
       num >>= 1;
       count--;
    }
    reverse_num <<= count;
    return reverse_num;
}

int bit(uint64_t var,int pos)
{
    return (var >> pos) & 1;
}

void writeByte(uint8_t* byte, int count)
{
    printf("\n");
    for (int i = 0; i < count; i++)
        printf("0x%02x ", byte[i]);
    printf("\n");
}

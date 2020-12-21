/* 
 * University of Victoria
 * CSC 360 Assingment 3
 * Part 1: diskinfo
 * Kiana Pazdernik
 * V00896924
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "a3functions.h"


/*
 * Getting the name of the OS from disk image
 * Since the starting byte in the OSName is 3, and the boot sector is 8 bytes long
 */ 
void getOsName(char* osname, char* ptr) {

	for (int i = 0; i < 8; i++) {
		osname[i] = ptr[i + 3];
	}
}

/* 
 * Given a disklabel, that points to the volume label
 * and a pointer to mapped memory
 * Reads the volume label and places into disklabel
 */
void getDiskLabel(char* disklabel, char* ptr) {
	
	for (int i = 0; i < 8; i++) {
		disklabel[i] = ptr[i + 43];
	}

	if (disklabel[0] == ' ') {
		ptr += SECTOR_SIZE * 19;
		while (ptr[0] != 0x00) {
			if (ptr[11] == 0x08) {

				// label is found
				for (int i = 0; i < 8; i++) {
					disklabel[i] = ptr[i];
				}
			}
			ptr += 32;
		}
	}
}

/*
 * Function to count the files in a given disk
 * Given a pointer to mapped memory, and offset to the physical location of the directory and the max directory entries (224 for root and 16 other)
 * returns the amount of files in the directory
 */ 
int countFiles(char* ptr, int offset, unsigned long size)
{
    int count = 0;
    
    for(int i = 0; i < size; i ++){
        
		char *file = ptr + offset + (i * 32);
        unsigned short firstByte = *file;
        int attr = ptr[offset + 0x0b + i * 32];
        char filename[9];
        strncpy(filename, file, 8);
        filename[8] = '\0';
        
        char *file_count = file + 26;
        unsigned char file_count_buffer[2];
        file_count_buffer[0] = *file_count; 
        file_count++;
        file_count_buffer[1] = *file_count;
        unsigned int firstLogicalSector = (file_count_buffer[0]) | (file_count_buffer[1] << 8);
        
        char *size_c = file + 28;
        
        for(int j = 0; j < 4; j++){
            size_c++;
        }
        
        if(firstLogicalSector == 0 || firstLogicalSector == 1){
			continue;
		}
		// Ensuring the current position is not empty
        if((firstByte & 0xFF) != 0xE5 && (firstByte & 0xFF) != 0x00){
            // Checking to ensure it's not apart of a filename
			if(attr != 0x0F){ 
                //Checking if it's not a volume label
				if((attr & 0x08) == 0x00 && (attr & 0x02) == 0x00){
                    
					//Checking if it's a file
					if((attr & 0x10) == 0x00){
                        count += 1;
                        
                    }else{
						// Discluding the files beginning with '.'
                        if(strncmp(filename, ".       ", 2) == 0 || strncmp(filename, "..      ", 3) == 0){
							continue;
						}
                            
						// Recusively calling to up the count of the next file in the current dir
                        count = count + countFiles(ptr, ((31 + firstLogicalSector) * SECTOR_SIZE), 16);
                    }                   
                }
            }
        } 
    }
    
    unsigned int nextPossibleSector = getFATEntry(((offset / SECTOR_SIZE) - 31), ptr);
   
	if(nextPossibleSector < 0xFF8 && size == 16){
		// Recusively calling to up the count of the next file in the current dir
        return count + countFiles(ptr, ((31 + nextPossibleSector) * SECTOR_SIZE), 16);
    }
    return count;
}

/*
 * Returns the total number of files in the disk
 * Calls countFiles to do the work
 */ 
int getNumberOfFiles(char* ptr){
    
    int offset = SECTOR_SIZE * 19;
    int count = 0;
    count = count + countFiles(ptr, offset, 224);
    return count;
}


/* 
 * Getting the info and printing
 */ 
int main(int argc, char* argv[]) {

	if (argc < 2) {
		printf("Error: execute: ./diskinfo <file system image>\n");
		exit(1);
	}

	// Opening the disk image provided
	int filedisk = open(argv[1], O_RDWR);
	if (filedisk < 0) {
		printf("Error: failed to read disk image. \n");
		exit(1);
	}
	struct stat buff;

	// Get info from the file including retreiving disk size
	fstat(filedisk, &buff);

	// Mapping memory
	char* ptr = mmap(NULL, buff.st_size, PROT_READ, MAP_SHARED, filedisk, 0);
	if (ptr == MAP_FAILED) {
		printf("Error: failed to map memory. \n");
		exit(1);
	}
	close(filedisk);

	// Get disk info, boot sector and directory entries
	char* osname = malloc(sizeof(char));
	getOsName(osname, ptr);
	char* disklabel = malloc(sizeof(char));
	getDiskLabel(disklabel, ptr);
	int diskSize = getTotalDiskSize(ptr);
	int freeSize = getFreeDiskSize(diskSize, ptr);
	int numberOfRootFiles = getNumberOfFiles(ptr);

	// Read information from disk image then printing it
	int numberOfFatCopies = ptr[16];
	int sectorsPerFat = (ptr[22] + (ptr[23] << 8));
	
	// Printing the disk info in order:
	printf("OS Name: %s\n", osname);
	printf("Label of the disk: %s\n", disklabel);
	printf("Total size of the disk: %d bytes\n", diskSize);
	printf("Free size of the disk: %d bytes\n\n", freeSize);
	printf("==============\n");
	printf("The number of files in the image: %d\n\n", numberOfRootFiles);
	printf("=============\n");
	printf("Number of FAT copies: %d\n", numberOfFatCopies);
	printf("Sectors per FAT: %d\n", sectorsPerFat);

	// Unmapping the memory
	munmap(ptr, buff.st_size);
	free(osname);
	free(disklabel);

	return 0;
}

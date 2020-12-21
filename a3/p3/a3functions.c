/*
 * Helper file for all the funcations that are used often
 */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "a3functions.h"

/*
 * Given the pointer to mapped memory
 * Calculates the total disksize
 */ 
int getTotalDiskSize(char *ptr){
    int sectorCount = ptr[19] + (ptr[20] << 8);
    int bytesPerSector = ptr[11] + (ptr[12] << 8);

    int total = sectorCount * bytesPerSector;
    return total;
}

/*
 * int n is an Entry in the FAT table
 * char* ptr is a pointer to the mapped memory
 *  Takes the Fat table entry and pointer and returns the value of the FAT Table at n
 */ 
int getFATEntry(int n, char* ptr){
	int result;
	int firstByte;
	int secondByte;

	if ((n % 2) == 0) {
		firstByte = ptr[SECTOR_SIZE + ((3*n) / 2) + 1] & 0x0F;
		secondByte = ptr[SECTOR_SIZE + ((3*n) / 2)] & 0xFF;
		result = (firstByte << 8) + secondByte;
	} else {
		firstByte = ptr[SECTOR_SIZE + (int)((3*n) / 2)] & 0xF0;
		secondByte = ptr[SECTOR_SIZE + (int)((3*n) / 2) + 1] & 0xFF;
		result = (firstByte >> 4) + (secondByte << 4);
	}

	return result;
}

/*
 * Given the total bytes on a disk and a pointer to mapped memory
 * Returns the calculated free space on the disk
 */ 
int getFreeDiskSize(int disksize, char* ptr){
    
	
	int freeSectors = 0;
    int sectorCount = 2848;
    
    unsigned int sv;
    for(int i = 2; i <= sectorCount; i ++){
        sv = getFATEntry(i,ptr);
        if(sv == 0x00){
            freeSectors++;
        }
    }
    return freeSectors * SECTOR_SIZE;
}

/*
 * Given the filename and pointer to mapped memory, 
 * Calculates the filesize in bytes if the file exists
 * Return -1 if the file does not exist in root directory
 */ 
int getFileSize(char* fileName, char* ptr) {
	while (ptr[0] != 0x00) {
		if ((ptr[11] & 0b00000010) == 0 && (ptr[11] & 0b00001000) == 0) {
			char* currentFileName = malloc(sizeof(char));
			char* currentFileExtension = malloc(sizeof(char));
			
			for (int i = 0; i < 8; i++) {
				if (ptr[i] == ' ') {
					break;
				}
				currentFileName[i] = ptr[i];
			}
			for (int i = 0; i < 3; i++) {
				currentFileExtension[i] = ptr[i+8];
			}

			strcat(currentFileName, ".");
			strcat(currentFileName, currentFileExtension);

			if (strcmp(fileName, currentFileName) == 0) {
				int fileSize = (ptr[28] & 0xFF) + ((ptr[29] & 0xFF) << 8) + ((ptr[30] & 0xFF) << 16) + ((ptr[31] & 0xFF) << 24);
				return fileSize;
			}

		}
		ptr += 32;
	}
	return -1;
}

/*
 * Gets the first logical sector in a given directory entry
 * Given the directory pointer, finds the first logical sector
 */ 
int getFirstLogicalSector(char* ptr) {

	char *firstLogicalSec_ptr = ptr + 26;
    unsigned char firstLogicalSecbuffer[2];
	// Getting the 26 position of ptr
    firstLogicalSecbuffer[0] = *firstLogicalSec_ptr; 
    firstLogicalSec_ptr++;
	// Getting the 27 position of ptr
    firstLogicalSecbuffer[1] = *firstLogicalSec_ptr;
    unsigned int firstLogicalSec  = (firstLogicalSecbuffer[0]) | (firstLogicalSecbuffer[1] << 8);
    return firstLogicalSec;
}


/*
 * Given a pointer to directory entry
 * Find the filename
 */ 
char* getFilename(char * file){
    char * filename;
    filename = (char*)malloc(sizeof(char) * 21);
    if(filename == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(filename, '\0', 20);
    strncpy(filename, file, 8);
    return filename;
}

/* 
 * Given a pointer to directory entry
 * Find the extension of a given file
 */ 
char* getExtension(char * file){
    file = file + 8;
    char * extension;
    extension = (char*)malloc(sizeof(char) * 4);
    if(extension == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(extension, '\0', 4);
    strncpy(extension, file, 3);
    return extension;
}


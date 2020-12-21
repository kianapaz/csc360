/* 
 * University of Victoria
 * CSC 360 Assingment 3
 * Part 4: diskput
 * Kiana Pazdernik
 * V00896924
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <libgen.h>
#include "queue.h"
#include "a3functions.h"



/* 
 * Gets the next available FAT
 * Given pointer to mapped memory, discluding current entry
 */ 
unsigned int getNextAvailableFAT(char* ptr, int j){

    int sectorsAmount = getTotalDiskSize(ptr)/SECTOR_SIZE;
    for(int i = 2; i < sectorsAmount; i++){
        if(getFATEntry(i, ptr) == 0x00 && i != j){
            return i;
        }
    }
    printf("Error: No available FAT entries\n");
    return 0;
}


/*
 * Finding the next available Entry from a directory
 * Given a pointer to mapped memory, a physical location of the first logical sector and the max of directory entries
 * Return the entry physical address
 */ 
int getNextAvailableEntry(char* ptr, int offset, int entries){
    // Getting the sector number form the offset
    int current_cluster = (offset/SECTOR_SIZE) - 31; 
    
    while(1){
        for(int i = 0; i < entries; i++){
            char *direc = ptr + offset + (i * 32);
            unsigned int attr = ptr[offset + 0x0b + i * 32];
            if(!strcmp(direc, "") && attr == 0x00){
                return offset + (i * 32);
            }
        }
        current_cluster = getFATEntry(current_cluster, ptr);
        offset = (31 + current_cluster) * SECTOR_SIZE;
    }
}

/* 
 * Sets the FAT Entry to n,
 * The current entry being written should point to
 */ 
void setFATEntry(char* ptr, int n, int value){

	if ((n % 2) == 0) {
		ptr[SECTOR_SIZE + ((3*n) / 2) + 1] = (value >> 8) & 0x0F;
		ptr[SECTOR_SIZE + ((3*n) / 2)] = value & 0xFF;
	} else {
		ptr[SECTOR_SIZE + (int)((3*n) / 2)] = (value << 4) & 0xF0;
		ptr[SECTOR_SIZE + (int)((3*n) / 2) + 1] = (value >> 4) & 0xFF;
	}
    
}

/*
 * Given a file copy it to the disk
 * Given a pointer to mapped meory, a file pointer and filename and the directory to place the image in
 * Copies the image into the directory
 */ 
void copyFile(char* ptr, FILE * infile, char* filename, time_t last_modified, unsigned int directory_put){
    
    // Getitng the directory location to copy the file into the subdir
    unsigned int directory_location = (31 + directory_put ) * SECTOR_SIZE;
    
    // Finding the file end
    fseek(infile, 0l, SEEK_END);
    int fileSize = ftell(infile);
    rewind(infile);

    // Finding the next free fat
    int fat = getNextAvailableFAT(ptr, 0);
    unsigned int entry_offset = getNextAvailableEntry(ptr, directory_location, 16);

    // Seperate filename and extension
    char* token = strtok(filename, ".");
    char* title = malloc(9);
    if(token == NULL || title == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(title, '\0', 9);
    strcpy(title,token);
    token = strtok(0, ".");
    char* extension = malloc(4);
    if(extension == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(extension, '\0', 4);
    if(token != NULL){
        strcpy(extension,token);
    }
    strncpy(ptr + entry_offset, title, 8);
    strncpy(ptr + entry_offset + 8, extension, 3);
    
    // Write current date and time to directory entry
    // Setting the create date and time
	time_t t = time(NULL);
	struct tm *now = localtime(&t);
	int year = now->tm_year + 1900;
	int month = (now->tm_mon + 1);
	int day = now->tm_mday;
	int hour = now->tm_hour;
	int minute = now->tm_min;
    
    // Setting attributes
    ptr[entry_offset + 14] = ((minute << 5) & 0xE0);
    ptr[entry_offset + 15] = ((hour << 3) & 0xF8) | ((minute >> 3) & 0x07);
    ptr[entry_offset + 16] = ((month << 5) & 0xE0) | (day & 0x1F);
    ptr[entry_offset + 17] = ((year << 1) & 0xFE) | ((month >> 3) & 0x01);

    // Write first logical cluster into directory entry
    ptr[entry_offset + 26] = fat & 0xFF;
    ptr[entry_offset + 27] = (fat >> 8) & 0xFF;
    ptr[entry_offset + 28] = fileSize & 0xFF;
    ptr[entry_offset + 29] = (fileSize >> 8) & 0xFF;
    ptr[entry_offset + 30] = (fileSize >> 16) & 0xFF;
    ptr[entry_offset + 31] = (fileSize >> 24) & 0xFF;

    // Write to data sector
    unsigned int curr_fat = fat;
    unsigned int next_fat = 0;
    unsigned int physical_location = 0;
    unsigned char buffer[SECTOR_SIZE];
    int bytes_left = fileSize;
    size_t bytes_read = 0;

    // While writing to file
    while(bytes_read < fileSize){
        // Reading the bytes to file
        int bytes_read_this = fread(buffer, sizeof(unsigned char), SECTOR_SIZE, infile);
        bytes_read += bytes_read_this;
        bytes_left -= bytes_read_this;
        physical_location = (31 + curr_fat ) * SECTOR_SIZE;
        memcpy(ptr + physical_location, buffer, bytes_read_this);
        // Checking if there are any bytes left
        if(bytes_left > 0){
            next_fat = getNextAvailableFAT(ptr, curr_fat);
            setFATEntry(ptr, curr_fat, next_fat);
        }
        curr_fat = getFATEntry(curr_fat, ptr);
    }
    // Setting the FAT entry for the new file that waas created 
    setFATEntry(ptr, curr_fat, 0xFFF);
    return;
}


/*
 * Searches for the directory location
 * Given pointer to mapped memory, a pointer to a file, the filename, the pysical location of the current directory and the max directory entries
 * Retruns 1 if the directory is found, 0 if not
 */ 
int findDirectoryTree(char* ptr, struct Queue * q, FILE * infile, char* filename, time_t last_modified, long offset, int size){
    int found = 0;

    char* toTraverse;
    // Ensuring the queue isn't empty
    if(!isEmpty(q)){
        toTraverse = dequeue(q);
    }else{
        return found;
    }

    // Check if in the root dir and if the root dir is wanted
    if(strncmp(toTraverse, ".", 1) == 0 && ((offset / SECTOR_SIZE) - 31) == -12){
        copyFile(ptr, infile, filename, last_modified, - 12);
        return 1;
    }

    // For the size of 16
    for(int i = 0; i < size; i ++){
        char *file = ptr + offset + (i * 32);
        // Getting the first byte of the file
        unsigned short firstByte = *file; 
        int attr = ptr[offset + 0x0b + i * 32];

        // Getting the first logical sector 
        unsigned int firstLogicalSec = getFirstLogicalSector(file);
        if(firstLogicalSec == 0 || firstLogicalSec == 1 || firstLogicalSec == ((offset / SECTOR_SIZE) - 31)){
            continue;
        }
        // Checking for the bytes that aren't free
        if((firstByte & 0xFF) != 0xE5 && (firstByte & 0xFF) != 0x00){ 
            // If the name is not apart of the file name
            if(attr != 0x0F){ 
                // Not a volume label
                if((attr & 0x08) == 0x00 ){ 
                    // Getting the filename
                    char* filename_found = getFilename(file);

                    // Ensuring it is a subdirectory
                    if((attr & 0x10) != 0x00){
                        // ENsuring it isn't the subdir . or ..
                        if(strncmp(filename_found,".          ",2) == 0 || strncmp(filename_found,"..      ",3) == 0 || strcmp(filename_found,"") == 0){
                            continue;
                        }
                            
                         // Getting the next physical location to continue traversing
                        unsigned int physicalLocation = (31 + firstLogicalSec ) * SECTOR_SIZE;
                        if(strncmp(filename_found, toTraverse, (strlen(toTraverse))) == 0){
                            
                            // If the file is found - copy it to the given dir
                            copyFile(ptr, infile, filename, last_modified, firstLogicalSec);
                            return 1;
                        }
                        found += findDirectoryTree(ptr, q, infile, filename, last_modified, physicalLocation, 16);
                    }
                    free(filename_found);     
                }
            }
        } 
    }
    return found;
}



/*
 * Overall function to put the given file into the given directory
 */ 
void diskput(char* ptr, struct Queue * q, char* filename){
    
    // Opening a file with with the given name
    FILE * infile = fopen(filename, "r");
    if(infile == NULL){
        printf("File not found: %s\n",filename);
        return;
    }
    // CHecking the last modified time
    struct stat buffer;
    stat(filename, &buffer);
    time_t last_modified = buffer.st_mtime;

    // Checking the end of the file
    fseek(infile, 0L, SEEK_END);
    int fileSize;
    fileSize = ftell(infile);
    rewind(infile);
    
    // ENsuring there is enough space to add this file to the subdirectory
    int diskSize = getTotalDiskSize(ptr);
    int freeSpace = getFreeDiskSize(diskSize, ptr);
    if(fileSize > freeSpace){
        printf("Not enough free space in the disk image. \n");
        return;
    }
    
    // Checking if the file was found
    int found = findDirectoryTree(ptr, q, infile, filename, last_modified, (SECTOR_SIZE * 19), 224);
    fclose(infile);
    if(found == 0){
        printf("The directory not found.\n");
        return;
    }
    return;
}


int main(int argc, char* argv[]){
    if (argc < 3) {
		printf("Error: execute: ./diskput <file system image> <file>\n");
		exit(1);
	}
    
    // Setting up the filename adn diskname
    char *file_image = malloc(strlen(argv[1])*sizeof(char));
    char *path = malloc(strlen(argv[2])*sizeof(char));
    if(file_image == NULL || path == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    strcpy(file_image, argv[1]);
    strcpy(path, argv[2]);

    int filedisk;
    filedisk = open(file_image, O_RDWR);
    if(filedisk < 0){
        printf("Error reading image %s\n",file_image);
        return 0;
    }
    // BEginning to read the disk
    struct stat buffer;
    fstat(filedisk, &buffer);
    
    // Creating mapped memory
    char *ptr = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, filedisk, 0);
    if (ptr == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

    // Setting up the attributes for diskput
    char* filename = basename(path);
    char* direc = dirname(path);
    char* limiter = "/";
    struct Queue * q = createQueue(10);
    char* root = ".";
    // Checking if the file will be placed in the directory
    if(strncmp(direc, root, 1) == 0){
        q = enqueue(q, direc);
        diskput(ptr, q, filename);
        return 1;
    }
    // Otherwise get the directory the file goes into
    char* token = strtok(direc, limiter);
    while(token != NULL){
        q = enqueue(q, token);
        token = strtok(0, limiter);
    }
    diskput(ptr, q, filename);

    // Unmapping the memory and closing file
    munmap(ptr, buffer.st_size);
    close(filedisk);
    return 1;

}


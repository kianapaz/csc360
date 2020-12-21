/* 
 * University of Victoria
 * CSC 360 Assingment 3
 * Part 3: diskget
 * Kiana Pazdernik
 * V00896924
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include "a3functions.h"

/* 
 * Given a pointer find an unsigned int of the file size
 */ 
unsigned int getFileSizeS(char * file){
    char *size_p = file + 28;
    unsigned char sizebuf[4];
    
    for(int j = 0; j < 4; j++){
        sizebuf[j] = *size_p;
        size_p++;
    }
    unsigned int size_int = (sizebuf[0]) | (sizebuf[1] << 8) | (sizebuf[2] << 16) | (sizebuf[3] << 24);
    return size_int;
}

/*
 * Given a pointer, a filename, the file extension, the first logical sector, and the size
 * copy the file from the given disk image to the current directory 
 */ 
void copyFile(char* ptr, char* filename, char* extension, int firstLogicalS, int size){
   
    unsigned int sector_next = firstLogicalS;
    unsigned int next_location = (31 + (sector_next)) * SECTOR_SIZE;

    int curr_size = 0;
    int length_full_file = strlen(filename) + 1 + strlen(extension) + 1;
    char* full_file = malloc(length_full_file);
    if(full_file == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(full_file, '\0', length_full_file);
    strcat(full_file, filename);
    strcat(full_file, (const char *) ".");
    strcat(full_file, extension);
    
    FILE *infile = fopen(full_file, "w+");
    free(full_file);
    while(sector_next < 0xFF8 && curr_size < size){
        
        for(int i = 0; i < SECTOR_SIZE; i++){
            if(curr_size < size){
                fprintf(infile, "%c", ptr[next_location + i]);
                curr_size += 1;
            }
            else{
                break;
            }   
        }
        sector_next = getFATEntry(sector_next, ptr);
        next_location = (31 + (sector_next)) * SECTOR_SIZE;
    }
    fclose(infile);
}

/*
 * Given a pointer to mapped meory, a filename, the file extension, the offset and the max number of directory entries 
 * Finds the given filename in the directory, ensuring it exists in order to copy
 * otherwise returns 0 for file not found
 */ 
int traverseDirec(char* ptr, char* filename, char* extension, long offset, int size){
    
    int found = 0;

    // FOr the size of 16
    for(int i = 0; i < size; i ++){
        char *file = ptr + offset + (i * 32);
        // Getting the first byte of the file
        unsigned short firstByte = *file; 
        int attr = ptr[offset + 0x0b + i * 32];

        // Getting the first logical sector 
        unsigned int firstLogicalS = getFirstLogicalSector(file);
        if(firstLogicalS == 0 || firstLogicalS == 1 || firstLogicalS == ((offset / SECTOR_SIZE) - 31)){
            continue;
        }

        // Checking for the bytes that aren't free
        if((firstByte & 0xFF) != 0xE5 && (firstByte & 0xFF) != 0x00){ 
            // If the name is not apart of the file name
            if(attr != 0x0F){ 
                // Not a volume label
                if((attr & 0x08) == 0x00 ){ 
                    // Getting the filename, the extension and size
                    char* filename_found = getFilename(file);
                    char* extension_found = getExtension(file);
                    int size_file = (int)getFileSizeS( file);

                    // Ensuring it is a file
                    if((attr & 0x10) == 0x00){
                        if(strncmp(filename_found, filename, strlen(filename)) == 0){
                            if(strncmp(extension_found, extension, strlen(extension)) == 0){
                                found += 1;
                                copyFile(ptr, filename, extension, firstLogicalS, size_file);
                                return found;
                            }
                        }
                    }else{
                        // ENsuring it isn't the subdir . or ..
                        if(strncmp(filename_found, ".       ", 2) == 0 || strncmp(filename_found, "..      ", 3) == 0){
                            continue;
                        }
                            
                        // Getting the next physical location to continue traversing
                        unsigned int physicalLocation = (33 + firstLogicalS - 2) * SECTOR_SIZE;
                        found += traverseDirec(ptr, filename, extension, physicalLocation, 16);
                    }
                    free(filename_found); 
                    free(extension_found);                 
                }
            }
        } 
    }
    // Returning if the given filename is found
    return found;
}


int main(int argc, char* argv[]){
    if (argc < 3) {
		printf("Error: execute: ./diskget <file system image> <file to copy> \n");
		exit(1);
	}
    
    char *imagename = malloc(strlen(argv[1])*sizeof(char));
    char *filename = malloc(strlen(argv[2])*sizeof(char));
    if(imagename == NULL || filename == NULL){
        printf("Failed to allocate memory.\n");
        exit(1);
    }
    strcpy(imagename, argv[1]);
    strcpy(filename, argv[2]);
        
    int filedisk = open(imagename, O_RDONLY);
    if(filedisk < 0){
        printf("Error reading image %s \n",imagename);
        exit(1);
    }
    
    struct stat buffer;
    fstat(filedisk, &buffer);
    
    char *ptr = mmap(NULL, buffer.st_size, PROT_READ, MAP_SHARED, filedisk, 0);
    if (ptr == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}
    char* token = strtok(filename, ".");
    token = strtok(0, ".");
    if(token == NULL){
        printf("Error. No extension specified.\n");
        exit(1);
    }
    char* extension = token;
    
    // Ensuring the file exists
    int found = traverseDirec(ptr, filename, extension, (SECTOR_SIZE * 19), 224);
    if(found == 0){
        printf("File not found: %s\n", filename);
        exit(0);
    }
    
    // Unmapping the memory and closing file
	munmap(ptr, buffer.st_size);
	//close(filedisk);

    return 0;
}
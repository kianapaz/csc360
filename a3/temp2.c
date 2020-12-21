/* 
 * University of Victoria
 * CSC 360 Assingment 3
 * Part 2: disklist
 * Kiana Pazdernik
 * V00896924
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include "queue.h"
#include "a3functions.h"


/*
 * Given a pointer to the directpry file entry, 
 * Finds the file size
 */ 
char* getFileSizeS(char * file){
    char *size_ptr = file + 28;
    unsigned char sizebuf[4];
    for(int j = 0; j < 4; j++){
        sizebuf[j] = *size_ptr;
        size_ptr++;
    }
    unsigned int size_int = (sizebuf[0]) | (sizebuf[1] << 8) | (sizebuf[2] << 16) | (sizebuf[3] << 24);
    char* filesize;
    filesize = (char*)malloc(sizeof(char) * 10 + 1);
    if(filesize == NULL){
        printf("Failed to allocate memory.\n");
		exit(1);
    }
    memset(filesize, ' ', 10);
    sprintf(filesize,"%u",size_int);
    filesize[10] = '\0';
    return filesize;
}


/* 
 * Finds all the files in a given disk
 * Given a pointer to mapped memory, the directory name, the physical location of the directory, the max directry entries (224 for root, 16 for data section)
 * and a value to determine if it's the beginning of a subdirectory/directory
 */ 
void findDirectoryTree(char* ptr, char* directory_name, long offset, int size, int first){
    
    if(first == 1){
        printf("\n%s\n==================\n", directory_name);
    }
    struct Queue *traverseLocation = createQueue(10);
    struct Queue *traverseDir = createQueue(10);
    
    
    char* subdir = (char*)malloc(sizeof(char));
    strncpy(subdir, "/", 1);

    // Looping through the size of 16
    for(int i = 0; i < size; i ++){
        
        char *file = (ptr + offset + ( i * 32 ));
        // Getting the first byte of the file
        unsigned short firstByte = *file; 
        int attr = ptr[offset + 0x0b + i * 32];
        
        unsigned int firstLogicalSec = getFirstLogicalSector(file);
        if(firstLogicalSec == 0 || firstLogicalSec == 1 || firstLogicalSec == ((offset / SECTOR_SIZE) - 33 + 2)){
            continue;
        } 
        // Checking for the bytes that aren't free
        if((firstByte & 0xFF) != 0xE5 && (firstByte & 0xFF) != 0x00){ 
            // If the name is not apart of the file name
            if(attr != 0x0F){ 
                // Not a volume label
                if((attr & 0x08) == 0x00 ){ 
                    
                    // Get the time and date the file was created
                    int year = (((*(unsigned short *)(file + 16) & 0xFE00)) >> 9) + 1980;
                    int month = ((*(unsigned short *)(file + 16)) & 0x1E0) >> 5;
                    int day = ((*(unsigned short *)(file + 16)) & 0x1F);
                    int hour = ((*(unsigned short *)(file + 14)) & 0xF800) >> 11;
                    int minute = ((*(unsigned short *)(file + 14)) & 0x7E0) >> 5;
                    
                    // Getting the filename with extensions
                    char* filesize = getFileSizeS(file);
                    
                    // Getting the Filename and DIrectory Names
                    char* F_filename = (char*)malloc(sizeof(char) * 21);
                    if(F_filename == NULL){
                        printf("Failed to allocate memory.\n");
		                exit(1);
                    }
                    memset(F_filename, '\0', 20);
                    

                    // getting the filename
                    for(int y = 0; y < 8; y++){
                        // Ensuring a space isn't copied into the file/dir name
                        if(ptr[offset + ( i * 32 ) + y] == ' '){
                            continue;
                        }
                        char *temp = (ptr + offset + ( i * 32 ) + y);
                        strncat(F_filename, temp, 1);
                    }
                    
                    char* D_filename = (char*)malloc(sizeof(char) * 21);
                    if(D_filename == NULL){
                        printf("Failed to allocate memory.\n");
		                exit(1);
                    }
                    strncat(D_filename, F_filename, strlen(F_filename));

                    // Getting the filename for the directory name
                    char* dirname = getFilename(file);
                    char* extension = getExtension(file);

                    char* filename_ext = malloc(strlen(dirname) + 4);
                    if(filename_ext == NULL){
                        printf("Failed to allocate memory.\n");
		                exit(1);
                    }
                    memset(filename_ext, '\0', strlen(dirname) + 5);
                    strncpy(filename_ext, dirname, strlen(dirname));
                    filename_ext[strlen(dirname)] = '.';
                    strncat(filename_ext, extension, strlen(extension));
                    
                    if((attr & 0x10) == 0x00){
                        // Ensuring the extension isn't blank
                        if(strncmp(extension, "   ", 3) != 0){
                            strncat(F_filename, ".", 1);
                        }
                        
                    }
                    // Getting the extension for the filename
                    for(int y = 0; y < 3; y++){
                        // Ensuring a space isn't copied into the file/dir name
                        if(ptr[offset + ( i * 32 ) + 8 + y] == ' '){
                            continue;
                        }
                        char *temp = (ptr + offset + ( i * 32 ) + 8 + y);
                        strncat(F_filename, temp, 1);
                    }

                    // Checking whether this is a directory or subdirectory 
                    if((attr & 0x10) == 0x00){
                        
                        printf("%s %10s %20s %d-%02d-%02d %02d:%02d\n", "F", filesize, F_filename, year, month, day, hour, minute);
                        free(F_filename);

                    }else{
                        // Ignoring the . and .. subdirs 
                        if(strncmp(dirname,".       ",2) == 0 || strncmp(dirname,"..      ",3) == 0){
                            continue;
                        }
                        unsigned int physicalLocation = (33 + firstLogicalSec - 2) * SECTOR_SIZE;
                        printf("%s %10s %20s %d-%02d-%02d %02d:%02d\n", "D", filesize, D_filename, year, month, day, hour, minute);

                        // Getting the physical location 
                        char* physicalLocation_s;
                        physicalLocation_s = malloc(((int)log10(physicalLocation) + 1) + 1);
                        if(physicalLocation_s == NULL){
                            printf("Failed to allocate memory.\n");
		                    exit(1);
                        }
                        // Adding the physical location to the location queue to track the current location
                        sprintf(physicalLocation_s, "%u", physicalLocation);
                        traverseLocation = enqueue(traverseLocation, physicalLocation_s);

                        // Getting the dirrectory name with the sub directory names
                        
                        if(first == 1 && strncmp(directory_name, "ROOT ", 5) != 0){
                            
                            strncat(directory_name, "/", 1);
                            strncat(directory_name, D_filename, strlen(D_filename));
                            strncat(subdir, directory_name, strlen(directory_name));
                            
                        }else{
                            strncpy(subdir, D_filename, strlen(D_filename));
                        }
                        // Adding the directory name to the current directory being traversed 
                        traverseDir = enqueue(traverseDir, subdir);
                        free(dirname);

                        
                    }  
                                   
                }
            }
        } 
    }
// Checking if the subdir is empty and there is another subdir to traverse
    while(!isEmpty(traverseLocation)){
        if(isEmpty(traverseDir)){
            printf("Error Traversing the directory and finding files/subdirs. \n");
        }
            
        char* loc = dequeue(traverseLocation);
        long physicalLocation = strtol(loc, NULL, 10);
        free(loc);
        char* name = dequeue(traverseDir);
        findDirectoryTree(ptr, name, physicalLocation, 16, 1);
    }
    free(traverseLocation);
    free(traverseDir);
    // Otherwise find the new physical loaction and continue traversing
    unsigned int nextPossibleSector = getFATEntry(((offset / SECTOR_SIZE ) - 31),ptr);
    unsigned int next_location = ((33 + nextPossibleSector - 2) * SECTOR_SIZE);
    if(nextPossibleSector < 0xFF8 && size == 16){
        findDirectoryTree(ptr, directory_name, next_location, 16, 0);
    }    
    
    return;
}


int main(int argc, char* argv[]){
    
    if (argc < 2) {
		printf("Error: execute: ./diskinfo <file system image>\n");
		exit(1);
	}

	char *filename = malloc(strlen(argv[1])*sizeof(char));
	if(filename == NULL){
		printf("Failed to allocate memory.\n");
		exit(1);
	}
	strcpy(filename, argv[1]);
    int filedisk = open(filename, O_RDONLY);
    if(filedisk < 0){
        printf("Failed to read file: %s \n",filename);
        exit(1);
    }

    struct stat buffer;
    fstat(filedisk, &buffer);
    
    char *ptr = mmap(NULL, buffer.st_size, PROT_READ, MAP_SHARED, filedisk, 0);
    if (ptr == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}
    findDirectoryTree(ptr, "ROOT ", (SECTOR_SIZE * 19), 224, 1);

    // Unmapping the memory
    munmap(ptr, buffer.st_size);
	return 0;
    
}


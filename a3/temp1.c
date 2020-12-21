#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "disk.h"

void diskinfo(char * filename);
void storeOSName(char * p);
void storeDiskLabel(char * p);
unsigned int getNumFATCopies(char * p);
unsigned int getSectorsPerFAT(char * p);
int getNumFiles(char * p);
int countFiles(char * p, int offset, unsigned long size);

struct diskinfo {
    char osname[9];
    char labeldisk[12];
    int totalsizedisk;
    int freesizedisk;
    int numfiles;
    int numFATcopies;
    int secperFAT;
}info;

int main(int argc, char* argv[])
{
    if(argc == 2)
    {
        char *filename = malloc(strlen(argv[1])*sizeof(char));
        if(filename == NULL){
            fprintf(stderr, "Fatal: failed to allocate memory.\n");
            abort();
        }
        strcpy(filename, argv[1]);
        diskinfo(filename);
    }
    else
    {
        printf("Incorrect Usage. Try %s <filename>\n",argv[0]);
    }
}

/**
Gets disk info and prints it out
Parameters:
char * filename - Name of disk image
Returns: void
**/
void diskinfo(char * filename)
{
    int fd;
    fd = open(filename, O_RDONLY);
    if(fd < 0){
        fprintf(stderr, "Error: failed to open file.\n");
        return;
    }
    //printf("Reading disk image file: %s\n",filename);
    struct stat sf;
    fstat(fd, &sf);
    char *p;
    p = mmap(NULL, sf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    storeOSName(p);
    storeDiskLabel(p);
    info.totalsizedisk = getTotalSizeDisk(p);
    info.freesizedisk = getFreeSizeDisk(p);
    info.numfiles = getNumFiles(p);
    info.numFATcopies = getNumFATCopies(p);
    info.secperFAT = getSectorsPerFAT(p);
    // fclose(fp);
    printf("OS Name: %s\n",info.osname);
    printf("Label of the disk: %s\n",info.labeldisk);
    printf("Total size of the disk: %d\n",info.totalsizedisk);
    printf("Free size of the disk: %d\n",info.freesizedisk);
    printf("\n==============\n");
    printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n",info.numfiles);
    printf("\n==============\n");
    printf("Number of FAT copies: %d\n",info.numFATcopies);
    printf("Sectors per FAT: %d\n\n\n",info.secperFAT);
    // printf("Get location for 230th FAT: %d\n",getLocation(230));
    return;
}

/**
Gets OS name and stores it in the info struct
Parameters:
char * p - Pointer to disk image
Returns: void
**/
void storeOSName(char *p)
{
    unsigned char *os = (unsigned char *)p + 3;
    int i;
    for(i = 0; i < 8; i++){
        info.osname[i] = *os;
        os++;
    }
    info.osname[8] = '\0';
}

/**
Gets disk label and stores it in the info struct
Parameters:
char * p - Pointer to disk image
Returns: void
**/
void storeDiskLabel(char *p)
{
    int root_offset = 512 * 19;
    int i;
    for (i = 0; i < 224; i++){
        int flag = p[root_offset + 0x0b + i*32];
        if(flag == 0x08) {
            char *offset = p+root_offset+(i*32);
            for(i = 0; i < 11; i++){
                info.labeldisk[i] = *offset;
                offset++;
            }
            info.labeldisk[11] = '\0';
            break;
        }
    }
    if(!strcmp(info.labeldisk, "")){
        strncpy(info.labeldisk, p+43, 11);
        info.labeldisk[11] = '\0';
    }
}

/**
Gets the number of FAT copies in the disk image
Parameters:
char * p - Pointer to disk image
Returns:
unsigned int - Number of FAT copies in the disk image
**/
unsigned int getNumFATCopies(char * p){
    unsigned int numfats = (unsigned int)(*(p + 16));
    return numfats;
}

/**
Gets the amount of Sectors per FAT
Parameters:
char * p - Pointer to disk image
Returns:
unsigned int - Number of sectors per FAT
**/
unsigned int getSectorsPerFAT(char * p){
        char *spf_c = p+22;
        unsigned char spfbuf[2];
        spfbuf[0] = *spf_c; // 22
        spf_c++;
        spfbuf[1] = *spf_c; // 23
        unsigned int spf  = (spfbuf[0]) | (spfbuf[1]<<8);
        return spf;
}

/**
Gets the amount of files in the disk image
Parameters:
char * p - Pointer to disk image
Returns:
int - Number of files in the disk image
**/
int getNumFiles(char * p){
    // starting from the root directory, traverse the directory tree.
    // # of files = # of directory entries in each directory
    // However, skip a directory if:
    // the values of it's attribute field is 0x0F
    // the 4th bit of it's attribute field is 1
    // the Volume Label bit of it's attribute field is 1
    // the directory_entry is free
    int offset = 512 * 19;
    int numFiles = 0;
    numFiles = numFiles + countFiles(p, offset, 224);
    return numFiles;
}

/**
Helper function that recursively calls itsself to count all files in the disk image
Parameters:
char * p - Pointer to disk image
int offset - Physical location of directory to search through
unsigned long size - Maximum amount of directory entries in the directory (224 for root, 16 otherwise)
Returns:
int - The amount of files in the current directory + amount of files in recursed directory
**/
int countFiles(char * p, int offset, unsigned long size)
{
    int numFiles = 0;
    int i = 0;
    for(i = 0; i < size; i ++){
        char *file = p+offset+(i*32);
        unsigned short ffb = *file;
        int attr = p[offset + 0x0b + i*32];
        char filename[9];
        strncpy(filename, file, 8);
        filename[8] = '\0';
        
        char *flc_c = file+26;
        unsigned char flcbuf[2];
        flcbuf[0] = *flc_c; // 26
        flc_c++;
        flcbuf[1] = *flc_c; // 27
        unsigned int flc  = (flcbuf[0]) | (flcbuf[1]<<8);
        
        char *size_c = file+28;
        unsigned char sizebuf[4];
        int j;
        for(j = 0; j < 4; j++){
            sizebuf[j] = *size_c;
            size_c++;
        }
        //unsigned int size = (sizebuf[0]) | (sizebuf[1]<<8) | (sizebuf[2]<<16) | (sizebuf[3]<<24);
        if(flc == 0 || flc == 1)
            continue;
        if((ffb&0xFF) != 0xE5 && (ffb&0xFF) != 0x00){ // not free
            if(attr != 0x0F){ // not part of long file name
                if((attr&0x08) == 0x00 && (attr&0x02) == 0x00){ // not a volume label
                    if((attr&0x10) == 0x00){// not a subdirectory
                        numFiles = numFiles + 1;
                        //printf("Found file %s at %d of size %d\n",filename,flc,size);
                    }else{
                        if(strncmp(filename,".       ",2) == 0 || strncmp(filename,"..      ",3) == 0)
                            continue;
                        unsigned int logicalID = flc;
                        unsigned int physicalID = 33 + logicalID - 2;
                        unsigned int physicalLocation = physicalID * 512;
                        numFiles = numFiles + countFiles(p, physicalLocation, 16);
                    }                   
                }
            }
        } 
    }
    int current_cluster = (offset/512)-33+2;
    unsigned int nextPossibleSector = getSectorLocation(current_cluster,p);
    unsigned int next_location = getPhysicalLocation(nextPossibleSector);
    if(nextPossibleSector < 0xFF8 && size == 16){
        return numFiles + countFiles(p,next_location,16);
    }
    return numFiles;
}
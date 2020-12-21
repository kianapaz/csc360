/*
 * A3 global variables
 */ 

#define TRUE 1
#define FALSE 0
#define SECTOR_SIZE 512

/*
 * Frequently used functions to help with finding FAT info
 */ 

int getTotalDiskSize(char* ptr);
int getFATEntry(int n, char* ptr);
int getFreeDiskSize(int disksize, char* ptr);
int getFileSize(char* fileName, char* ptr);
int getFirstLogicalSector(char* ptr);
char* getExtension(char * file);
char* getFilename(char * file);

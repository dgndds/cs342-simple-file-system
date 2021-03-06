#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "simplefs.h"


// Global Variables =======================================
int vdisk_fd; // Global virtual disk file descriptor. Global within the library.
              // Will be assigned with the vsfs_mount call.
              // Any function in this file can use this.
              // Applications will not use  this directly. 
// ========================================================


//Structs =======================================
struct Supernode{
	int blockCount;
	int freeBlockCount;
	int iBlockCount;
	int fcbCount;
	int freeDirCount;
	int freeFcbCount;
	int fcbStartIndex;
	int dirsStartIndex;
	int bitmapStartIndex;
	int padding[1015];
};

struct bitmapBlock{
	int bitmap[1024];
};

struct DirEntry{
	int avaible;
	char fileName[110];
	int fcbBlockIndex[2];
	char padding[2];
};

struct DirBlock{
	struct DirEntry dirs[32];
};

struct FCBEntry{
	int avaible;
	int indexBlockIndex;
	int fileSize;
	int lasPos[2];
	int mode;
	int padding[26];
};

struct FcbBlock{
	struct FCBEntry fcbs[32];	
};

struct indexBlock{
	int dataPtrs[1024];
};

struct DataBlock{
	char data[4096];
};

int openFiles[128];
int openFileCount = 0;

// ========================================================

// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("read error\n");
	return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("write error\n");
	return (-1);
    }
    return 0; 
}

void SetBitmapBit(int bitmap[],int index){
    bitmap[index/32] |= 1<<(index%32);
}

void ResetBitmapBit(int bitmap[],int index){
    bitmap[index/32] &= ~(1<<(index%32));
}

int TestBitmapBit(int bitmap[],int index){
    return ((bitmap[index/32]&(1<<(index%32)))!= 0) ;     
}

void SetAvaibleBit(int index, int value){
	struct Supernode* superBlock = malloc(sizeof(struct Supernode));
	read_block(superBlock,0);
	
	int bitmapStartIndex = superBlock->bitmapStartIndex;
	
	//Find block location
	int blockSize = 4096*8;
	int blockIndex = ((index) / blockSize) + bitmapStartIndex;
	int bitIndex = (index) % (blockSize);
	
	struct bitmapBlock* bitmapBlock = malloc(sizeof(struct bitmapBlock));
	read_block(bitmapBlock,blockIndex);
	
	if(value){
		SetBitmapBit(bitmapBlock->bitmap,bitIndex);
	}else{
		ResetBitmapBit(bitmapBlock->bitmap,bitIndex);
	}
	
	write_block(bitmapBlock,blockIndex);
}

int* FindFreeDirEntry(){
	struct Supernode* superBlock = malloc(sizeof(struct Supernode));
	read_block(superBlock,0);
	
	int dirsStartIndex = superBlock->dirsStartIndex;
	
	int foundFreeDirEntry = 0;
	static int dirBlockIndex[2] = {-1,-1};
	
    struct DirBlock* dirBlock = malloc(sizeof(struct DirBlock));
    
    for(int i = dirsStartIndex; i < dirsStartIndex + 4; i++){
    	read_block(dirBlock,i);
    	
    	for(int k = 0; k < 32; k++){
    		if(dirBlock->dirs[k].avaible == 1){
    			foundFreeDirEntry = 1;
    			dirBlockIndex[0] = i;
    			dirBlockIndex[1] = k;
    			break;
    		}
    	}
    	
    	if(foundFreeDirEntry){
    		break;
    	}
    }
    	
   	return dirBlockIndex;
}

int* FindFreeFcbEntry(){
	struct Supernode* superBlock = malloc(sizeof(struct Supernode));
	read_block(superBlock,0);
	
	int fcbStartIndex = superBlock->fcbStartIndex;
	
	int foundFreeFcbEntry = 0;
	static int fcbBlockIndex[2] = {-1,-1};
	
    struct FcbBlock* fcbBlock = malloc(sizeof(struct FcbBlock));
    
    for(int i = fcbStartIndex; i < fcbStartIndex + 4; i++){
    	read_block(fcbBlock,i);
    	
    	for(int k = 0; k < 32; k++){
    		if(fcbBlock->fcbs[k].avaible){
    			foundFreeFcbEntry = 1;
    			fcbBlockIndex[0] = i;
    			fcbBlockIndex[1] = k;
    			break;
    		}
    	}
    	
    	if(foundFreeFcbEntry){
    		break;
    	}
    }
    	
   	return fcbBlockIndex;
}

int findFreeBlock(){
	struct Supernode* superBlock = malloc(sizeof(struct Supernode));
	read_block(superBlock,0);	
	
	int bitmapStartIndex = superBlock->bitmapStartIndex;
	int freeIndex = -1;
	int freeIndexFound = 0;
	int currIndex = 0;
	
	struct bitmapBlock* bitBlock = malloc(sizeof(struct bitmapBlock));
	
	for(int i = bitmapStartIndex; i < bitmapStartIndex + 4; i++){
    	read_block(bitBlock,i);
    	
		for(int k = 0; k < 4096*8; k++){
    		if(TestBitmapBit(bitBlock->bitmap,k) == 0){
    			freeIndexFound = 1;
    			freeIndex = currIndex;
    			break;
    		}else{
    			currIndex++;
    		}
    	}
    	
    	if(freeIndexFound){
    		break;
    	}
	}
	
	return freeIndex;
}

int* FindFileIndexByName(char* file){
	struct Supernode* superBlock = malloc(sizeof(struct Supernode));
	read_block(superBlock,0);
	
	int dirsStartIndex = superBlock->dirsStartIndex;
	
	int foundFileDirEntry = 0;
	static int dirBlockIndex[2] = {-1,-1};
	
    struct DirBlock* dirBlock = malloc(sizeof(struct DirBlock));
    
    for(int i = dirsStartIndex; i < dirsStartIndex + 4; i++){
    	read_block(dirBlock,i);
    	
    	for(int k = 0; k < 32; k++){
    		if(strcmp(dirBlock->dirs[k].fileName,file) == 0){
    			foundFileDirEntry = 1;
    			dirBlockIndex[0] = i;
    			dirBlockIndex[1] = k;
    			break;
    		}
    	}
    	
    	if(foundFileDirEntry){
    		break;
    	}
    }
    	
   	return dirBlockIndex;
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

// this function is partially implemented.
int create_format_vdisk (char *vdiskname, unsigned int m)
{	
    char command[1000];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE;
    //    printf ("%d %d", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
             vdiskname, BLOCKSIZE, count);
    printf ("executing command = %s\n", command);
    system (command);

    // now write the code to format the disk below.
    // .. your code...
    
    sfs_mount(vdiskname);
    
    //Set all files as closed
    for(int i = 0; i < 128; i++){
    	openFiles[i] = 0;
    }
    
    //Set Superblock
    struct Supernode* superblock = malloc(sizeof(struct Supernode));
   
    superblock->blockCount = count;
    superblock->freeBlockCount = count;
    superblock->iBlockCount = 128;
    superblock->fcbCount = 128;
    superblock->freeDirCount = 128;
    superblock->freeFcbCount = 128;
    superblock->fcbStartIndex = 9;
    superblock->dirsStartIndex = 5;
    superblock-> bitmapStartIndex = 1;
    
    
    
    //Set Bitmap
    for(int i = 1; i <= 4; i++){
    	struct bitmapBlock* bitBlock = malloc(sizeof(struct bitmapBlock));
    	read_block(bitBlock,i);
    	
    	if(i == 1){
    		//Set free blocks
    		for(int k = 0; k < 12; k++){
    			SetBitmapBit(bitBlock->bitmap,k);
    		}
    		
    		for(int k = 12; k < 4096*8; k++){
    			ResetBitmapBit(bitBlock->bitmap,k);
    		}
    		
    	}else{
    		for(int k = 0; k < 4096*8; k++){
    			ResetBitmapBit(bitBlock->bitmap,k);
    		}
    	}
    	
    	write_block(bitBlock,i);
    	superblock->freeBlockCount--;
    }
    
    //Setup Directory Entries
    for(int i = 5; i <= 8; i++){
    	struct DirBlock* dirBlock = malloc(sizeof(struct DirBlock));
    	read_block(dirBlock,i);
    	
    	for(int k = 0; k < 32; k++){
    		struct DirEntry dirEntry;
    		memcpy(dirEntry.fileName,"",1);
    		dirEntry.fcbBlockIndex[0] = -1;
    		dirEntry.fcbBlockIndex[1] = -1;
    		dirEntry.avaible = 1;
    		
    		dirBlock->dirs[k] = dirEntry;
    	}
    	
    	write_block(dirBlock,i);
    }
    
    //Setup FCBs
    for(int i = 9; i <=12; i++){
    	struct FcbBlock* fcbBlock = malloc(sizeof(struct FcbBlock));
    	read_block(fcbBlock,i);
    	
    	for(int k = 0; k < 32; k++){
    		struct FCBEntry fcb;
    		
    		fcb.avaible = 1;
    		fcb.indexBlockIndex = -1;
    		fcb.fileSize = -13123;
    		
    		fcbBlock->fcbs[k] = fcb;
    	} 
    	
    	write_block(fcbBlock,i);
    }
    
    write_block(superblock,0);
    sfs_umount();
    return (0); 
}


// already implemented
int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it. 
    vdisk_fd = open(vdiskname, O_RDWR); 
    return(0);
}


// already implemented
int sfs_umount ()
{
    fsync (vdisk_fd); // copy everything in memory to disk
    close (vdisk_fd);
    return (0); 
}


int sfs_create(char *filename)
{	
	struct Supernode* superBlock = malloc(sizeof(struct Supernode));
	read_block(superBlock,0);
	
	int* dirBlockIndex = FindFreeDirEntry();
	
	if(dirBlockIndex[0] == -1 || dirBlockIndex[1] == -1){
		printf("No free directory entries!\n");
		printf("File creation failed! file: %s\n",filename);
		return -1;
	}
	
	int* fcbBlockIndex = FindFreeFcbEntry();
	
	if(fcbBlockIndex[0] == -1 || fcbBlockIndex[1] == -1){
		printf("No free fcbs!\n");
		printf("File creation failed! file: %s\n",filename);
		return -1;
	}
	
	int indexBlockIndex = findFreeBlock();
	
	if(indexBlockIndex == -1){
		printf("No space for index block!\n");
		printf("File creation failed! file: %s\n",filename);
		return -1;
	}
	
	SetAvaibleBit(indexBlockIndex,1);
	superBlock->freeBlockCount--;
	
	
	struct DirBlock* dirBlock = malloc(sizeof(struct DirBlock));
	read_block(dirBlock,dirBlockIndex[0]);
	
	dirBlock->dirs[dirBlockIndex[1]].avaible = 0;
	memcpy(dirBlock->dirs[dirBlockIndex[1]].fileName,filename,110);
	dirBlock->dirs[dirBlockIndex[1]].fcbBlockIndex[0] = fcbBlockIndex[0];
	dirBlock->dirs[dirBlockIndex[1]].fcbBlockIndex[1] = fcbBlockIndex[1];
	
	write_block(dirBlock,dirBlockIndex[0]);
	superBlock->freeDirCount--;
	
	struct FcbBlock* fcbBlock = malloc(sizeof(struct FcbBlock));
	read_block(fcbBlock,fcbBlockIndex[0]);
	int firstDataBlockIndex = findFreeBlock();
	
	fcbBlock->fcbs[fcbBlockIndex[1]].avaible = 0;
	fcbBlock->fcbs[fcbBlockIndex[1]].indexBlockIndex = indexBlockIndex;
	fcbBlock->fcbs[fcbBlockIndex[1]].fileSize = 0;
	fcbBlock->fcbs[fcbBlockIndex[1]].lasPos[0] = 0;
	fcbBlock->fcbs[fcbBlockIndex[1]].lasPos[1] = 0;
	fcbBlock->fcbs[fcbBlockIndex[1]].mode = -1;
	
	SetAvaibleBit(fcbBlockIndex[0],1);
	
	write_block(fcbBlock,fcbBlockIndex[0]);
	superBlock->freeFcbCount--;
	
	
	
	struct indexBlock* indexBlock = malloc(sizeof(struct indexBlock));
	read_block(indexBlock,indexBlockIndex);
	indexBlock->dataPtrs[0] = firstDataBlockIndex;
	SetAvaibleBit(firstDataBlockIndex,1);
	superBlock->freeBlockCount--;
	
	for(int i = 1; i < 1024; i++){
		indexBlock->dataPtrs[i] = -1;
	}
	
	write_block(indexBlock,indexBlockIndex);
	write_block(superBlock,0);
	
    return 0;
}


int sfs_open(char *file, int mode)
{
	int* fileEntryIndex = FindFileIndexByName(file);
	
	if(fileEntryIndex[0] == -1 || fileEntryIndex[1] == -1){
		printf("sfs_open failed: No such file found! file: %s\n",file);
		return -1;
	}
	
	if(openFileCount >= 128){
		printf("sfs_open failed: Max opened file amount reached! file: %s\n",file);
		return -1;
	}
	
	
	int fd = (fileEntryIndex[0] -5) * 32 + fileEntryIndex[1];
	
	if(openFiles[fd] == 1){
		printf("sfs_open failed: File is already open! file: %s\n fd: %d",file,fd);
		return -1;
	}
	
	openFiles[fd] = 1;
	
	struct DirBlock* dirblock = malloc(sizeof(struct DirBlock));
	read_block(dirblock,fileEntryIndex[0]);
	
	int* fcbIndex = dirblock->dirs[fileEntryIndex[1]].fcbBlockIndex;
	
	struct FcbBlock* fcbblock = malloc(sizeof(struct FcbBlock));
	read_block(fcbblock,fcbIndex[0]);
	fcbblock->fcbs[fcbIndex[1]].mode = mode;
	write_block(fcbblock,fcbIndex[0]);
	
	openFileCount++;
    return fd; 
}

int sfs_close(int fd){
	if(openFiles[fd] == 0){
		printf("sfs_close failed: File is not open! fd: %d\n",fd);
		return -1;
	}
	
	openFiles[fd] = 0;
	openFileCount--;
    return 0; 
}

int sfs_getsize (int  fd)
{
	struct Supernode* superBlock = malloc(sizeof(struct Supernode));
	read_block(superBlock,0);
	
	if(openFiles[fd] == 0){
		printf("sfs_getsize failed: File is not open! fd: %d\n",fd);
		return -1;
	}
	
	struct Supernode* superblock = malloc(sizeof(struct Supernode));
	read_block(superblock,0);
	
	int dirsStartIndex = superblock->dirsStartIndex;
	
	int fileBlockIndex = fd / 32 + dirsStartIndex;
	int fileEntryIndex = fd % 32;
	
	struct DirBlock* dirblock = malloc(sizeof(struct DirBlock));
	read_block(dirblock,fileBlockIndex);
	
	int* fcbIndex = dirblock->dirs[fileEntryIndex].fcbBlockIndex;
	
	struct FcbBlock* fcbblock = malloc(sizeof(struct FcbBlock));
	read_block(fcbblock,fcbIndex[0]);
	
	int filesize = fcbblock->fcbs[fcbIndex[1]].fileSize;
	
    return filesize; 
}

int sfs_read(int fd, void *buf, int n){
	
	if(openFiles[fd] == 0){
		printf("sfs_read failed: File is not open! fd: %d\n",fd);
		return -1;
	}
	
	struct Supernode* superblock = malloc(sizeof(struct Supernode));
	read_block(superblock,0);
	
	int dirsStartIndex = superblock->dirsStartIndex;
	
	int fileBlockIndex = fd / 32 + dirsStartIndex;
	int fileEntryIndex = fd % 32;
	
	struct DirBlock* dirblock = malloc(sizeof(struct DirBlock));
	read_block(dirblock,fileBlockIndex);
	
	int* fcbIndex = dirblock->dirs[fileEntryIndex].fcbBlockIndex;
	
	struct FcbBlock* fcbblock = malloc(sizeof(struct FcbBlock));
	read_block(fcbblock,fcbIndex[0]);
	
	int mode = fcbblock->fcbs[fcbIndex[1]].mode;
	
	if(mode != 0){
		printf("sfs_read failed: file is not in read mode! fd: %d\n",fd);
		return -1;
	}
	
	if(fcbblock->fcbs[fcbIndex[1]].fileSize == 0){
		return 0;
	}
	
	int indexBlockIndex = fcbblock->fcbs[fcbIndex[1]].indexBlockIndex;
	
	struct indexBlock* indexBlock = malloc(sizeof(struct indexBlock));
	read_block(indexBlock,indexBlockIndex);
	
	int bytesRead = 0;
	char* buffer = (char *) buf;
	
	for(int i = 0; i < 1024; i++){
		if(indexBlock->dataPtrs[i] == -1){
			break;
		}
		
		struct DataBlock* datablock = malloc(sizeof(struct DataBlock));
		for(int k = 0; k < 4096; k++){
			read_block(datablock,indexBlock->dataPtrs[i]);
			buffer[k]= datablock->data[k];
			bytesRead++;
			
			if(bytesRead >= n){
				return bytesRead;
			}
		}
	}
	
    return bytesRead; 
}


int sfs_append(int fd, void *buf, int n)
{
	if(openFiles[fd] == 0){
		printf("sfs_append failed: File is not open! fd: %d\n",fd);
		return -1;
	}
	
	struct Supernode* superblock = malloc(sizeof(struct Supernode));
	read_block(superblock,0);
	
	int dirsStartIndex = superblock->dirsStartIndex;
	
	int fileBlockIndex = fd / 32 + dirsStartIndex;
	int fileEntryIndex = fd % 32;
	
	struct DirBlock* dirblock = malloc(sizeof(struct DirBlock));
	read_block(dirblock,fileBlockIndex);
	
	int* fcbIndex = dirblock->dirs[fileEntryIndex].fcbBlockIndex;
	
	struct FcbBlock* fcbblock = malloc(sizeof(struct FcbBlock));
	read_block(fcbblock,fcbIndex[0]);
	
	int mode = fcbblock->fcbs[fcbIndex[1]].mode;
	
	if(mode != 1){
		printf("sfs_append failed: file is not in append mode! fd: %d\n",fd);
		return -1;
	}
	
	int indexBlockIndex = fcbblock->fcbs[fcbIndex[1]].indexBlockIndex;
	int* lasPos = fcbblock->fcbs[fcbIndex[1]].lasPos;
	int a = lasPos[0];
	int b = lasPos[1];
	
	struct indexBlock* indexBlock = malloc(sizeof(struct indexBlock));
	read_block(indexBlock,indexBlockIndex);
	int bytesWritten = 0;
	char * buffer = (char *)buf;
	
	for(int i = a; i < 1024; i++){
		read_block(indexBlock,indexBlockIndex);
		
		struct DataBlock* datablock = malloc(sizeof(struct DataBlock));
		for(int k = b; k < 4096; k++){
			read_block(datablock,indexBlock->dataPtrs[i]);
			datablock->data[k] = buffer[k];
			bytesWritten++;
			write_block(datablock,indexBlock->dataPtrs[i]);
			
			if(bytesWritten >= n){
				fcbblock->fcbs[fcbIndex[1]].lasPos[0] = i;
				fcbblock->fcbs[fcbIndex[1]].lasPos[1] = k + 1;
				break;
			}
		}
		
		if(bytesWritten >= n){
				break;
		}	
		
		//Alloc new data block
		int nextDataBlockIndex = findFreeBlock();
		struct DataBlock* newdatablock = malloc(sizeof(struct DataBlock));
		write_block(newdatablock,nextDataBlockIndex);
		superblock->freeBlockCount--;
		
		int freeIndex = 0;
			
		for(int i = 0; i < 1024; i++){
			if(indexBlock->dataPtrs[i] == -1){
				freeIndex = i;
				break;
			}
		}
		
		indexBlock->dataPtrs[freeIndex] = nextDataBlockIndex;
		write_block(indexBlock,indexBlockIndex);
		fcbblock->fcbs[fcbIndex[1]].lasPos[1] = 0;
		b=0;
		SetAvaibleBit(nextDataBlockIndex, 1);
	}
	
	fcbblock->fcbs[fcbIndex[1]].fileSize = fcbblock->fcbs[fcbIndex[1]].fileSize + bytesWritten;
	write_block(fcbblock,fcbIndex[0]);
	write_block(superblock,0);
	
    return bytesWritten; 
}

int sfs_delete(char *filename)
{
	struct Supernode* superblock = malloc(sizeof(struct Supernode));
	read_block(superblock,0);
	
	int* fileEntryIndex = FindFileIndexByName(filename);
	
	if(fileEntryIndex[0] == -1 || fileEntryIndex[1] == -1){
		printf("sfs_delete failed: No such file found! file: %s\n",filename);
		return -1;
	}
	
	struct DirBlock* dirblock = malloc(sizeof(struct DirBlock));
	read_block(dirblock,fileEntryIndex[0]);
	dirblock->dirs[fileEntryIndex[1]].avaible = 1;
	memcpy(dirblock->dirs[fileEntryIndex[1]].fileName,"",1);
	write_block(dirblock,fileEntryIndex[0]);
	
	int* fcbIndex = dirblock->dirs[fileEntryIndex[1]].fcbBlockIndex;
	
	struct FcbBlock* fcbblock = malloc(sizeof(struct FcbBlock));
	read_block(fcbblock,fcbIndex[0]);
	fcbblock->fcbs[fcbIndex[1]].avaible = 1;
	fcbblock->fcbs[fcbIndex[1]].fileSize = 0;
	fcbblock->fcbs[fcbIndex[1]].lasPos[0] = 0;
	fcbblock->fcbs[fcbIndex[1]].lasPos[1] = 1;
	fcbblock->fcbs[fcbIndex[1]].mode = -1;
	write_block(fcbblock,fcbIndex[0]);
	
	int indexBlockIndex = fcbblock->fcbs[fcbIndex[1]].indexBlockIndex;
	
	struct indexBlock* indexBlock = malloc(sizeof(struct indexBlock));
	read_block(indexBlock,indexBlockIndex);
	SetAvaibleBit(indexBlockIndex,0);
	
	for(int i = 0; i < 1024; i++){
		if(indexBlock->dataPtrs[i] == -1){
			continue;
		}
		
		struct DataBlock* datablock = malloc(sizeof(struct DataBlock));
		read_block(datablock,indexBlock->dataPtrs[i]);
		
		for(int k = 0; k <= 4096; k++){
			datablock->data[k] = 0;
		}
		
		write_block(datablock,indexBlock->dataPtrs[i]);
		SetAvaibleBit(indexBlock->dataPtrs[i],0);
	}
	
	write_block(indexBlock,indexBlockIndex);
	write_block(superblock,0);	
    return 0; 
}


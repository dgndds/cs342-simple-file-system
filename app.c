#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "simplefs.h"

int main(int argc, char **argv)
{
    int ret;
    int fd1, fd2, fd3,fd4,fd5; 
    int i;
    char c; 
    char buffer[1024];
    //char buffer2[8] = {50, 50, 50, 50, 50, 50, 50, 50};
    int size1, size2, size3, size4, size5;
    char vdiskname[200]; 

    printf ("started\n");

    if (argc != 2) {
        printf ("usage: app  <vdiskname>\n");
        exit(0);
    }
    strcpy (vdiskname, argv[1]); 
    
    ret = sfs_mount (vdiskname);
    if (ret != 0) {
        printf ("could not mount \n");
        exit (1);
    }

    printf ("creating files\n");
    
    clock_t begin = clock(); 
    sfs_create ("file1.bin");
    clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to create file1 is %f milliseconds\n", time_spent*1000);
    
    begin = clock();
    sfs_create ("file2.bin");
   	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to create file2 is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    sfs_create ("file3.bin");
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to create file3 is %f milliseconds\n", time_spent*1000);
	
    begin = clock();
    sfs_create ("file4.bin");
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to create file4 is %f milliseconds\n", time_spent*1000);
	
    begin = clock();
    sfs_create ("file5.bin");
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to create file5 is %f milliseconds\n", time_spent*1000);
	
	printf("=========================================================\n");
	
    fd1 = sfs_open ("file1.bin", MODE_APPEND);
    fd2 = sfs_open ("file2.bin", MODE_APPEND);
    fd3 = sfs_open ("file3.bin", MODE_APPEND);
    fd4 = sfs_open ("file4.bin", MODE_APPEND);
    fd5 = sfs_open ("file5.bin", MODE_APPEND);
    
    begin = clock();
    for (i = 0; i < 10000; ++i) {
        buffer[0] =   (char) 65;
        sfs_append (fd1, (void *) buffer, 1);
    }
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to write 10000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < 15000; ++i) {
        buffer[0] =   (char) 65;
        sfs_append (fd2, (void *) buffer, 1);
    }
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to write 15000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < 20000; ++i) {
        buffer[0] =   (char) 65;
        sfs_append (fd3, (void *) buffer, 1);
    }
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to write 20000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < 25000; ++i) {
        buffer[0] =   (char) 65;
        sfs_append (fd4, (void *) buffer, 1);
    }
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to write 25000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < 30000; ++i) {
        buffer[0] =   (char) 65;
        sfs_append (fd5, (void *) buffer, 1);
    }
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to write 30000 bytes is %f milliseconds\n", time_spent*1000);
	
    /*for (i = 0; i < 10000; ++i) {
        buffer[0] = (char) 65;
        buffer[1] = (char) 66;
        buffer[2] = (char) 67;
        buffer[3] = (char) 68;
        sfs_append(fd2, (void *) buffer, 4);
    }*/
    
    sfs_close(fd1);
    sfs_close(fd2);
    sfs_close(fd3);
    sfs_close(fd4);
    sfs_close(fd5);

    /*fd = sfs_open("file3.bin", MODE_APPEND);
    for (i = 0; i < 10000; ++i) {
        memcpy (buffer, buffer2, 8); // just to show memcpy
        sfs_append(fd, (void *) buffer, 8);
    }
    sfs_close (fd);*/
    
    printf("=========================================================\n");

    fd1 = sfs_open("file1.bin", MODE_READ);
    fd2 = sfs_open("file2.bin", MODE_READ);
    fd3 = sfs_open("file3.bin", MODE_READ);
    fd4 = sfs_open("file4.bin", MODE_READ);
    fd5 = sfs_open("file5.bin", MODE_READ);
   	
   	size1 = sfs_getsize(fd1);
   	size2 = sfs_getsize(fd2);
   	size3 = sfs_getsize(fd3);
   	size4 = sfs_getsize(fd4);
   	size5 = sfs_getsize(fd5);
    
    begin = clock();
    for (i = 0; i < size1; ++i) {
        sfs_read (fd1, (void *) buffer, 1);
        c = (char) buffer[0];
        c = c + 1;
    }
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to read 10000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < size2; ++i) {
        sfs_read (fd2, (void *) buffer, 1);
        c = (char) buffer[0];
        c = c + 1;
    }
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to read 15000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < size3; ++i) {
        sfs_read (fd3, (void *) buffer, 1);
        c = (char) buffer[0];
        c = c + 1;
    }
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to read 20000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < size4; ++i) {
        sfs_read (fd4, (void *) buffer, 1);
        c = (char) buffer[0];
        c = c + 1;
    }
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to read 25000 bytes is %f milliseconds\n", time_spent*1000);
	
	begin = clock();
    for (i = 0; i < size5; ++i) {
        sfs_read (fd5, (void *) buffer, 1);
        c = (char) buffer[0];
        c = c + 1;
    }
    end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("The elapsed time to read 30000 bytes is %f milliseconds\n", time_spent*1000);
    
   	sfs_close(fd1);
    sfs_close(fd2);
    sfs_close(fd3);
    sfs_close(fd4);
    sfs_close(fd5);
    
    sfs_delete("file3.bin");
    ret = sfs_umount();
}

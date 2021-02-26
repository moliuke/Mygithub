#include<stdio.h>
#include<fcntl.h>
#include <stdlib.h>

mode_t get_filepemis(char *filename)
{
	mode_t mode,retmode;		//×ÜÈ¨ÏÞ  
    struct stat buf;    //lstat();  
	if(lstat(filename, &buf) == -1)	
	{  
		printf("line %d ", __LINE__);	
		perror("lstat"); 
		//kill_app();
		//exit(1);  
	}  
	mode = buf.st_mode; 

	if(mode & S_IRUSR)retmode |= S_IRUSR;
	if(mode & S_IWUSR)retmode |= S_IWUSR;
	if(mode & S_IXUSR)retmode |= S_IXUSR;

	if(mode & S_IRGRP)retmode |= S_IRGRP;
	if(mode & S_IWGRP)retmode |= S_IWGRP;
	if(mode & S_IXGRP)retmode |= S_IXGRP;

	if(mode & S_IROTH)retmode |= S_IROTH;
	if(mode & S_IWOTH)retmode |= S_IWOTH;
	if(mode & S_IXOTH)retmode |= S_IXOTH;
		
	return retmode;
}

void set_filepermis(char *filename,mode_t mode)
{
	chmod(filename,mode);
}


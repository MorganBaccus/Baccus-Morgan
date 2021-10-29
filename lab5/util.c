// util.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
#include "util.h"

// globalvariables are defined in main.c

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];


int get_block(int dev, int blk, char *buf)
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	write(fd, buf, BLKSIZE);
}   

int tokenize1(char *pathname)
{
	// tokenize the pathname into n number of components
	int i=0;
	if(pathname[0] == '/')
		name[i++] = "/";

	name[i++] = strtok(pathname, "/");
	while(name[i++] = strtok(0, "/"));
	return i - 1;
}


MINODE *iget(int dev, int ino)
{
  // Code in Chapter 11.7.2

    MINODE *mip;
    INODE *ip;
    int i, block,offset;
    char buf[BLKSIZE];
    
    for(i = 0; i < NMINODE;i++)
    {
        mip = &minode[i];
        if(mip->refCount && (mip->dev==dev) && (mip->ino== ino))
        {
            mip->refCount++;
            return mip;
        }

		// else the needed INODE=(dev,ino) not in memory
    }

    mip->dev = dev;
    mip->ino = ino;
    block = (ino - 1) / 8 + inode_start;
    offset = (ino - 1) % 8;
    get_block(dev, block, buf);
    ip = (INODE *)buf + offset;
    mip->INODE = *ip;
    
    mip->refCount = 1;
    mip->mounted = 0;
    mip->dirty = 0;
    mip->mptr = 0;
    return mip;
}

void iput(MINODE *mip)
{
  // Code in Chapter 11.7.2

    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];  
    
    if( mip == 0) 
      {return -1;}
    mip->refCount--;
    if( mip->refCount > 0 || mip->dirty == 0)
      {return -1;}

    block = (mip->ino -1) / 8 + inode_start;
    offset = (mip->ino -1) % 8;

    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset;
    *ip = mip->INODE;
    put_block(mip->dev, block, buf);

    return 0;
} 


int search(MINODE *mip, char *name)
{
  // Code in Chapter 11.7.2

    int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;

    for (i = 0; i < 12; i++)
    {
      if (mip->INODE.i_block[i] == 0)
      {
        return -1;
      }
      get_block(mip->dev, mip->INODE.i_block[i],sbuf);
      dp = (DIR *)sbuf;
      cp = sbuf;

      while(cp<sbuf + BLKSIZE){
        strncpy(temp,dp->name,dp->name_len);
        temp[dp->name_len]=0;
        printf("%8d%8d%8u %s\n",dp->inode, dp->rec_len, dp->name_len, temp);
        if(strcmp(name,temp)==0)
        {
          printf("found %s : inumber = %d\n", name, dp->inode);
          return dp->inode;
        }
        cp += dp->rec_len;
        dp = (DIR *)cp;
      }
    }
    return 0;
}

int getino1(char *pathname)
{ 
	char temp[256];
	MINODE *mip;
	int n, i, ino;

	strcpy(temp, pathname);
	n = tokenize1(temp);

	if(!strcmp(name[0], "/"))
	{
		i = 1;
		mip = root;
	}
	else
	{
		i = 0;
		mip = running->cwd;
	}

	while(i < n)
	{
		if(!S_ISDIR(mip->INODE.i_mode))
		{
			printf("%s is not a directory!", name[i - 1]);
			return 0;
		}
		ino = search(mip, name[i]);
		if(!ino)
		{
			printf("%s not found\n", name[i]);
			return 0;
		}
		
		mip = iget(dev, ino);
		i++;
	}
	
	return ino;
}

int findmyname(MINODE *parent, u32 myino, char *myname)
{
  int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    MINODE *mip = parent;

    for (i = 0; i < 12; i++)
    {
        if (mip->INODE.i_block[i] == 0)
            return -1;
        get_block(mip->dev, mip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            if (dp->inode == myino)
            {
                strncpy(myname, dp->name, dp->name_len);
                myname[dp->name_len] = 0;
                return 0;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return -1; 
}
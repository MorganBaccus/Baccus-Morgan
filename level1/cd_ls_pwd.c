#include "type.h"


int my_chdir(char *pathname)   
{
  printf("chdir %s\n", pathname);
  

  int ino = getino(pathname);
  if(ino == -1)
  {
    printf("Can't find ino\n");
    return 0;
  }

  MINODE *mip = iget(dev,ino);
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Not a directory\n");
    return 0;
  }


  iput(running->cwd);
  running->cwd = mip; // changed cwd to mip
  return 1;
}

int ls_file(MINODE *mip, char *name)
{
 

  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";
  int i;
  char ftime[256];
  u16 mode = mip->INODE.i_mode;

  if (S_ISREG(mode)){
    printf("%c",'-');
  }

  if (S_ISDIR(mode)){
    printf("%c",'d');
  }

  if (S_ISLNK(mode)){
    printf("%c",'l');
  }

  for (i = 8; i >= 0; i--){
    if(mode & (1 << i)){ 
      printf("%c", t1[i]);
    }
    else{
      printf("%c", t2[i]);
    }
  }

  printf("%4d ", mip->INODE.i_links_count); // link count
  printf("%4d ", mip->INODE.i_gid); // gid
  printf("%4d ", mip->INODE.i_uid); // uid
  printf("%8d ", mip->INODE.i_size); // full size

  
  strcpy(ftime, ctime((time_t *)&(mip->INODE.i_mtime))); // time printed in calandar form
  ftime[strlen(ftime) - 1] = 0;
  printf("%s ", ftime);

  // print name
  printf("%s", basename(name)); // p basename file

  if (S_ISLNK(mode))
    {
      
      printf(" -> %s", (char *)mip->INODE.i_block); //p link name
    }
    printf("\n");
    return 0;
}

int ls_dir(MINODE *mip)
{
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    char ftime[256];
    MINODE *tmip;

    u16 mode = mip->INODE.i_mode;

    char buf[BLKSIZE], temp[BLKSIZE];
    DIR *dp;
    char *cp;

                     // Assume DIR has only one data block i_block[0]
    get_block(dev, mip->INODE.i_block[0], buf);
    dp = (DIR*)buf;
    cp = buf;

    while (cp < buf + BLKSIZE)
    {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        tmip = iget(dev, dp->inode);
        ls_file(tmip, temp);
        tmip->dirty =1;
        iput(tmip);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    printf("\n");
    return 0;
}

int my_ls(char *pathname)  
{
   MINODE *mip;
  u16 mode;
  int dev, ino;

  if (pathname[0] == 0)
    ls_dir(running->cwd);
  else{
    dev = root->dev;
    ino = getino(pathname);
    if (ino==0){
      printf("no such file %s\n", pathname);
      return -1;
    }
    mip = iget(dev, ino);
    mode = mip->INODE.i_mode;
    if (!S_ISDIR(mode))
      ls_file(mip, (char *)basename(pathname));
    else
      ls_dir(mip);
    iput(mip);
        }
}


char *my_pwd(MINODE *wd)
{
  if (wd == root){
    printf("CWD= /\n");
  
  }

  else{
    rpwd(wd);
    printf("\n");
  } 
}



void rpwd(MINODE *wd)
{
  int my_ino, parent_ino;
  char buf[BLKSIZE], my_name[256]; 

  if( wd == root)
  {return;}

  get_block(dev,wd->INODE.i_block[0], buf);
  
  parent_ino = findino(wd, &my_ino);
  MINODE *pip = iget(dev,parent_ino);

  findmyname(pip,my_ino,my_name);
  rpwd(pip);
  pip->dirty = 1;
  iput(pip);
  printf("/%s", my_name);
}
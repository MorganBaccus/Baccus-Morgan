/*********** util.c file ****************/

char gline[128]; 


int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   
int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   
MTABLE *getmtable(int dev)
{
    MTABLE *nm;
    for(int i = 0; i < NMTABLE; i++)
    {
        nm = &mtable[i];
        if (nm->dev == dev)
        {
            return nm;
        }
    }
    return 0;
}
int tokenize(char *pathname)
{
  // copy pathname into gpath[]; tokenize it into name[0] to name[n-1]
  // Code in Chapter 11.7.2 

  char *s;
  strcpy(gpath, pathname);
  n = 0;
  s = strtok(gpath, "/");
  while(s){
    name[n++] = s;
    s = strtok(0,"/");
  }
  return n;
}


MINODE *iget(int dev, int ino)
{
  // return minode pointer of loaded INODE=(dev, ino)
  // returned minode is unique, only one copy of the INODE exists in mem
  // Code in Chapter 11.7.2

    MINODE *mip;
    MTABLE *mp;
    INODE *ip;
    int i, block,offset;
    char buf[BLKSIZE];
    
    // search in-memory minodes first
    for(i = 0; i < NMINODE;i++)
    {
        mip = &minode[i]; // global minode
        if(mip->refCount && (mip->dev==dev) && (mip->ino== ino))
        {
            mip->refCount++;
            return mip;
        }
    }

    // else the needed INODE=(dev,ino) not in memory
    mip = mialloc();                     // allocate a FREE minode
    mip->dev = dev;                      // assign to (dev, ino)
    mip->ino = ino;                      // 
    block = (ino - 1) / 8 + inode_start; // disk block containing this inode
    offset = (ino - 1) % 8; // which inode in this block
    get_block(dev, block, buf); // read the block
    ip = (INODE *)buf + offset;
    mip->INODE = *ip; // copy inode to minode.INODE
    
    // initialize minode
    mip->refCount = 1;
    mip->mounted = 0;
    mip->dirty = 0;
    mip->mptr = 0;
    return mip;
}

void iput(MINODE *mip)
{
  // dispose of minode pointed by mip
  // refounct reprensets the number of users using the minode, decrements by 1
  // Code in Chapter 11.7.2

    INODE *ip;
    MTABLE *mp;
    int i, block, offset;
    char buf[BLKSIZE];  
    
    if( mip == 0) 
      {return -1;}
    mip->refCount--;
    if( mip->refCount > 0 || mip->dirty == 0) // still has a user or no need to write back
      {return -1;}

    // write INODE back to disk
    block = (mip->ino -1) / 8 + inode_start; //  iblock
    offset = (mip->ino -1) % 8;

    // get block containing this inode
    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset; // ip points at INODE
    *ip = mip->INODE; // copy INODE to inode in block
    put_block(mip->dev, block, buf); // write back to disk
    //midalloc(mip); // mip->refCount = 0;

    return 0;

} 

int search(MINODE *mip, char *name)
{
  // search for name in (DIRECT) data blocks of mip->INODE
  // return its ino
  // Code in Chapter 11.7.2

    int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;

    for (i = 0; i < 12; i++)
    {
      if (mip->INODE.i_block[i] == 0) // looking for DIR direct blocks only
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
        if(strcmp(name,temp)==0) // error here
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

int getino(char *pathname) // tree traversal alg, returns (dev,ino)
{
  // return ino of pathname
  // Code in Chapter 11.7.2

    char buf[BLKSIZE];
    INODE *ip;
    MTABLE *mp;
    MINODE *mip;
    int i, ino;
    
    if (strcmp(pathname, "/") == 0)
        return 2; // return root ino = 2
    if (pathname[0] == '/') {
        //mip = root; // if absolute pathname: start from root
        dev = root->dev;
        ino = root->ino;
    } 
    else
    {
        //mip = running->cwd; // if relative pathname: start from cwd
        dev = running->cwd->dev;
        ino = running->cwd->ino;
    }

    mip = iget(dev, ino);


    tokenize(pathname); // assume: name [ ], n are globals

    for (i = 0; i < n; i++){ // search for each component string
      if ( !S_ISDIR(mip->INODE.i_mode)){ // check DIR type
        printf("%s is not a directory\n", name[i]);
        iput(mip);
        return 0;
      }
    
      printf("%s\n",name[i]);
      ino = search(mip, name[i]);
      if (!ino){
        printf("no such component name %s exists\n", name[i]);
        iput(mip);
        return -1;
      }

      iput(mip); // release current minode
      mip = iget(dev, ino); // switch to new minode

     if(ino == 2 && dev != rootdev) //if we are at ino of root, but our device is not the initial device STEP OFF the mount
      {
       if(i > 0){
          printf("Crossing mount points\n");
          mp = getmtable(dev);
          mip = mp->mntDirPtr;
          dev = mip->dev;
        }
        i++;
      }
      else if( mip->mounted)
      {
        //Downward since is mounted
        mp = mip->mptr;
        if( dev != mp->dev) // device does not match cross mount points upwards
        {
          printf("Cross mount points\n");
			    dev = mp->dev; // update global device
			    mip = iget(dev, 2); // get root of new mount
			    ino = 2; //ino = rootino (2)
        }
      }
  }
  iput(mip);
  return ino;
    
}

int findmyname(MINODE *parent, u32 myino, char *myname) // rewrite search func, return myname
{
  // WRITE YOUR code here:
  int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    MINODE *mip = parent;

    for (i = 0; i < 12; i++)
    { //Search DIR direct blocks only
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
                strncpy(myname, dp->name, dp->name_len); // copy its name STRING to myname[ ];
                myname[dp->name_len] = 0;
                return 0;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return -1; 
}

// trying to find the parent, similar to LS
int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  // mip->a DIR minode. Write YOUR code to get mino=ino of .
  //                                         return ino of ..
  char buf[BLKSIZE]; 
  char *cp;
  DIR *dp;

  get_block(mip->dev,mip->INODE.i_block[0],buf); // gets block of memory into buf, entries in mem
  cp = buf;
  dp = (DIR *)buf; // cur directory

  *myino = dp->inode;
  cp += dp->rec_len; // advances to parent by x amount
  dp = (DIR *) cp; // second entry, parent

  return dp->inode;
  
}


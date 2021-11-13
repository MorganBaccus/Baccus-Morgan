
int make_dir(char *pathname)
{
    MINODE *start;

    char path1[256], path2[256];
    strcpy(path1, pathname);
    strcpy(path2, pathname);

    if(pathname[0] == '/') 
    {
        start = root;
        dev = root->dev; 
    }

    else{ 
        start = running->cwd;
        dev = running->cwd->dev;
    }

    char *parent = dirname(path1); 
    char *child = basename(path2); 

    MINODE *pino  = getino(parent); 
    MINODE *pip   = iget(dev, pino); //returns minode pointer, inode of parent

    if(!S_ISDIR(pip->INODE.i_mode)) //checks for directory
    {
        printf("ERROR: %s is not a directory\n", parent);
        return -1;
    }
    // verify that the child does not exist in the parent directory
    if(!search(pip,child))
    {
        printf("ERROR: child %s already exists under parent %s", child, parent);
        return -1;
    }

    mymkdir(pip, child);           //creates dir
    pip->INODE.i_links_count++;    // increment linkc
    pip->INODE.i_atime = time(0L); 
    pip->dirty = 1;                
    iput(pip);                     //updates block
    return 0;
}

int mymkdir(MINODE *pip, char *name) 
{
    MINODE *mip;
    char *buf[BLKSIZE], *cp;

    int ino = ialloc(dev); // allocate a new inode
    printf("INO %d\n",ino);
    int bno = balloc(dev); // allocate a new block
    printf("BNO %d\n",bno);

    mip = iget(dev, ino);  //load the inode into a minode[] (in order to write contents to the INODE in memory.
    INODE *ip = &mip->INODE;
    bzero(ip,sizeof(INODE));
    *ip = (INODE){
    .i_mode = 040755,       
    .i_uid = running->uid,    
    .i_gid = running->gid,    
    .i_size = BLKSIZE,       
    .i_links_count = 2,      
    .i_atime = time(0L),      
    .i_ctime = time(0L),      
    .i_mtime = time(0L),      
    .i_blocks = 2,            
    .i_block[0] = bno       
    };
    for(int i = 1; i < 15; i ++)
    {
        ip->i_block[i] = 0;
    }

    mip->dirty = 1;               
    iput(mip);                    

    
    bzero(buf, BLKSIZE);
    get_block(dev, bno, buf); 
    dp = (DIR *)buf;
    cp = buf;

    printf("Create . and .. in %s\n", name);

    dp->inode = ino;   
    dp->rec_len = 12;  
    dp->name_len = 1;  
    dp->name[0] = '.'; 

    cp += dp->rec_len; 
    dp = (DIR *)cp;

    dp->inode = pip->ino;       
    dp->rec_len = BLKSIZE - 12; 
    dp->name_len = 2;           
    dp->name[0] = '.';
    dp->name[1] = '.';

    put_block(dev, bno, buf); 

    enter_name(pip, ino, name); 

    return 0;

}


int enter_name(MINODE *pip, int myino, char *myname)
{
    char buf[BLKSIZE], *cp;
    int bno; // our block
    INODE *ip;
    DIR *dp;

    int need_len = 4 * ((8 + strlen(myname) + 3) / 4);

    ip = &pip->INODE; // get the inode

    for (int i = 0; i < 12; i++)
    {

        if (ip->i_block[i] == 0)
        {
            break;
        }

        bno = ip->i_block[i];
        get_block(pip->dev, ip->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while (cp + dp->rec_len < buf + BLKSIZE) // last entry of block (until)
        {
            printf("%s\n", dp->name);
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        // at last entry
        int ideal_len = 4 * ((8 + dp->name_len + 3) / 4); 
        int remainder = dp->rec_len - ideal_len;  

        if (remainder >= need_len)
        {                            
            dp->rec_len = ideal_len; 
            cp += dp->rec_len;       //Uses both lengths to create ideal length
            dp = (DIR *)cp;          // point to new open entry space

            dp->inode = myino;             
            strcpy(dp->name, myname);      
            dp->name_len = strlen(myname); 
            dp->rec_len = remainder;       

            put_block(dev, bno, buf); 
            return 0;
        }
        else
        {                         
            ip->i_size = BLKSIZE; 
            bno = balloc(dev);    
            ip->i_block[i] = bno; 
            pip->dirty = 1;       

            get_block(dev, bno, buf); 
            dp = (DIR *)buf;
            cp = buf;

            dp->name_len = strlen(myname); 
            strcpy(dp->name, myname);      
            dp->inode = myino;             
            dp->rec_len = BLKSIZE;        

            put_block(dev, bno, buf); 
            return 1;
        }
    }
}




int creat_file(char *pathname)
{
    MINODE *start;
    char rel_path[256], abs_path[256];

    if(pathname[0] == '/') 
    {
        start = root;
        root = root->dev; 
    }

    else{ 
        start = running->cwd;
        root = running->cwd->dev;
        
    }

    char *parent = dirname(pathname); 
    char *child = basename(pathname); 

    MINODE *pino  = getino(parent); 
    MINODE *pip   = iget(dev, pino);

    if(!S_ISDIR(pip->INODE.i_mode))
    {
        printf("ERROR: %s is not a directory\n", parent);
        return -1;
    }
    
    if(!search(pip,child))
    {
        printf("ERROR: child %s already exists under parent %s", child, parent);     
        return -1;
    }

    my_creat(pip, child);          
    pip->INODE.i_atime = time(0L); 
    pip->dirty = 1;                
    iput(pip);                     
    return 0;
}





int my_creat(MINODE *pip, char *name)
{
    MINODE *mip;
    char *buf[BLKSIZE], *cp;
    DIR *dp;

    int ino = ialloc(dev); 
    int bno = balloc(dev); 

    printf("ino: %d, bno: %d\n", ino, bno);

    mip = iget(dev, ino); 

    INODE *ip = &mip->INODE;
    mip->INODE.i_mode = 0x81A4;   
    ip->i_uid = running->uid; 
    ip->i_gid = running->gid; 
    ip->i_size = 0;    
    ip->i_links_count = 1;    
    ip->i_atime = time(0L);   
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 2;            
    ip->i_block[0] = 0;          
    for (int i = 1; i < 15; i++) 
    {
        ip->i_block[i] = 0;
    }

    mip->dirty = 1; 
    iput(mip);     

    enter_name(pip, ino, name); 

    return 0;
}
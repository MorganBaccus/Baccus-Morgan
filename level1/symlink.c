

int my_symlink(char *old_file, char *new_file)
{
    MINODE *mip;
    if (old_file[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    // checks to see if the old file exists
    int old_ino = getino(old_file);

    if(old_ino ==0)
    {
        printf("%s exists\n",old_file);
    }

    if (new_file[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    creat_file(new_file);
    //checks to see if the new_file doesn't exist
    int new_ino = getino(new_file);
    if (new_ino == -1) {
        printf("%s does not exist\n", new_file);
        return -1;
    }

    mip = iget(dev, new_ino);
    mip->INODE.i_mode = 0xA1FF;
    mip->dirty = 1;

    strncpy(mip->INODE.i_block, old_file, 84);

    mip->INODE.i_size = strlen(old_file) + 1; 

    mip->dirty = 1;
    iput(mip);


}

int my_readlink(char *file)
{
    MINODE *mip;
    char*buf[BLKSIZE];
    int ino = getino(file);
    mip = iget(dev, ino);

    INODE *ip = &mip->INODE;
    if(!S_ISLNK(ip->i_mode))
    {
        printf("This target file is not a SLINK file\n");
    }

    strncpy(mip->INODE.i_block, file, buf);
    return strlen((char*)mip->INODE.i_block);

}
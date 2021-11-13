





int link_wrapper(char *old, char *new) {
    printf("old = %s\nnew = %s\n", old, new);   //Delimit through space
    my_link(old, new);
}

int my_link(char *oldname, char *newname)
{
    int old_ino, new_ino;
    MINODE* old_mip, new_mip;

    if (oldname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    old_ino = getino(oldname);
    old_mip = iget(oldname,old_ino);

    if (S_ISDIR(old_mip->INODE.i_mode)) {
        printf("Directory is not allowed\n");
        return -1;
    }

    new_ino = getino(newname);

    if(new_ino != -1)
    {
        printf("File already exists\n");
        return -1;
    }
    char parent[256], child[256];           //newF same INO
    strcpy(child, basename(newname));
    strcpy(parent, dirname(newname));

    int pino = getino(parent);
    MINODE *pmip = iget(dev,pino);
    enter_name(pmip,old_ino,child);         //new parent DIR same INO

    old_mip->INODE.i_links_count++;
    old_mip->dirty=1;
    iput(old_mip);
    iput(pmip);
}



int my_unlink(char *filename)
{
    int ino;
    MINODE *mip;

    ino = getino(filename);
    mip = iget(dev,ino);
    
    
     if (S_ISDIR(mip->INODE.i_mode)) {
        printf("dir cannot be link; cannot unlink %s\n", filename);
        return -1;
    }   
    char parent[256], child[256];
    strcpy(parent, dirname(filename));
    strcpy(child, basename(filename));

    int pino = getino(parent);
    MINODE *pmip = iget(dev,pino);
    rm_child(pmip, child);
    pmip->dirty =1;
    iput(pmip);

    mip->INODE.i_links_count--;             //deincrement link count
    if(mip->INODE.i_links_count > 0)
        mip->dirty = 1;
    else{
        inode_truncate(mip);  //for deallocating
    }

    iput(mip); //delete child
    
}




#include "type.h"

// globals
MINODE minode[NMINODE];
MINODE *root;
MTABLE mtable[NMTABLE];
PROC   proc[NPROC], *running;


char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev,rootdev;
int nblocks, ninodes, bmap, imap, inode_start;

//mount-root------------------------------------------------------------
char *my_pwd(MINODE *wd);
void rpwd(MINODE *wd);
int init();
int mount_root();
int quit();
int my_chdir(char *pathname);
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *mip);
int my_ls(char *pathname);
MINODE *mialloc();
int midalloc(MINODE *mip);
MINODE *iget(int dev, int ino);
//mount-root------------------------------------------------------------



//mkdir------------------------------------------------------------
int make_dir(char *pathname);
int mymkdir(MINODE *pip, char *name);
int enter_name(MINODE *pip, int myino, char *myname);
int tst_bit(char *buf, int bitnum);
int set_bit(char *buf, int bitnum);
int ialloc(int dev);
int balloc(int dev);
int decFreeBlocks(int dev);
//mkdir------------------------------------------------------------


//creat------------------------------------------------------------
int my_creat(MINODE *pip, char *name);
int creat_file(char *pathname);
//creat------------------------------------------------------------

#include "util.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "cd_ls_pwd.c"

int init() // Initialize all global data structures and let running point at PROC[0], uid = 0:
{
  int i, j;
  MINODE *mip;
  PROC   *p;
  MTABLE *mptr;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    p->status = FREE;
  }

  for (i = 0; i < NMTABLE; i++)
    {
        mtable[i].dev = 0;
    }

   
}

MINODE *mialloc()
{
  int i;
  for (i=0; i<NMINODE; i++){
    MINODE *mp = &minode[i];
    if (mp->refCount == 0){
      mp->refCount = 1;
      return mp;
    }
  }
    printf("FS panic: out of minodes\n");
    return 0;
}
int midalloc(MINODE *mip) // release a used minode
{
  mip->refCount = 0;
}


// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "diskimage";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  char line[128], cmd[32], pathname[128],pathname_two[128];

  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }
  dev = rootdev = fd;   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  while(1){
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s", cmd, pathname);
    printf("cmd=%s pathname=%s\n", cmd, pathname);
  
    if (!strcmp(cmd, "ls"))
        my_ls(pathname);
    if (!strcmp(cmd, "cd"))
        my_chdir(pathname);
    if (!strcmp(cmd, "pwd"))
        my_pwd(running->cwd);
    if (!strcmp(cmd, "quit") ||!strcmp(cmd, "q"))
        quit();
    if (!strcmp(cmd, "mkdir"))
        make_dir(pathname);
    if (!strcmp(cmd, "creat"))
          creat_file(pathname);
    if (!strcmp(cmd, "rmdir"))
          myrmdir(pathname); 
    if (!strcmp(cmd, "link"))
        {
          sscanf(line, "%s %s %s", cmd, pathname, pathname_two);
          link_wrapper(pathname, pathname_two);
        }
    if (!strcmp(cmd, "unlink"))
        my_unlink(pathname); 
     if (!strcmp(cmd, "symlink")) {
          sscanf(line, "%s %s %s", cmd, pathname, pathname_two);
          my_symlink(pathname, pathname_two);       
    }

  
  }
}

int quit()
{
  int i;
  MINODE *mip;
  
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}




int tst_bit(char *buf, int bit)
{
  return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit){
  return buf[bit/8] |= (1 << (bit % 8));
}


int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for(i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
        put_block(dev, imap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        return i+1;
    }
  }
  return 0;
}
int decFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}
// need to write a balloc(Dev) func allocates a free disk block (number)
//allocates block same as ino
int balloc(int dev)
{
  int i;
    char buf[BLKSIZE];

    get_block(dev, bmap, buf);

    for (i = 0; i < nblocks; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            decFreeBlocks(dev);
            put_block(dev, bmap, buf);
            printf("Free disk block at %d\n", i + 1); // bits count from 0; ino from 1
            return i + 1;
        }
    }
    return 0;
}

int clr_bit(char *buf, int bit) // clear bit in char buf[BLKSIZE]
{ return buf[bit/8] &= ~(1 << (bit%8)); }

int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

int incFreeInodes(int dev)
{
    char buf[BLKSIZE];
    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}
int idealloc(int dev, int ino)
{
     int i;
    char buf[BLKSIZE];

    if (ino > ninodes)
    {
        printf("inumber %d out of range\n", ino);
        return;
    }
    // get bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino - 1);
    // write buf back
    put_block(dev, imap, buf);
    // update free inode count in SUPER and GD
    incFreeInodes(dev);

}
int inode_truncate(MINODE* mip) //KEEP
{
    char buf[BLKSIZE];
    INODE *ip = &mip->INODE;
    // 12 direct blocks
    for (int i = 0; i < 12; i++) {
        if (ip->i_block[i] == 0)
            break;
        // now deallocate block
        bdealloc(dev, ip->i_block[i]);
        ip->i_block[i] = 0;
    }
    // now worry about indirect blocks and doubly indirect blocks
    
    // indirect blocks:
    if (ip->i_block[12] != NULL) {
        get_block(dev, ip->i_block[12], buf); // follow the ptr to the block
        int *ip_indirect = (int *)buf; // reference to indirect block via integer ptr
        int indirect_count = 0;
        while (indirect_count < BLKSIZE / sizeof(int)) { // split blksize into int sized chunks (4 bytes at a time)
            if (ip_indirect[indirect_count] == 0)
                break;
            // deallocate indirect block
            bdealloc(dev, ip_indirect[indirect_count]);
            ip_indirect[indirect_count] = 0;
            indirect_count++;
        }
        // now all indirect blocks have been dealt with, deallocate reference to indirect
        bdealloc(dev, ip->i_block[12]);
        ip->i_block[12] = 0;
    }

    // doubly indirect blocks (same code as above, different variables):
    if (ip->i_block[13] != NULL) {
        get_block(dev, ip->i_block[13], buf);
        int *ip_doubly_indirect = (int *)buf;
        int doubly_indirect_count = 0;
        while (doubly_indirect_count < BLKSIZE / sizeof(int)) {
            if (ip_doubly_indirect[doubly_indirect_count] == 0)
                break;
            // deallocate doubly indirect block
            bdealloc(dev, ip_doubly_indirect[doubly_indirect_count]);
            ip_doubly_indirect[doubly_indirect_count] = 0;
            doubly_indirect_count++;
        }
        bdealloc(dev, ip->i_block[13]);
        ip->i_block[13] = 0;
    }

    mip->INODE.i_blocks = 0;
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    iput(mip);

}
int bdealloc(dev, bno)
{
    
    char buf[BLKSIZE]; // a sweet buffer

    get_block(dev, bmap, buf); // get the block
    clr_bit(buf, bno - 1);     // clear the bits to 0
    put_block(dev, bmap, buf); // write the block back
    incFreeBlocks(dev);        // increment the free block count
    return 0;
}





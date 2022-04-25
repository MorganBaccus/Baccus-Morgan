#include <stdio.h>             // for I/O
#include <stdlib.h>            // for I/O
#include <libgen.h>            // for dirname()/basename()
#include <string.h>

typedef struct node{
          char  name[64];       // node's name string
          char  type;           // 'D' for DIR; 'F' for file
   struct node *child, *sibling, *parent;
}NODE;


NODE *root, *cwd, *start;
//char line[128];
char command[16], pathname[64];
char dpathcopy[64], bpathcopy[64];
char *dname, *bname;

char *cmd[] = {"mkdir", "rmdir","cd","ls","pwd", "creat", "rm", "save", "reload", "menu", "quit", 0};


int findCmd(char *command)
{
   int i = 0;
   while(cmd[i]){
     if (strcmp(command, cmd[i])==0)
         return i;
     i++;
   }
   return -1;
}

NODE *search_child(NODE *parent, char *name)
{
  NODE *p;
  //printf("search for %s in parent DIR\n", name);
  p = parent->child;
  if (p==0)
    return 0;
  while(p){
    if (strcmp(p->name, name)==0)
      return p;
    p = p->sibling;
  }
  return 0;
}

int insert_child(NODE *parent, NODE *q)
{
  NODE *p;
  //printf("insert NODE %s into END of parent child list\n", q->name);
  p = parent->child;
  if (p==0)
    parent->child = q;
  else{
    while(p->sibling)
      p = p->sibling;
    p->sibling = q;
  }
  q->parent = parent;
  q->child = 0;
  q->sibling = 0;
}

/***************************************************************
 This mkdir(char *name) makes a DIR in the current directory
 You MUST improve it to mkdir(char *pathname) for ANY pathname
****************************************************************/

// Finds if path is in the tree, returns last node in path or 0 if path is not in tree
NODE *search_path(char *path) // Won't change the path and cannot handle ".."
{
  NODE *p;
  if(strcmp(path, "/") == 0 || strcmp(path,".") == 0){ // p is the start
    return start;
  }else{
    char dcopy[64], bcopy[64]; // dirname and basename can alter string
    strcpy(dcopy, path);
    strcpy(bcopy, path);
    char *d = dirname(dcopy);
    char *b = basename(bcopy);
    //printf("dir %s base:%s\n", d, b); 
    
    p = search_path(d); // Doesn't start at the beginning of path so must check directory one level above
    if(p){ // If directory one level up exists
      if(strcmp(b, ".") == 0 || strcmp(b, "/") == 0)
	return p;
      return search_child(p,b); // Check if this path exists
    }else{
      return p; // p is 0 so path does not exist
    }
  }
}

int getdbnames(){
  strcpy(dpathcopy, pathname);
  strcpy(bpathcopy, pathname);
  dname = dirname(dpathcopy);
  bname = basename(bpathcopy);
}

int set_start(char *path)
{
  if(path[0]=='/'){
    start = root; // Absolute: dname starts with /
  }else{
    start = cwd; // Relative: dname starts with .
  }
}

int mkdir(char *pathname)
{
  NODE *dir_ptr, *q;
  getdbnames();
  // printf("mkdir: pathname=%s\n", pathname);
  // printf("mkdir: directory=%s\n", dname);
  // printf("mkdir: dname=%s\n", bname);
  
  if (!strcmp(bname, "/") || !strcmp(bname, ".") || !strcmp(bname, "..") || !strcmp(bname, "./") || !strcmp(bname, "../")){
    printf("can't mkdir with %s\n", pathname);
    return -1;
  }

  // Set start pointer to either relative or absolute
  set_start(dname);

  // Check if directory already exists
  dir_ptr = search_path(dname);
  if(dir_ptr){
    if(dir_ptr->type == 'D'){
      if(search_child(dir_ptr, bname)){
	printf("path %s already exists, mkdir FAILED\n", pathname);
	return -1;
      }
    }else{
      printf("Given directory %s is not DIR type, mkdir FAILED\n", dname);
      return -1;
    }
  }else{
      printf("%s is not a valid path, mkdir FAILED\n", dname);
    return -1;
  }
	    
  q = (NODE *)malloc(sizeof(NODE));
  q->type = 'D';
  strcpy(q->name, bname);
  insert_child(dir_ptr, q);
  printf("mkdir %s OK\n", pathname);
    
  return 0;
}

int rmdir(char *pathname)
{
  set_start(pathname);
  NODE *p = search_path(pathname);
  if(p){
    if(p->type == 'D'){
      if(p->child){
	printf("%s directory is not empty, rmdir FAILED\n", pathname);
	return -1;
      }else{
	// rm directory
	// first fix pointers of neighboring nodes
	if(p->parent->child == p){
	  // removing oldest child need to update parent
	  p->parent->child = p->sibling;
	}else{
	  // not oldest sibling, update sibling
	  // find sibling that points at p, then update to point at p's sibling
	  NODE *ptemp = p->parent->child;
	  while(ptemp->sibling != p){
	    ptemp = ptemp->sibling;
	  }
	  ptemp->sibling = p->sibling;
	}
	free(p); //deallocate memory for directory deleting
	printf("directory %s removed\n", pathname);
      }
    }else{
      printf("%s is not a valid directory, rmdir FAILED\n", pathname);
      return -1;
    }
  }else{
    printf("%s is not a valid path, rmdir FAILED\n", pathname);
    return -1;
  }

}

// This lists the given director, dir must be a valid directory
int ls_dir(NODE *dir) 
{
  NODE *p = dir->child;
  //printf("contents in %s\n", dirname);
  if(!p)
    printf("empty\n");
  while(p){
    if(p->type == 'D')
      printf("\033[1;34m"); //blue text for directories
    printf("[%c %s] ", p->type, p->name);
    if(p->type == 'D') 
      printf("\033[0m");//set back to default text color
    p = p->sibling;
  }
  printf("\n");
}

int ls(char *pathname)
{
  if(pathname[0] != '\0'){
    NODE *p;
    set_start(pathname); //set start to relative or absolute
    p= search_path(pathname); //find path node
    if(p){
      //check for if path is a directory
      if(p->type = 'D'){
	if(pathname[0] != '/'){ //append ./ to pathname for display purposes
	  char temp[64] = "./";
	  strcat(temp,pathname);
	  strcpy(pathname, temp);
	}
	//list all items in the directory
	ls_dir(p);
      }else{
	printf("%s is not a valid directory, ls FAILED\n",pathname);
	return -1;
      }
    }else{
      printf("%s is not a valid path, ls FAILED\n", pathname);
      return -1;
    }
  }else{ //no pathname specified, list current working directory
    ls_dir(cwd); 
  }
}

int cd(char *pathname)
{
  if(strcmp(pathname, "")==0){
    //no pathname specified, go to root
    cwd = root;
  }else if(strcmp(pathname, "..") == 0){ //pathname is ".."
    cwd = cwd->parent; //move the cwd diretory up a directory. If at root will stay at root because root's parent pointer is itself
  }else{
    //deal with ../../path senerios
   
    while(strncmp("../",pathname, 3) == 0){
       cwd = cwd->parent; //move the cwd diretory up a directory. If at root will stay at root because root's parent pointer is itself
       strcpy(pathname, pathname + 3); //remove the "../" from begining of pathname
      }
    
  if(strcmp(pathname, "") != 0 ){ //if command just../../ then no more path to deal with
    NODE *p=search_path(pathname);
    if(p){
      //check if it is a directory
      if(p->type == 'D'){
	cwd = p; //update cwd
      }else{
	printf("%s is not a directory, cd FAILED\n", pathname);
	return -1;
      }
    }else{
        printf("%s is not a valid path, cd FAILED\n", pathname);
	return -1;
    }
   }
  }
}

int creat(char *pathname)
{
  NODE *dir_ptr, *q;
  getdbnames();
  //printf("creat: pathname=%s\n", pathname);
  // printf("creat: directory=%s\n", dname);
  //printf("creat: dname=%s\n", bname);

   if (!strcmp(bname, "/") || !strcmp(bname, ".") || !strcmp(bname, "..") || !strcmp(bname, "./") || !strcmp(bname, "../")){
    printf("can't creat with %s\n", pathname);
    return -1;
  }

  //set start pointer to relative or absolute
  set_start(dname);

  //check whether directory exists
  dir_ptr = search_path(dname);
  if(dir_ptr){
    if(dir_ptr->type == 'D'){
      if(search_child(dir_ptr, bname)){
	printf("path %s already exists, creat FAILED\n",pathname);
	return -1;
      }
    }else{
      printf("Given path  %s is not File type, creat FAILED\n", dname);
      return -1;
    }
  }else{
      printf("Path %s does not exist, creat FAILED\n", dname);
    return -1;
  }
	    
 
  q = (NODE *)malloc(sizeof(NODE));
  q->type = 'F';
  strcpy(q->name, bname);
  insert_child(dir_ptr, q);
  printf("creat %s OK\n", pathname); 
  return 0;
}

//for file not dir
int rm(char *pathname)
{
  set_start(pathname);
  NODE *p = search_path(pathname);
  if(p){
    if(p->type == 'F'){
	//rm file
	//first fix pointers of neighboring nodes
	if(p->parent->child == p){
	  //removing oldest child need to update parent
	  p->parent->child = p->sibling;
	}else{
	  //not oldest sibling, update sibling
	  //find sibling that points at p, then update to point at p's siblin
	  NODE *ptemp = p->parent->child;
	  while(ptemp->sibling != p){
	    ptemp = ptemp->sibling;
	  }
	  ptemp->sibling = p->sibling;
	}
	free(p); //deallocate memory for file node removing
	printf("file %s removed\n", pathname);
    }else{
      printf("%s is not a valid file, rm FAILED\n", pathname);
      return -1;
    }
  }else{
    printf("%s is not a valid path, rm FAILED\n", pathname);
    return -1;
  }
}


int initialize()
{
    root = (NODE *)malloc(sizeof(NODE));
    strcpy(root->name, "/");
    root->parent = root;
    root->sibling = 0; //according to textbook this should be root, we will see if I need to change that
    root->child = 0;
    root->type = 'D';
    cwd = root;
    printf("Root initialized OK\n");
}

int pwd_helper(NODE *p) //recursively prints cwd's absolute path
{
  char *name = p->name;
  if(p != root){
    pwd_helper(p->parent);
  }
  else{ //is root so don't want to print extra /
    printf("%s",p->name);
    return 0;
  }
  printf("%s/", p->name);
}

int pwd()
{
  printf("cwd= ");
  pwd_helper(cwd);
  printf("\n");
}

int absolute_path(NODE *p, char *path) //recursively 
{
  char *name = p->name;
  if(p != root){
    absolute_path(p->parent, path); //determine prior path
    strcat(path, "/");
    strcat(path, name); //append this node name on
  }
  return 0;
}

int save_helper(FILE *fp, NODE *p)
{
  if(p != root){ //don't include root in save file (assumed on reload)
    //get full file name for current node and print
    char path[64] = "";
    absolute_path(p, path);
    fprintf(fp, "%c\t%s\n", p->type, path);
  }
  if(p->child)
    save_helper(fp, p->child); //then do same for child (preorder traversal)
  if(p->sibling)
    save_helper(fp, p->sibling); //then do same for sibling
  return 0;
}

int save(char *filename)
{
  if(strcmp(filename, "")==0) //if not pathname/filename given in command, default to "systree"
    strcpy(filename, "systree");
  FILE *fp = fopen(filename, "w+");
  save_helper(fp, root);
  fclose(fp);
  printf("Current system tree saved to %s \n", filename);
  return 0;
}

//deallocate heap memory/delete tree to use before reload and exit
int delete_tree(NODE *p)
{
  if(p && p!= root){
    delete_tree(p->child); //postorder traversal to free all nodes
    delete_tree(p->sibling);
    free(p);
  }else if(p == root){ //root doesn't have a sibling (points to self)
    delete_tree(p->child);
  }
  return 0; //returns if p is null
}

int reload(char *filename)
{
  //delete existing tree
  delete_tree(root);
  
  //initialize (don't make new root node, therefore not using intitialize())
  strcpy(root->name, "/");
  root->parent = root;
  root->sibling = 0; 
  root->child = 0;
  root->type = 'D';
  cwd = root;
  printf("Root initialized OK\n");

  if(strcmp(filename, "")==0) //if not pathname/filename given in command, default to "systree"
    strcpy(filename, "systree");
  
  FILE *fp = fopen(filename, "r");
  if(fp){
    char type='\0';
    while((type = fgetc(fp))!= EOF){ //get file type
      int check = fscanf(fp, "%s", pathname); //get node path, use global pathname because mkdir and creat rely on that path matching (to get basename and dirname)
      if(check==1){ //expect 1 correcly read item from scanf
	if(type == 'D'){
	  mkdir(pathname);
	}else if(type == 'F'){
	  creat(pathname);
	}else{
	  printf("Encountered invalid file type, reload failed\n");
	}
	char garbage = fgetc(fp); //clear newline char
      }else if(check != EOF){//if eof that ok, want to end without message, but if not eof and not 1 there is an error
	printf("Encountered error with path, reload failed\n");
      }
    }
  }else{
    printf("%s not found, reload FAILED\n", filename);
  }
  fclose(fp);
  return 0;
}

int menu()
{
  char *cmd_descriptions[] ={" pathname - make directory", " pathname - remove empty directory", " [pathname] - change current working directory", " [pathname] - list current working directoy", " - print the absolute path of the current working directory" , " pathname - create a file node", " pathname - remove a file node", " filename - save the current file system tree as a file", " filename - construct a file system tree from a file", " - show available commands", " - save the file system tree and terminate the program"};
  printf("Available commands\n");
  for(int i=0; i <11; i++)
    printf("\t%s%s\n", cmd[i], cmd_descriptions[i]);
}

  // improve quit() to SAVE the current tree as a Linux file
  // for reload the file to reconstruct the original tree
int quit()
{
  save("systree"); //save tree to file
  delete_tree(root); //deallocate heap memory
  exit(0); //exit program
}


int main()
{
  int index;
  char line[128];
  initialize();

  printf("Enter \'menu\' to list commands\n");
  
  while(1){
     printf("\n");
    //prompt to enter command (cwd followed by $)
     printf("\033[1;34m"); //blue
     pwd_helper(cwd);
     printf("\033[0m");
     printf("$ ");
     
      fgets(line, 128, stdin);
      line[strlen(line)-1] = 0;

      sscanf(line, "%s %s", command, pathname);
      // printf("command=%s pathname=%s\n", command, pathname);

      //if (command[0]==0) 
      // continue;

      index = findCmd(command);

      switch (index){
        case -1: printf("invalid command %s\n",command); break;
        case 0: mkdir(pathname); break;
        case 1: rmdir(pathname); break;
        case 2: cd(pathname);    break;
        case 3: ls(pathname);    break;
        case 4: pwd();           break;
        case 5: creat(pathname); break;
        case 6: rm(pathname);    break;
        case 7: save(pathname);  break; 
	case 8: reload(pathname); break;
        case 9: menu();          break;
        case 10: quit();         break;
     
      }
      
      pathname[0] = '\0'; //clear pathname so if next command doesn't take a pathname, it correctly knows to use cwd (such as with ls after mkdir)
  }
}

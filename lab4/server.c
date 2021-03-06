#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <time.h> 

#define MAX 256
#define BLK  1024
#define PORT 1234
#define HOST "localhost"
#define IP "127.0.0.1"

char ans[MAX];
char line[MAX];

int sfd, cfd;                       // file descriptors for sockets
char *serverIP = "127.0.0.1";       // define server IP address
int serverPORT = 1234;              // define server port number

struct sockaddr_in saddr, caddr;

int find_cmd_index(char *command);
int has_argument(char *line);
int server_get(char *filename);
int server_put(char *filename);
int server_ls(char *pathname);
int ls_dir(char *pathname);
int ls_file(char *fname);
int server_cd(char *pathname);
int server_pwd();
int server_mkdir();
int server_rmdir();
int server_rm();

char *cmd[] = {"get", "put", "ls", "cd", "pwd", "mkdir", "rmdir", "rm"};

int (*fptr[])(char *) = {(int (*)())server_get, server_put, server_ls, server_cd, server_pwd, server_mkdir, server_rmdir, server_rm};


int init()
{
    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET;
    //saddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    saddr.sin_addr.s_addr = inet_addr(serverIP);
    saddr.sin_port = htons(serverPORT);
    
    printf("3. bind socket to server\n");
    int r = (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr))); 
    if(r < 0) { 
        printf("socket bind failed\n"); 
        exit(0); 
    }
    // Now server is ready to listen and verification 
    else {
        printf("Socket bind successful\n");
    }
    printf(" hostname = %s port = %d\n", HOST, PORT);
    printf("4. server listen with queue size = 5\n");
    if ((listen(sfd, 5)) != 0) { 
        printf("Listen failed\n"); 
        exit(0); 
    }
}

int main(int argc, char *argv[], char *env[])
{
    // create virtual root to CWD
    chdir("./");
    chroot("./");

    int n, length;
    char line[MAX];
    char command[16], arg[64];
    
    init();  

    while(1){
       // Try to accept a client connection as descriptor newsock
       length = sizeof(caddr);
       cfd = accept(sfd, (struct sockaddr *)&caddr, &length);
       if (cfd < 0){
          printf("server: accept error\n");
          exit(1);
       }

       printf("server: accepted a client connection from\n");
       printf("-----------------------------------------------\n");
       printf("    IP=%s  port=%d\n", "127.0.0.1", ntohs(caddr.sin_port));
       printf("-----------------------------------------------\n");

       // Processing loop
       while(1){
         memset(command, '\0', sizeof(command));
            memset(arg, '\0', sizeof(arg));
            memset(line,'\0', sizeof(line));  
         printf("server ready for next request ....\n");
         n = read(cfd, line, MAX);
         if (n==0){
           printf("server: client died, server loops\n");
           close(cfd);
           break;
         }
         line[n]=0;
         // show the line string
         printf("server: read  n=%d bytes; line=[%s]\n", n, line);

        memset(data, 0, MAX);
          if (has_argument(line)) {
                sscanf(line, "%s %s", command, arg);
            }
            else {
                strcpy(command, line);
                strcpy(arg, "");
            }

            int index = find_cmd_index(command);
            if (index != -1) {
                int r = fptr[index](arg);
                strcat(data, line);
                if (r != -1) {
                    strcat(data, " OK\n");
                }
                else {
                    strcat(data, " FAILED\n");
                }
            }
            else {
                printf("invalid command %s\n", line);
            }
       }
    }
}

int has_argument(char *line)
{
    int size = strlen(line);
    for (int i = 0; i < size; i++) {
        if (line[i] == ' ') {
            if (line[i + 1] != ' ' || line[i + 1] != '\0') {
                return 1;
            }
        }
    }
    return 0;
}

int find_cmd_index(char *command)
{
    int i = 0;
    while (cmd[i]) {
        if (!strcmp(command, cmd[i])) {
            printf("Found %s, index %d\n", command, i);
            return i;
        }
        i++;
    }
    return -1;
}

// server commands
int server_get(char *filename)
{
    int r;
    char buffer[MAX];

    struct stat fstat, *sp;
    sp = &fstat;
    if ((r = lstat(filename, &fstat)) < 0) {
        printf("can???t stat %s\n", filename);
        return -1;
    }
    int file_size = sp->st_size;
    sprintf(buffer, "%d", file_size);

    write(cfd, buffer, MAX);
    int fp = open(filename, O_RDONLY);
    if (fp > 0) {
        char buf[MAX];
        memset(buf, '\0', sizeof(buf));
        int n = read(fp, buf, MAX);
        while(n > 0) {
            write(cfd, buf, n);
            n = read(fp, buf, MAX);
        }
    }
    close(fp);
    return 0;
}

int server_put(char *filename)
{
    char buf[MAX];
    int fd;
    int b = read(cfd, buf, MAX);
    int file_size = atoi(buf);
    memset(buf, 0, sizeof(buf));
    // synch data for get, arg is the filename
    fd = open(filename, O_WRONLY|O_CREAT, 0644);
    if (fd > 0) {
        int bytes_read = 0;
        int packet_size = 0; // packet size is between 0 and MAX
        while (file_size > 0) {
            read(cfd, buf, MAX);
            bytes_read += MAX;
            if(file_size < MAX) {
                write(fd, buf, file_size);
                file_size -= file_size;
            }
            else {
                write(fd, buf, MAX);
                file_size -= MAX;
            }
        }
        close(fd);
    }
}

int server_ls(char *pathname)
{
    char buf[MAX];
    getcwd(buf, MAX-1);
    if (!strcmp(pathname, "")) {
        ls_dir(buf);
        return 1;
    }
    ls_dir(pathname);
    return 0;
}

int ls_dir(char *pathname)
{
    struct dirent *dp;
    DIR *mydir;
    char fullPath[MAX * 2];
    memset(fullPath, '\0', sizeof(fullPath));

    if ((mydir = opendir(pathname)) == NULL) {
        perror("couldn't open pathname");
        return 1;
    }

    do {
        if ((dp = readdir(mydir)) != NULL) {
            memset(fullPath, '\0', sizeof(fullPath));
            strcpy(fullPath, pathname);
            strcat(fullPath, "/");
            strcat(fullPath, dp->d_name);
            ls_file(fullPath);
        }

    } while (dp != NULL);

    closedir(mydir);
    return 0;
}

int ls_file(char *fname)
{
    int n;
    char buff[MAX];
    char fmt[MAX];
    char linkname[MAX];
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(fname, &fstat)) < 0) {
        sprintf(fmt,"can???t stat %s\n", fname);
        strcpy(buff, fmt);
        n = write(cfd, buff, MAX);
        printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, buff);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) {     // if (S_ISREG())
        sprintf(fmt, "%c", '-');
        strcat(buff, fmt);
    }
    if ((sp->st_mode & 0xF000) == 0x4000) {     // if (S_ISDIR())
        sprintf(fmt, "%c", 'd');
        strcat(buff, fmt);
    }   
    if ((sp->st_mode & 0xF000) == 0xA000) {      // if (S_ISLNK())
        sprintf(fmt, "%c", 'l');
        strcat(buff, fmt);
    }
    for (i = 8; i >= 0; i--) {
        if (sp->st_mode & (1 << i)){
            sprintf(fmt, "%c", t1[i]);      // print r|w|x printf("%c", t1[i]);
            strcat(buff, fmt);
        }
        else {
            sprintf(fmt, "%c", t2[i]);      // or print -
            strcat(buff, fmt);
        }
    }
    sprintf(fmt, "%4d ", sp->st_nlink);     // link count
    strcat(buff, fmt);
    sprintf(fmt, "%4d ", sp->st_gid);       // gid
    strcat(buff, fmt);
    sprintf(fmt, "%4d ", sp->st_uid);       // uid
    strcat(buff, fmt);
    sprintf(fmt, "%8ld ", sp->st_size);     // file size
    strcat(buff, fmt);

    strcpy(ftime, ctime(&sp->st_ctime));    // print time in calendar form
    ftime[strlen(ftime) - 1] = 0;
    sprintf(fmt, "%s ", ftime);
    strcat(buff, fmt);

    sprintf(fmt, "%s", basename(fname));
    strcat(buff, fmt);
    if ((sp->st_mode & 0xF000) == 0xA000) {
        readlink(fname, linkname, MAX);
        sprintf(fmt," -> %s", linkname);
        strcat(buff, fmt);
    }

    strcat(buff, "\n");
    n = write(cfd, buff, MAX);

    printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, buff);
    memset(buff, 0, MAX);
}

int server_cd(char *pathname)
{
    return chdir(pathname);
}

int server_pwd()
{
    char buf[MAX];
    getcwd(buf, MAX-1);
    strcat(buf, "\n");
    printf("SERVER LOCAL: %s\n", buf);
    write(cfd, buf, MAX);
}

int server_mkdir(char *pathname)
{
    return mkdir(pathname, 0755);
}

int server_rmdir(char *pathname)
{
    return rmdir(pathname);
}

int server_rm(char *pathname)
{
    return unlink(pathname);
}
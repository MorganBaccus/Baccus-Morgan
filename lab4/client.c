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
#include <libgen.h>     // for dirname()/basename()
#include <time.h>

#define MAX 256
//#define PORT 1234
#define BLK 1024
#define EOT "\\r\\n\\r\\n"
#define DEBUG 0

int PORT = 1234;
char *serverIP = "127.0.0.1";

//char line[MAX], ans[MAX];
int n;

struct sockaddr_in saddr;
int sfd;

int find_cmd_index(char *command);
int lcat(char *filename);
int lls(char * pathname);
int ls_dir(char *pathname);
int ls_file(char *fname);
int lcd(char *pathname);
int is_end_of_tranmission(char * response);
int lpwd();
int lmkdir(char *pathname);
int lrmdir(char *pathname);
int lrm(char *pathname);
int menu();
int init();

char *cmd[] = {"lcat", "lls", "lcd", "lpwd", "lmkdir", "lrmdir", "lrm", "menu"};

int (*fptr[])(char *) = {(int (*)())lcat, lls, lcd, lpwd, lmkdir, lrmdir, lrm, menu};

int init()
{
    int n;


    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        printf("socket creation failed\n");
        exit(0);
    }

    printf("2. fill in server IP=%s, port number=%d\n", serverIP, PORT);
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(serverIP);
    saddr.sin_port = htons(PORT);

    printf("3. connect to server\n");
    if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }

    printf("\nconnection to server successful\n");
}

int main(int argc, char *argv[], char *env[])
{

    int n;
    char line[MAX], response[MAX];
    char command[16], arg[64];

    init();

    while (1){
        memset(command, '\0', sizeof(command));
        memset(arg, '\0', sizeof(arg));
        memset(line,'\0', sizeof(line));
        memset(response,'\0', sizeof(response));

        printf("$ ");
        fgets(line, MAX, stdin);
        line[strlen(line) - 1] = 0; // kill <CR> at end
        if (line[0] == 0 || !strcmp(line, "quit") || !strcmp(line, "exit")) // exit if NULL line or "quit" or "exit"
            exit(0);

        // check if the command should be executed locally
        sscanf(line, "%s %s", command, arg);
        if (find_cmd_index(command) != -1) { // local command, run on client only
        #if DEBUG
            printf("%s\n", command);
        #endif
            int index = find_cmd_index(command);
            if (index != -1) {
                fptr[index](arg);
            }
            else {
                printf("invalid command %s\n", line);
            }
        }
        else { // send to server
            // Send ENTIRE line to server
            n = write(sfd, line, MAX);
        #if DEBUG
            printf("Sent: %s\n", line);
        #endif
            char command[16], arg[64];
            sscanf(line, "%s %s", command, arg);
            char buf[MAX];
            if (!strcmp(command, "get")) {
                int fd;
                int b = read(sfd, buf, MAX);
                int file_size = atoi(buf);
                memset(buf, 0, sizeof(buf));
                // synch data get, arg is the filename
                fd = open(arg, O_WRONLY|O_CREAT, 0644);
                if (fd > 0) {
                    int bytes_read = 0;
                    int packet_size = 0; // packet sizes are between 0 and MAX
                    while (file_size > 0) {
                        read(sfd, buf, MAX);
                        bytes_read += MAX;
                        if(file_size < MAX){
                            write(fd, buf, file_size);
                            file_size -= file_size;
                        }
                        else{
                            write(fd, buf, MAX);
                            file_size -= MAX;
                        }
                    }
                    close(fd);
                }

            } else if (!strcmp(command, "put")) {
                // synch data for put, arg is the filename
                int r;
                char buffer[MAX];

                struct stat fstat, *sp;
                sp = &fstat;
                if ((r = lstat(arg, &fstat)) < 0) {
                    printf("can???t stat %s\n", arg);
                    return -1;
                }
                int file_size = sp->st_size;
                sprintf(buffer, "%d", file_size);

                write(sfd, buffer, MAX);
                int fp = open(arg, O_RDONLY);
                if (fp > 0) {
                    char buf[MAX];
                    memset(buf, '\0', sizeof(buf));
                    int n = read(fp, buf, MAX);
                    while(n > 0){
                        write(sfd, buf, n);
                        n = read(fp, buf, MAX);
                    }
                }
                close(fp);
                bzero(response, sizeof(response));
                n = read(sfd, response, sizeof(response));
                while(!is_end_of_tranmission(response)) {
                    #if DEBUG
                        printf("client read: %s", response);
                    #else 
                        printf("%s", response);
                    #endif
                    bzero(response, sizeof(response));
                    n = read(sfd, response, sizeof(response));
                }
            } else {
                // Read a line from sock and show it
                bzero(response, sizeof(response));
                n = read(sfd, response, sizeof(response));
                while(!is_end_of_tranmission(response)) {
                    #if DEBUG
                        printf("%s", response);
                    #else
                        printf("client read: %s", response);
                    #endif

                    bzero(response, sizeof(response));
                    n = read(sfd, response, sizeof(response));
                }
            }
        }
    }
}

int is_end_of_tranmission(char * response)
{
    if(!strcmp(response, EOT)) {
        #if DEBUG
            printf("End of transmission\n");
        #endif
        return 1;
    }
    return 0;
}

int find_cmd_index(char *command)
{
    int i = 0;
    while (cmd[i]) {
        if (!strcmp(command, cmd[i])) {
            #if DEBUG
                printf("Found %s, index %d\n", command, i);
            #endif
            return i;
        }
        i++;
    }
    return -1;
}

int menu()
{
    puts("MENU:");
    puts("get  put  ls   cd   pwd   mkdir   rmdir   rm");
    puts("lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm");
    puts("\n\n");
}

int lcat(char *filename) {
    char buf[512];
    FILE *fd = fopen(filename, "r");
    if (fd != NULL)
    {
        while (fgets(buf, 512, fd) != NULL)
        {
            buf[strlen(buf) - 1] = '\0';
            puts(buf);
        }
    }
    else
    {
        printf("fopen failed, aborting\n");
        return 1;
    }
    return 0;
}

int ls_dir(char *pathname)
{
    struct dirent *dp;
    DIR *mydir;
    char fullPath[MAX * 2];
    memset(fullPath, '\0', sizeof(fullPath));

    if ((mydir = opendir(pathname)) == NULL)
    {
        perror("couldn't open pathname");
        return 1;
    }

    do
    {
        if ((dp = readdir(mydir)) != NULL)
        {
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

int lls(char * pathname)
{
    if (!strcmp(pathname, "")){
        ls_dir("./");
        return 1;
    }
    ls_dir(pathname);
    return 0;
}

int ls_file(char *fname)
{
    char linkname[MAX];
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(fname, &fstat)) < 0) {
        printf("can???t stat %s\n", fname);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000)   // if (S_ISREG())
        printf("%c", '-');
    if ((sp->st_mode & 0xF000) == 0x4000)   // if (S_ISDIR())
        printf("%c", 'd');
    if ((sp->st_mode & 0xF000) == 0xA000)   // if (S_ISLNK())
        printf("%c", 'l');
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i))
            printf("%c", t1[i]);    // print r|w|x printf("%c", t1[i]);
        else
            printf("%c", t2[i]);    // or print -
    }
    printf("%4d ", sp->st_nlink);   // link count
    printf("%4d ", sp->st_gid);     // gid
    printf("%4d ", sp->st_uid);     // uid
    printf("%8ld ", sp->st_size);   // file size

    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form ftime[strlen(ftime)-1] = 0; // kill \n at end
    ftime[strlen(ftime) - 1] = 0;        // removes the \n
    printf("%s ", ftime);                // prints the time

    printf("%s", basename(fname));      // print file basename // print -> linkname if symbolic file
    if ((sp->st_mode & 0xF000) == 0xA000) {
        readlink(fname, linkname, MAX);
        printf(" -> %s", linkname);
    }

    printf("\n");
    return 0;
}

int lcd(char *pathname)
{
    return chdir(pathname);
}

int lpwd()
{
    char buf[MAX];
    getcwd(buf, MAX);
    printf("%s\n", buf);
}

int lmkdir(char *pathname)
{
    return mkdir(pathname, 0755);
}

int lrmdir(char *pathname)
{
    return rmdir(pathname);
}

int lrm(char *pathname)
{
    return unlink(pathname);
}
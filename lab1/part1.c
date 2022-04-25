/**********part1.c**********/

typedef unsigned int u32; //given
char *tab = "0123456789ABCDEF"; //given
int BASE = 10; //given

int prints(char *s); // given
int rpu(u32 x); //given
int printu(u32 x); //given
int printd(int x); //name given
int printx(u32 x); //name given
int printo(u32 x); //name given
int myprintf(char *fmt, ...); //name given

int main(int argc, char *argv[], char *env[])
{
printu(123);
char *letters = 'abc';
prints(letters);
printu(10);
printd(10);
printx(10);
printo(10);
myprintf("cha=%c string=%s dec=%d hex=%x oct=%o neg=%d\n", 'A', "this is a test", 100, 100, 100, -100);
}

// 1-1. Write your own prints(char *s) function to print a string
int prints(char *s)
{
for (int i=0; s[i] != '\0'; i++)
{
putchar(s[i]);
}
putchar('\n');
}




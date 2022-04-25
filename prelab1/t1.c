/**********t1.c**********/
#include <stdio.h>
int g;
int main()
{
static int a,b,c;
a = 1; b = 2;
c = a + b;
printf("c=%d\n",c);
}

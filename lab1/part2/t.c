/**********t.c file**********/

#include <stdio.h>
#include <stdlib.h>

int *FP;

int A(int x, int y);
int B(int x, int y);
int C(int x, int y);

int main(int argc, char *argv[ ], char *env[ ])
{
	int a,b,c;
	printf("enter main\n");
	
	printf("&argc=%x argv=%x env=%x\n", &argc, argv, env);
	printf("&a=%8x &b=%8x &c=%8x\n", &a, &b, &c);
	
// 1. Write C code to print values of argc and argv[] entries

	// Print arc
	printf("argc %d\n", argc);
	
	// Print argv[]
	for(int i = 0; i < argc; i++)
	{
		printf("argv[%d]=%s\n", i, argv[i]);
	}

	a=1; b=2; c=3;
	A(a,b);
	printf("exit main\n");
}
	
int A(int x, int y)
{
	int d,e,f;
	printf("enter A\n");
	
// Write C code to print the addresses of d, e, and f
	printf("&d=%8x\n&e=%8x\n&f=%8x\n", &d, &e, &f);

	d=4; e=5; f=6;
	B(d,e);
	printf("Exit A\n");
}

int B(int x, int y)
{
	int g,h,i;
	printf("enter B\n");
	
// Write C code to print the addresses of g, h, and i
	printf("&g=%8x\n&h=%8x\n&i=%8x\n", &g, &h, &i);

	g=7; h=8; i=9;
	C(g,h);
	printf("exit B\n");
}

int C(int x, int y)
{
	int u, v, w, i, *p;
	
	printf("enter c\n");

// Write C code to print the addresses of u, v, w, i, and p
	printf("&u=%8x\n&v=%8x\n&w=%8x\n&i=%8x\n&p=%8x\n", &u, &v, &w,
	 &i, &p);

	u=10; v=11; w=12; i=13;
	
	FP = (int *)getebp(); // FP = stack frame pointer of the C() function
	
// Print FP value in HEX
	printf("FP=%8x\n", FP);

// 2. Write C code to print the stack frame link list
	p = FP;
	while(p != 0)
	{
	printf("FP=%8x\n", p);
	p = *p;
	}
	printf("FP -> %8x ->\n", p);

	// p = (int *)&p;
	
// 3. Print the stack contents from p to the frame of main() (128 entries will suffice)
	for (int i = 0; i <128; i++)
	{
	printf("%d(p)->%8x\n%d(p) = %d\n\n", i, p, i, *p);
	p++;
	}
	

// 4. On a hard copy of the print out, identify the stack contents as Local variables, Parameters, Stack frame pointer of each function

}
	

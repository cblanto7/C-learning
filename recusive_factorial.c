/* Calculates the factorial of a number recursively  */

#include <stdio.h>

unsigned long long factorial(unsigned long long);

int main(void){
	char temp[50];
	unsigned long long number = 0LL;

	printf("Enter an interger value: ");
	fgets(temp, sizeof(temp), stdin);
	sscanf(temp, "%llu", &number);
	printf("the factorial of %llu is %llu\n", number, factorial(number) );
	return 0;
}

//recursive factorial function
unsigned long long factorial(unsigned long long n){
	if( n < 2L )
		return n;

	return n*factorial(n - 1LL);
}
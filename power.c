/* program that calculates X to the power of N */
/* takes input from the user and the power function is recursive */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

double power(double x, int n); //proto returns x to the n power

int main(int argc, char * argv[]){

	char *pInput[10];
	double x, result;
	int n;

	printf("%lu\n", sizeof(char *) );
	printf("%lu\n", sizeof(pInput) );

	memset(pInput, 0, 10 * sizeof(char *)); //set all memory to 0;


	pInput[0] = malloc(10);
	printf("Enter X:\n");
	fgets(pInput[0], sizeof(pInput), stdin);
	sscanf(pInput[0], "%lf", &x);

	pInput[1] = malloc(10);
	printf("Enter n:\n");
	fgets(pInput[1], sizeof(pInput), stdin);
	sscanf(pInput[1], "%i", &n);

	printf("Computing %.2lf to the %i power\n", x, n);

	result = power(x, n);

		printf("result: %lf\n", result );

	return 0;
}

double power(double x, int n){
	if( n < 0){
		n = -n;
		x = 1.0/x;
	}
	else if( n == 0){
		return 1.0;
	}
	else{
		;
	}
	return x * power(x, n-1);
}
/* This program converts a decimal value to twos complement and displays it */
#include <stdio.h>
#include <stdlib.h>

int main(void){
	signed int inputNum = 0;
	signed char input;

	//get user input
	printf("Enter a number between -128 and 127\n");
	scanf("%d", &inputNum);
	input = inputNum;

	if((inputNum < -128) || (inputNum > 127))
		abort();

	int i;
	for(i = 7; i >= 0; i--){
		putchar('0'+ ((input >> i) & 1));
	}

	return 0;
}
#include<stdio.h>
#include<unistd.h>
#include"zing.h"

void zing() {
	char* username = getlogin();
	if (username != NULL) {
		printf("Hola, %s!\n", username);
	} else {
		printf("Hola, stranger!\n");
	}
}

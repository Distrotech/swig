#include "example.h"
#include <stdio.h>


void Sync::printer(void) {

	printf("The value of global s is %s\n", s);
	printf("The value of global x is %d\n", x);
	printf("The value of class s is %s\n", s);
	printf("The value of class x is %d\n", x);
};

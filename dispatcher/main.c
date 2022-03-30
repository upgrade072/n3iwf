#include <stdio.h>
#include <stdlib.h>

int main()
{
	fprintf(stderr, "hello?\n");

	for (int i = 0; i < 1000000000; i++) {
		char *ptr = malloc(102400);
		free(ptr);
	}
}

#include <stdio.h>

int main(){
	int i;
	printf("o programa p5 está começando..");
	for(i = 0; i < 20000; i++){
		printf("5");
		if((i / 10) == 0)
			printf("\n");	
	}
	printf("o programa p5 está acabando..");

	return 0;
}

#include <stdio.h>

int main(){
	int i;
	printf("o programa p1 está começando..");
	for(i = 0; i < 1000; i++){
		printf("1");
		if((i / 10) == 0)
			printf("\n");	
	}
	printf("o programa p1 está acabando..");

	return 0;
}

#include <stdio.h>

int main(){
	int i;
	printf("o programa p2 está começando..");
	for(i = 0; i < 30000; i++){
		printf("2");
		if((i / 10) == 0)
			printf("\n");	
	}
	printf("o programa p2 está acabando..");

	return 0;
}

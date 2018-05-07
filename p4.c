#include <stdio.h>

int main(){
	int i;
	printf("o programa p4 está começando..");
	for(i = 0; i < 10000; i++){
		printf("4");
		if((i / 10) == 0)
			printf("\n");	
	}
	printf("o programa p4 está acabando..");

	return 0;
}

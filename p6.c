#include <stdio.h>

int main(){
	int i;
	printf("o programa p6 está começando..");
	for(i = 0; i < 5000; i++){
		printf("6");
		if((i / 10) == 0)
			printf("\n");	
	}
	printf("o programa p6 está acabando..");

	return 0;
}

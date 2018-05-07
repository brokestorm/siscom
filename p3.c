#include <stdio.h>

int main(){
	int i;
	printf("o programa p3 está começando..");
	for(i = 0; i < 8000; i++){
		printf("3");
		if((i / 10) == 0)
			printf("\n");	
	}
	printf("o programa p3 está acabando..");

	return 0;
}

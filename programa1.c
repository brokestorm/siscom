#include <stdio.h>
#include <unistd.h>

#define EVER ;;

int main(){
	int i = 0;
	printf("programa1 comecou\n");
	for(EVER){
		sleep(1);
		i++;
		//printf("O programa1 já foi executado por %d segundos\n", i);
			
	}
	printf("programa1 acabou\n");
	return 0;
}

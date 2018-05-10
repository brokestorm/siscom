#include <stdio.h>
#include <unistd.h>

#define EVER ;;

int main(){
	int i = 0;
	printf("prog_11 comecou\n");
	for(EVER){
		sleep(1);
		i++;
		//printf("O prog_11 já foi executado por %d segundos\n", i);
			
	}

	printf("prog_11 acabou\n");
	return 0;
}

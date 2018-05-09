#include <stdio.h>
#include <unistd.h>

#define EVER ;;

int main(){
	int i = 0;
	for(EVER){
		sleep(1);
		i++;
		printf("O prog_11 já foi executado por %d segundos\n", i);
			
	}

	return 0;
}

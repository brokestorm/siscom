#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define EVER ;;

int main(){
	int i = 0;
	printf("ALO BRASIL COMECOU\n");
	for(EVER){
		sleep(1);
		i++;
		//printf("O ALOBRASIL já foi executado por %d segundos\n", i);
			
	}
	printf("ALO BRASIL ACABOU\n");
	return 0;
}

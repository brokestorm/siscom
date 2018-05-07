#include <stdio.h>
#define EVER ;;

int main(){
	int i = 0;
	for(EVER){
		sleep(1);
		i++;
		print("O programa1 já foi executado por %d segundos\n", i);
			
	}

	return 0;
}

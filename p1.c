#include <stdio.h>
#include <time.h>

#define TAM 5000000

int main(){

	int i, timeBuffer = 0;
	time_t now;
	struct tm *tm;

	now = time(0);
	
	tm = localtime (&now);
	timeBuffer = tm->tm_sec;
	printf("o programa p1 começou.\n");
	
	for(i = 0; i != TAM; i++)
	{
		time(&now);
		tm = localtime(&now);
		if(timeBuffer < tm->tm_sec)
		{
			timeBuffer = tm->tm_sec;
			printf("Oi, eu sou o p1! %d/%d\n", i, TAM);
		}
	}
	
	printf("o programa p1 acabou.\n");

	return 0;
}

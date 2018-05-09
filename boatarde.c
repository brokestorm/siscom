#include <stdio.h>
#include <time.h>

#define TAM 80000000

int main(){

	int i, timeBuffer = 0;
	time_t now;
	struct tm *tm;

	now = time(0);
	
	tm = localtime (&now);
	timeBuffer = tm->tm_sec;
	printf("o programa boatarde começou.\n");
	
	for(i = 0; i < TAM; i++)
	{
		time(&now);
		tm = localtime(&now);
		if(timeBuffer != tm->tm_sec)
		{
			timeBuffer = tm->tm_sec;
			printf ("Current time: %02d:%02d:%02d, %d/%d\n", tm->tm_hour, tm->tm_min, tm->tm_sec, i, TAM);
		}
	}
	
	printf("o programa boatarde acabou.\n");

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define EVER ;;
#define TAM 200

void usr1Handler (int sinal)
{
	printf("Dados enviados\n");
}

void usr2Handler(int sinal)
{
}

int main()
{
	int i = 0, aux = 0, pid;									// auxiliares
	int s = 0, d = 0, pol = 0;		// parametros para o escalonador
 	int prio = 0;									// 1 para REAL TIME, 2 para Prioridade, 0 para ROUND-ROBIN
	char parametro[TAM], nomeDoPrograma[TAM];			// buff de texto do arquivo 
	char character;										// buff de character do arquivo
	FILE *lista;										// arquivo "exec.txt"
	int shdPrio, shdS, shdD, shdPol, segundos, duracao, prioridade, politica;
	
	signal(SIGUSR1, usr1Handler);
	signal(SIGUSR2, usr2Handler);	

	shdPrio = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdS = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdD = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdPol = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	prioridade = (int) shdmat(shdPrio, 0, 0);
	politica = (int) shmat(shdPol, 0, 0);
	segundos = (int) shmat(shdS, 0, 0);
	duracao = (int) shmat(shdD, 0, 0);
	
	if ((pid2 = fork()) == 0) // Interpretador de comandos
	{
		lista = fopen("exec.txt", "r");
		if (lista == NULL) // verificando erros
		{
			printf("Ocorreu um erro ao criar um novo arquivo\n");
			exit(1);
		}
		for (EVER) 
		{
			fscanf(lista, "%c", &character); // lendo cada character do arquivo de texto
			if (aux == 0) 
			{
				nomeDoPrograma[i] = character; // salvando o nome do programa
				if (i >= 1 && nomeDoPrograma[i] == ' ') 
						aux = i; // isso indica que o nome do programa terminou de ser lido
			}
			else if (character == '\n') 
			{
				
				sleep(1);
				prioridade = prio;
				politica = pol; // O valor padrão é ROUND-ROBIN, ou seja, se não identificar nenhuma politica na linha de comando, ele vai passar como ROUND-ROBIN.
				segundos = s;
				duracao = d;
				kill(pid2, SIGUSR1);

				// resetando variaveis....
				aux = 0;
				i = 0;
				pol = 0;

			}
			else 
			{
				i = 0; // agora usando i para indexar o vetor "parametro"
				parametro[i] = character;
				if (parametro[i] == '=') { 
					if (parametro[i - 1] == 'I') { // parametro de segundos
						pol = 1; // politica: REAL TIME
						fscanf(lista, "%d", &s); 
					}
					else if (parametro[i - 1] == 'D') { 
						fscanf(lista, "%d", &d); 
					}
					else if (parametro[i - 1] == 'R') { 
						pol = 2; // politica: PRIORIDADE
						fscanf(lista, "%d", &prio);  
					}
			}
			
			i++;
		}
	}
	fclose(lista);
	}
	//else // Escalonador
	//{
	//	
	//}
	
	// libera a memória compartilhada do processo
	shmdt (prioridade); 
	shmdt (segundos); 
	shmdt (duracao);
	shmdt (politica); 

	// libera a memória compartilhada
	shmctl (shdPrio, IPC_RMID, 0);
	shmctl (shdPol, IPC_RMID, 0);
	shmctl (shdS, IPC_RMID, 0);
	shmctl (shdD, IPC_RMID, 0);

	return 0;
}

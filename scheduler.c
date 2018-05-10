#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#include "lista.h"

#define EVER ;;
#define QUANTUM 1

int main()
{
	Fila *filaRR = criaFila(); 
	Fila *filaRT = criaFila();
	Fila *filaPR = criaFila();
	
	int status;

	int i = 0, j = 0, aux = 0;									// auxiliares
	int s = 0, d = 0, pol = 0;									// parametros para o escalonador
 	int prio = 0;		
															// 1 para REAL TIME, 2 para Prioridade, 0 para ROUND-ROBIN
	char parametro[TAM], nomeDoPrograma[TAM];				// buff de texto do arquivo 
	char character;											// buff de character do arquivo

	FILE *lista;		
											// arquivo "exec.txt"
	int shdPrio, shdS, shdD, shdPol, shdPronto, shdNome;
	int *segundos, *duracao, *prioridade, *politica, *pronto;
	char *nome;

	time_t now;
	struct tm *tm;

	now = time(0);
	if ((tm = localtime (&now)) == NULL) 
	{
   		printf ("Erro pegando o tempo atual.\n");
    		return 1;
	}

	printf ("Tempo de inicio: %04d-%02d-%02d %02d:%02d:%02d\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	shdPrio = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdS = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdD = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdPol = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdNome = shmget(IPC_PRIVATE, TAM*sizeof(char), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shdPronto = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	nome = (char *)shmat(shdNome, 0, 0);
	pronto = (int *) shmat(shdPronto, 0, 0);
	prioridade = (int *) shmat(shdPrio, 0, 0);
	politica = (int *) shmat(shdPol, 0, 0);
	segundos = (int *) shmat(shdS, 0, 0);
	duracao = (int *) shmat(shdD, 0, 0);
	
	*pronto = 0;

	/********** INTERPRETADOR **********/
	if (fork() != 0)
	{	
		lista = fopen("exec.txt", "r");
		if (lista == NULL) // verificando erros
		{
			printf("Interpretador: Ocorreu um erro ao abrir o arquivo\n");
			exit(1);
		}

		fscanf(lista, "Exec ");	

		while (fscanf(lista, "%c", &character) != EOF) 
		{	
			if (aux == 0) 
			{
				nomeDoPrograma[i] = character; // salvando o nome do programa
				if (nomeDoPrograma[i] == ' ' || nomeDoPrograma[i] == '\n')
				{
					aux++; // isso indica que o nome do programa terminou de ser lido
					nomeDoPrograma[i] = '\0';
					i = 0;
				}
			}

			if (character == '\n' && aux == 1) 
			{
				memset(nome, 0, TAM*sizeof(char)); // Esvaziando a variável do nome
	
				for(j = 0; nomeDoPrograma[j] != '\0'; j++)
				{
					nome[j] = nomeDoPrograma[j]; // Colacando o nome logo após o diretorio "./"
				}
				
				nome[j] = '\0';
				
				*prioridade = prio;
				*politica = pol; // O valor padrão é ROUND-ROBIN, ou seja, se não identificar nenhuma politica na linha de comando, ele vai passar como ROUND-ROBIN.
				*segundos = s;
				*duracao = d;
				*pronto = 1;

				sleep(1);
				fscanf(lista, "Exec ");	

				// resetando variaveis....
				memset(nomeDoPrograma, 0, sizeof(nomeDoPrograma)); // Esvaziando a variável do nome
				aux = 0;
				i = -1;
				pol = 0;
				s = 0;
				d = 0;
				prio = 0;
			}
			
			if(character != '\n' && aux == 1)
			{ 
				// agora usando i para indexar o vetor "parametro"
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
			}
			i++;
			
		}
		fclose(lista);
	}
	
	/********** ESCALONADOR **********/
	else 
	{	
		int timeBuffer = tm->tm_sec;
		int timeline = -1;                      // Essa variavel sera usada para olhar em que segundo estamos aos olhos do escalonador (ela vai de 0 a 59)
		int RTisExecuting = 0;
		int PRisExecuting = 0;
		int RRisExecuting = 0;
		int sinceLastPreemp = 0;                // Aqui guardamos quanto tempo tem desde o ultimo quantum/preempção 
		int timeForNextRT = 60;
		
		int shdPid;
		int *shPid;
		
		// Alguns nos auxiliares, para fazermos operações e sabermos quem é o processo atual rodando
		No *PRAtual = NULL;
		No *RTAtual = NULL;
		No *RTAux = NULL;
		
		shdPid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
		
		shPid = shmat(shdPid, 0, 0);
		
		*shPid = -42; // -42 é um valor aleatório que servira para checar se shPid ja foi preenchido ou nao

		for(EVER)
		{
			time(&now);
			tm = localtime(&now);

			// Caso exista, adiciona processos nas respectivas filas
			if(*pronto == 1)
			{
				if(*politica == 0) 			// ROUND-ROBIN
				{
					printf("ESCALONADOR: Eu, programa %s, entrei na fila Round robin\n", nome);
					inserirRR(filaRR, nome, 0, 0);
				}
				else if(*politica == 1) 		// REAL-TIME
				{
					
					RTAux = inserirRT(filaRT, *segundos, *duracao, nome);
					if(RTAux != NULL)
					{
						printf("ESCALONADOR: Eu, programa %s, entrei na fila Real Time\n", nome);
					 	if(RTAux->segundos >= timeline && RTAux->segundos < timeForNextRT)
					 	{
							timeForNextRT = RTAux->segundos;
							RTAtual = RTAux;
						}
					}
					
				}
				else 					// PRIORIDADE
				{	
					printf("ESCALONADOR: Eu, programa %s, entrei na fila Prioridade\n", nome);
					inserirPR(filaPR, *prioridade, nome);
					
					if(!PRisExecuting)
						PRAtual = filaPR->inicial;
				}

				*pronto = 0;
			}
			
			/*
				Aqui começa o escalonador de verdade, fazemos as operações a cada segundo.
				Operaçoes significa checar quem é o processo que deveria estar rodando no momento.
			*/
			if(timeBuffer != tm->tm_sec)
			{
				timeBuffer = tm->tm_sec;
				timeline++;
				if(timeline == 60)
				{
					printf("PASSARAM-SE 60 SEG, O CICLO RESETOU\n");
					timeline = 0;
				}
				
				/***** CHECKS DE REAL TIME *****/
				// Se um processo Real time estiver executando ficamos checando se ele chegou ao seu fim de duração para pará-lo
				if(RTisExecuting)
				{
					if (timeline == RTAtual->segundos + RTAtual->duracao)
					{
						kill(RTAtual->pid, SIGSTOP);
						RTAtual = RTAtual->prox;
						timeForNextRT = RTAtual->segundos;
						RTisExecuting = 0;
					}
				}
				
				// Se o segundo atual é igual ao segundo de inicio do proximo próximo Real Time
				if(timeline == timeForNextRT)
				{
					// Paramos qualquer processo de prioridade
					if(PRisExecuting)
					{
						kill(PRAtual->pid, SIGSTOP);
						PRisExecuting = 0;
					}
					// Ou paramos processo de Round Robin
					else if(RRisExecuting)
					{
						kill(filaRR->inicial->pid, SIGSTOP);
						RRisExecuting = 0;
					}
					
					// Garantindo que tem alguém na fila de RT para rodar
					if(filaRT->size != 0)
					{
						// Se ele já rodou pelo menos uma vez apenas damos um SIGCONT e avisamos
						if(RTAtual->count > 0)
						{
							kill(RTAtual->pid, SIGCONT);
							RTisExecuting = 1;
							printf("ESCALONADOR: O programa %s (RT) foi reescalonado no tempo %d!\n", RTAtual->nomeDoPrograma, timeline);
						}
						// Se essa é a sua primeira vez rodando, damos fork, armazenamos seu pid e damos execv
						else
						{
							if(fork() != 0)
							{
								printf("ESCALONADOR: O programa %s (RT) foi escalonado no tempo %d!\n", RTAtual->nomeDoPrograma, timeline);
								*shPid = getpid();
								if(execve(RTAtual->nomeDoPrograma, NULL, NULL) == -1)
								{
									printf("ESCALONADOR: Ocorreu algum erro ao executar %s (RT)!\n", RTAtual->nomeDoPrograma);
									*shPid = -1;
								}
							}
							while(*shPid == -42);
							if(*shPid != -1)
							{
								RTAtual->pid = *shPid;
								*shPid = -42;
								RTisExecuting = 1;
								RTAtual->count++;
							}
						}
						
						
					}
				}/***** Fim dos checks para Real Time *****/
				
				
				/* Checando se algum programa de RR ou PR chegou ao seu fim para podermos remove-lo das suas listas */
				if(PRisExecuting)
				{
					if(waitpid(PRAtual->pid, &status, WNOHANG) == PRAtual->pid)
					{
						printf("ESCALONADOR: O programa %s (PR) acabou e foi retirado da sua fila\n", PRAtual->nomeDoPrograma);
						PRAtual = filaPR->inicial;
						removeMeioPR(filaPR, PRAtual);
						PRisExecuting = 0;
					}
				}
				
				if(RRisExecuting)
				{
					if(waitpid(filaRR->inicial->pid, &status, WNOHANG) == filaRR->inicial->pid)
					{
						printf("ESCALONADOR: O programa %s (RR) acabou e foi retirado da sua fila\n", filaRR->inicial->nomeDoPrograma);
						removePrimeiro(filaRR);
						RRisExecuting = 0;
					}
				}
				
				/***** CHECKS DE PRIORIDADE *****/
				if(filaPR->size != 0 && !RTisExecuting)
				{	
					// Se tem algum processo Round Robin ativo, paramos ele para dar espaço pro de prioridade
					if(RRisExecuting)
					{
						kill(filaRR->inicial->pid, SIGSTOP);
						RRisExecuting = 0;
					}
					
					// Se não tiver ninguém de Prioridade executando já, começamos a executar
					if(!PRisExecuting)
					{
						if(PRAtual->count > 0)
						{
							kill(PRAtual->pid, SIGCONT);
							PRisExecuting = 1;
							printf("ESCALONADOR: O programa %s (PR) foi reescalonado no tempo %d!\n", PRAtual->nomeDoPrograma, timeline);
						}
						else
						{
							if(fork() != 0)
							{
								printf("ESCALONADOR: O programa %s (PR) foi escalonado no tempo %d!\n", PRAtual->nomeDoPrograma, timeline);
								*shPid = getpid();
								if(execve(PRAtual->nomeDoPrograma, NULL, NULL)==-1)
								{
									printf("ESCALONADOR: Ocorreu algum erro ao executar %s (PR)!\n", PRAtual->nomeDoPrograma);
									*shPid = -1;
								}
							}
							while(*shPid == -42);
							if(*shPid != -1)
							{
								PRAtual->pid = *shPid;
								*shPid = -42;
								PRisExecuting = 1;
								PRAtual->count++;
							}
						}
					} 
				} /***** Fim dos checks para Prioridade *****/
				/***** CHECKS DE ROUND ROBIN *****/
				else if(filaRR->size != 0 && !RTisExecuting)
				{
					sinceLastPreemp++;
					// Se tem um round robin executando e chegamos no tempo de quantum, fazemos a preemptamos
					if(RRisExecuting && sinceLastPreemp == QUANTUM)
					{
						kill(filaRR->inicial->pid, SIGSTOP);
						inserirRR(filaRR, filaRR->inicial->nomeDoPrograma, ++filaRR->inicial->count, filaRR->inicial->pid);
						removePrimeiro(filaRR);
						sinceLastPreemp = 0;
						RRisExecuting = 0;
					}
					
					// Se não tem RR executando, mandamos executar
					if(!RRisExecuting)
					{
						if(filaRR->inicial->count > 0)
						{
							kill(filaRR->inicial->pid, SIGCONT);
							RRisExecuting = 1;
							filaRR->inicial->count++;
							printf("ESCALONADOR: O programa %s (RR) foi reescalonado no tempo %d!\n", filaRR->inicial->nomeDoPrograma, timeline);
						}
						else
						{
							if(fork() != 0)
							{
								printf("ESCALONADOR: O programa %s (RR) foi escalonado no tempo %d!\n", filaRR->inicial->nomeDoPrograma, timeline);
								*shPid = getpid();
								if(execve(filaRR->inicial->nomeDoPrograma, NULL, NULL)==-1)
								{
									printf("ESCALONADOR: Ocorreu algum erro ao executar %s (RR)!\n", filaRR->inicial->nomeDoPrograma);
									*shPid = -1;
								}
								
							}
							while(*shPid == -42);
							if(*shPid != -1)
							{
								filaRR->inicial->pid = *shPid;
								*shPid = -42;
								RRisExecuting = 1;
								filaRR->inicial->count++;
							}
						}
					} 
				}/***** Fim dos checks para Round Robin *****/
				
				
			} // endif(timebuffer)
		
		} // fim for(EVER)
		
		shmdt(shPid);
		
		shmctl(shdPid, IPC_RMID, 0);
	}// Fim do escalonador
	waitpid(-1, &status, 0);
	
	// libera a memória compartilhada do processo
	shmdt (prioridade); 
	shmdt (segundos); 
	shmdt (duracao);
	shmdt (politica);
	shmdt (nome);
	shmdt (pronto); 

	// libera a memória compartilhada
	shmctl (shdPrio, IPC_RMID, 0);
	shmctl (shdPol, IPC_RMID, 0);
	shmctl (shdS, IPC_RMID, 0);
	shmctl (shdD, IPC_RMID, 0);
	shmctl (shdNome, IPC_RMID, 0);
	shmctl (shdPronto, IPC_RMID, 0);

	return 0;
}
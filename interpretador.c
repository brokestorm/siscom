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

#define EVER ;;
#define TAM 200
#define QUANTUM 1

struct no
{
	char *nomeDoPrograma;
	int prioridade;
	int segundos;
	int duracao;
	int pid;
	int count;
	
	struct no *prox;

} typedef No;


struct fila
{
	No *inicial;
	No *final;
	int size;
} typedef Fila;


Fila* criaFila()
{
	Fila *p = (Fila*) malloc(sizeof(Fila));
	if( p == NULL)
	{
		return NULL;
	}

	p->inicial = NULL;
	p->final = NULL;
	p->size = 0;

	return p;
}


// Round Robin funciona como uma fila normal, apenas insere no final e remove no primeiro
int inserirRR(Fila *p, char* nome, int count)
{
	No *novo = (No*) malloc(sizeof(No));

	novo->nomeDoPrograma = nome;
	novo->prox = NULL;
	novo->count = count;

	// Se a fila está vazia
	if(p->size == 0)
	{
		p->inicial = novo;
		p->final = novo;
		p->size++;
		return 0;
	}
	else
	{
		p->final = novo;
		p->size++;
		return 0;
	}

	return 1;
}

// RT vamos modelar como uma fila circular, já que RT cicla pelos processos pra sempre. E é possivel inserir em qualquer lugar da fila
No* inserirRT(Fila *p, int seg, int dur, char* nome)
{
	No *novo = (No *)malloc(sizeof(No));
	No *b, *aux;
	int tempoFinalAtual, tempoFinalNovo;


	if(seg+dur > 60)
	{
		printf("Tempo de duração do processo %s é maior que 60 segundos\n", nome);
		return NULL;	
	}

	novo->nomeDoPrograma = nome;
	novo->segundos = seg;
	novo->duracao = dur;
	novo->count = 0;
	
	// Se a fila está vazia
	if(p->size == 0)
	{
		novo->prox = novo;
		p->inicial = novo;
		p->final = novo;
		p->size++;
		return novo;
	}

	tempoFinalNovo = seg + dur;

	// Se o processo novo roda antes do primeiro da fila
	if(tempoFinalNovo < p->inicial->segundos) 
	{
		novo->prox = p->inicial;
		p->inicial = novo;
		p->final->prox = novo;
		p->size++;
		return novo;
	}
	
	// Se o processo novo roda depois do ultimo da fila
	if(novo->segundos > (p->final->segundos + p->final->duracao) )
	{
		novo->prox = p->inicial;
		p->final->prox = novo;
		p->final = novo;
		p->size++;
		return novo;
	}
	// Se o processo novo esta no meio da fila
	for(b = p->inicial->prox; b->prox != p->final; b = b->prox)
	{
		tempoFinalAtual = b->segundos + b->duracao;
	
		if( (tempoFinalAtual < tempoFinalNovo) && (tempoFinalNovo < b->prox->segundos) )
		{
			aux = b->prox;
			b->prox = novo;
			novo->prox = aux;
			p->size++;
			return novo;
		}
	}

	// Caso não tenha conseguido encaixar, é pq ele conflitava com algum processo existente da fila
	return NULL;
}


// PR é uma fila, onde só removemos o primeiro, porém é possível inserir em qualquer ponto
int inserirPR(Fila *p, int prio, char* nome)
{
	No *novo = (No *)malloc(sizeof(No));
	No *b, *aux;

	novo->nomeDoPrograma = nome;
	novo->prioridade = prio;

	// Se a fila está vazia
	if(p->size == 0)
	{
		novo->prox = novo;
		p->inicial = novo;
		p->final = novo;
		p->size++;
		return 0;
	}

	// Caso o processo a ser inserido tenha a melhor prioridade de todas
	if(novo->prioridade < p->inicial->prioridade)
	{
		novo->prox = p->inicial;
		p->inicial = novo;
		p->size++;
		return 0;	
	}

	// Caso o processo a ser inserido tenha a pior prioridade de todas
	if(novo->prioridade >= p->final->prioridade)
	{
		p->final->prox = novo;
		novo->prox = NULL;
		p->size++;
		return 0;
	}

	// Caso esteja no meio (entre maior e menor prioridades), procura seu lugar na fila
	for(b = p->inicial; b->prox != p->final; b = b->prox)
	{
		if(b->prox->prioridade > novo->prioridade)
		{
			aux = b->prox;
			b->prox = novo;
			novo->prox = aux;
			p->size++;
			return 0;		
		}
	}

	
	// Se tudo der errado...
	return 1;
}

void removePrimeiro(Fila *p)
{
	No* aux = p->inicial;
	p->inicial = p->inicial->prox;

	p->size--;
	free(aux);
}


int main()
{
	Fila *filaRR = criaFila(), *filaRT = criaFila(), *filaPR = criaFila();

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

	int iniTime;
	time_t now;
	struct tm *tm;

	now = time(0);
	if ((tm = localtime (&now)) == NULL) 
	{
   		printf ("Error extracting time stuff\n");
    		return 1;
	}

	printf ("Current time: %04d-%02d-%02d %02d:%02d:%02d\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	iniTime = tm->tm_sec;

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
			printf("Ocorreu um erro ao abrir o arquivo\n");
			exit(1);
		}

		fscanf(lista, "Exec ");	

		while (fscanf(lista, "%c", &character) != EOF) 
		{	
			//printf("%c", character);
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
		int timeBuffer = tm->tm_sec, pid;
		int timeline = 0;
		char *arg;
		int RTisExecuting = 0;
		int PRisExecuting = 0;
		int RRisExecuting = 0;
		int sinceLastPreemp = 0;
		No *RTAtual = NULL;
		No *RTAux = NULL;
		int timeForNextRT = 60;

		for(EVER)
		{
			time(&now);
			tm = localtime(&now);

			// Caso exista, adiciona processos nas respectivas filas
			if(*pronto == 1)
			{
				if(*politica == 0) 			// ROUND-ROBIN
				{
					printf("Eu sou o processo %s, politica: %d\n", nome, *politica);
					inserirRR(filaRR, nome, 0);	
				}
				else if(*politica == 1) 		// REAL-TIME
				{
					RTAux = inserirRT(filaRT, *segundos, *duracao, nome);
					if(RTAux != NULL)
					{
					 	if(*segundos >= timeline && *segundos < timeForNextRT)
					 	{
							timeForNextRT = *segundos;
							RTAtual = RTAux;
						}
					}
					
				}
				else 					// PRIORIDADE
				{	
					inserirPR(filaPR, *prioridade, nome);
				}

				*pronto = 0;
			}
			
			
			if(timeBuffer != tm->tm_sec)
			{
				timeBuffer = tm->tm_sec;
				timeline++;
				printf("timeline = %ds\n", timeline);
				if(tm->tm_sec == iniTime)
				{
					timeline = 0;
					printf("Timeline foi resetada!\n");
				}
				
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
				
				if(timeline == timeForNextRT)
				{
					if(PRisExecuting)
					{
						kill(filaPR->inicial->pid, SIGSTOP);
						PRisExecuting = 0;
					}
					else if(RRisExecuting)
					{
						kill(filaRR->inicial->pid, SIGSTOP);
						RRisExecuting = 0;
					}

					if(filaRT->size != 0)
					{
						if(RTAtual->count > 0)
						{
							kill(RTAtual->pid, SIGCONT);
							printf("O programa %s foi escalonado!\n", RTAtual->nomeDoPrograma);
						}
						else
						{
							pid = fork();
							if(pid != 0)
							{
								RTAtual->pid = pid;
								if(execve(RTAtual->nomeDoPrograma, NULL, NULL) == -1)
									printf("Ocorreu algum erro ao executar %s!\n", RTAtual->nomeDoPrograma);
								else
									printf("O programa %s foi escalonado!\n", RTAtual->nomeDoPrograma);
							}
						}
						RTisExecuting = 1;
						RTAtual->count++;
					}
				}
				
				if(filaPR->size != 0 && !RTisExecuting)
				{	
					if(RRisExecuting)
					{
						kill(filaRR->inicial->pid, SIGSTOP);
						RRisExecuting = 0;
					}
					
					if(!PRisExecuting)
					{
						if(filaPR->inicial->count > 0)
						{
							kill(filaPR->inicial->pid, SIGCONT);
							printf("O programa %s foi escalonado!\n", filaPR->inicial->nomeDoPrograma);
						}
						else{
							pid = fork();
							if(pid != 0)
							{
								filaPR->inicial->pid = pid;
								if(execve(filaPR->inicial->nomeDoPrograma, NULL, NULL)==-1)
									printf("Ocorreu algum erro ao executar %s!\n", filaPR->inicial->nomeDoPrograma);
								else
									printf("O programa %s foi escalonado!\n", filaPR->inicial->nomeDoPrograma);
							}
						}
						PRisExecuting = 1;
						filaPR->inicial->count++;
						printf("O programa %s foi escalonado!\n", filaPR->inicial->nomeDoPrograma);
					} 
				} 
				else if(filaRR->size != 0 && !RTisExecuting)
				{
					sinceLastPreemp++;
					if(RRisExecuting && sinceLastPreemp == QUANTUM)
					{
						kill(filaRR->inicial->pid, SIGSTOP);
						inserirRR(filaRR, filaRR->inicial->nomeDoPrograma, ++filaRR->inicial->count);
						removePrimeiro(filaRR);
						sinceLastPreemp = 0;
						RRisExecuting = 0;
					}
					
					if(!RRisExecuting)
					{
						if(filaRR->inicial->count > 0)
						{
							kill(filaRR->inicial->pid, SIGCONT);
							printf("O programa %s foi reescalonado!\n", filaRR->inicial->nomeDoPrograma);
						}
						else{
							pid = fork();
							if(pid != 0)
							{
								filaRR->inicial->pid = pid;
								if(execve(filaRR->inicial->nomeDoPrograma, NULL, NULL)==-1)
									printf("Ocorreu algum erro ao executar %s!\n", filaRR->inicial->nomeDoPrograma);
								else
									printf("O programa %s foi escalonado!\n", filaRR->inicial->nomeDoPrograma);
							}
						}
						RRisExecuting = 1;
						printf("O programa %s foi escalonado!\n", filaRR->inicial->nomeDoPrograma);
					} 
				}
				
				
			} // endif(timebuffer)
		}
	}
	
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include<sys/stat.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#define EVER ;;
#define TAM 200

struct no
{
	char *nomeDoPrograma;
	int prioridade;
	int segundos;
	int duracao;
	int pid;
	int taVazia;   // Esse elemento é um pouco gambiarrento, ele está aqui apenas para a primeira entrada de elemento
	
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
int inserirRR(Fila *p, char* nome)
{
	No *novo = (No*) malloc(sizeof(No));

	novo->nomeDoPrograma = nome;
	novo->prox = NULL;

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
int inserirRT(Fila *p, int seg, int dur, char* nome)
{
	No *novo = (No *)malloc(sizeof(No));
	No *b, *aux;
	int tempoFinalAtual, tempoFinalNovo;


	if(seg+dur > 60)
	{
		printf("Tempo de duração do processo %s é maior que 60 segundos\n", nome);
		return 1;	
	}

	novo->nomeDoPrograma = nome;
	novo->segundos = seg;
	novo->duracao = dur;
	
	// Se a fila está vazia
	if(p->size == 0)
	{
		novo->prox = novo;
		p->inicial = novo;
		p->final = novo;
		p->size++;
		return 0;
	}

	tempoFinalNovo = seg + dur;

	// Se o processo novo roda antes do primeiro da fila
	if(tempoFinalNovo < p->inicial->segundos) 
	{
		novo->prox = p->inicial;
		p->inicial = novo;
		p->final->prox = novo;
		p->size++;
		return 0;
	}
	
	// Se o processo novo roda depois do ultimo da fila
	if(novo->segundos > (p->final->segundos + p->final->duracao) )
	{
		novo->prox = p->inicial;
		p->final->prox = novo;
		p->final = novo;
		p->size++;
		return 0;
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
			return 0;
		}
	}

	// Caso não tenha conseguido encaixar, é pq ele conflitava com algum processo existente da fila
	return 1;
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

	free(aux);
}

int minorCompareTime(struct tm *a, struct tm *b)
{
	if(a->tm_min < b->tm_min)
		return 0; 
	else if(a->tm_min > b->tm_min)
		return 1;
	else
	{	if(a->tm_sec < b->tm_sec)
			return 0;
		else if(a->tm_sec > b->tm_sec)
			return 1;
		else
			return 1;
		
	}
}

int equalCompareTime(struct tm *a, struct tm *b)
{
	if(a->tm_min < b->tm_min)
		return 1; 
	else if(a->tm_min > b->tm_min)
		return 1;
	else
	{	if(a->tm_sec < b->tm_sec)
			return 1;
		else if(a->tm_sec > b->tm_sec)
			return 1;
		else
			return 0;
		
	}
}

int setTime(int sec1, int sec2, struct tm *a)
{
	a->tm_year = 0;
	a->tm_mon = 0;
	a->tm_mday = 0;
   	a->tm_hour = 0;
	a->tm_min = (sec1 + sec2) / 60;
	a->tm_sec = (sec1 + sec2) % 60;
}

int main()
{
	Fila *filaRR = criaFila(), *filaRT = criaFila(), *filaPR = criaFila();

	int i = 0, j = 0, aux = 0;									// auxiliares
	int s = 0, d = 0, pol = 0;									// parametros para o escalonador
 	int prio = 0;		
															// 1 para REAL TIME, 2 para Prioridade, 0 para ROUND-ROBIN
	char parametro[TAM], nomeDoPrograma[TAM], *nomeAux;				// buff de texto do arquivo 
	char diretorioDosProgramas[] = "./";
	char character;											// buff de character do arquivo
	int tamanhoDoNome, status;

	FILE *lista;		
											// arquivo "exec.txt"
	int shdPrio, shdS, shdD, shdPol, shdPronto, shdNome;
	int *segundos, *duracao, *prioridade, *politica, *pronto;
	char *nome;

	int iniTime;
	time_t now;
	struct tm *tm;
	struct tm *tv;

	now = time(0);
	if ((tm = localtime (&now)) == NULL) 
	{
   		printf ("Error extracting time stuff\n");
    		return 1;
	}

	printf ("Current time: %04d-%02d-%02d %02d:%02d:%02d\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	iniTime = tm->tm_sec;

	printf("Initial seconds: %ds\n", iniTime);

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
		
		fseek(lista, 5, SEEK_SET); // Pulando o "Exec "
		
		while (fscanf(lista, "%c", &character) != EOF) 
		{	
			//printf("%c", character);
			if (aux == 0 && character != '\n') 
			{
				nomeDoPrograma[i] = character; // salvando o nome do programa

				if (nomeDoPrograma[i] == ' ')
				{
					aux++; // isso indica que o nome do programa terminou de ser lido
					tamanhoDoNome = i;
					i = 0;
				}
			}
			else if (character == '\n') 
			{
				nomeAux = (char*) malloc(2+(tamanhoDoNome*sizeof(char)));
				strcpy(nomeAux, diretorioDosProgramas); // Colocando o diretorio no nome

				for(j = 0; j < tamanhoDoNome; j++)
				{
					nomeAux[2+j] = nomeDoPrograma[j]; // Colacando o nome logo após o diretorio "./"
				}
				
				nome = nomeAux; // Ja enviando o nome com o diretório
				*prioridade = prio;
				*politica = pol; // O valor padrão é ROUND-ROBIN, ou seja, se não identificar nenhuma politica na linha de comando, ele vai passar como ROUND-ROBIN.
				*segundos = s;
				*duracao = d;
				*pronto = 1;

				sleep(1);
				
				fseek(lista, 5, SEEK_CUR); // Pulando o "Exec "

				// resetando variaveis....
				memset(nomeDoPrograma, 0, sizeof(nomeDoPrograma)); // Esvaziando a variável do nome
				aux = 0;
				i = -1;
				pol = 0;
				s = 0;
				d = 0;
				prio = 0;
			}
			else 
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
		char *arg;
		int RTisExecuting;
		int PRisExecuting;
		int RRisExecuting;

		for(EVER)
		{
			time(&now);
			tm = localtime(&now);

			// Caso exista, adiciona processos nas respectivas filas
			if(*pronto == 1)
			{
				if(*politica == 0) 			// ROUND-ROBIN
				{
					printf("Sou RR\n");
					inserirRR(filaRR, nome);	
				}
				else if(*politica == 1) 		// REAL-TIME
				{
					printf("Sou RT\n");
					inserirRT(filaRT, *segundos, *duracao, nome);
				}
				else 					// PRIORIDADE
				{	
					printf("Sou PR\n");
					inserirPR(filaPR, *prioridade, nome);
				}
				*pronto = 0;
				printf("Alterado!\n");

			}

			if(timeBuffer != tm->tm_sec)
			{
				timeBuffer = tm->tm_sec;
				if(filaRT != NULL){ // NEEDS OTHER CONDITIONS!!!!!!!!
			//		pid = fork();
			//		if(pid != 0)
			//		{
			//			filaRT->pid = pid;
			//			//execv(filaRT->nomeDoPrograma, &arg);
			//		}
				}
				else if(filaPR != NULL)
				{	
					printf("programa vai ser %s executado!\n", filaPR->inicial->nomeDoPrograma);
					pid = fork();
					if(pid != 0)
					{
						filaPR->pid = pid;
						execv(filaPR->inicial->nomeDoPrograma, &arg);
					}
				} 
				else if(filaRR != NULL)
				{
					printf("programa vai ser %s executado!\n", filaRR->nomeDoPrograma);
					pid = fork();
					if(pid != 0)
					{
						filaRR->pid = pid;
						execv(filaRR->inicial->nomeDoPrograma, &arg);
					}

				}
			}
		}
	}
	
	waitpid(-1, &status, 0);
	
	// libera a memória compartilhada do processo
	shmdt (prioridade); 
	shmdt (segundos); 
	shmdt (duracao);
	shmdt (politica);
	shmdt (pronto); 

	// libera a memória compartilhada
	shmctl (shdPrio, IPC_RMID, 0);
	shmctl (shdPol, IPC_RMID, 0);
	shmctl (shdS, IPC_RMID, 0);
	shmctl (shdD, IPC_RMID, 0);
	shmctl (shdPronto, IPC_RMID, 0);

	return 0;
}

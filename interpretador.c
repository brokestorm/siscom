#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include<sys/stat.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define EVER ;;
#define TAM 200

struct fila
{
	char *nomeDoPrograma;
	int prioridade;
	int segundos;
	int duracao;
	int pid;
	
	struct fila *prox;

} typedef Fila;

int inserirRR(Fila *p, char* nome)
{
	Fila *novo = (Fila *)malloc(sizeof(Fila));
	Fila *b;

	novo->nomeDoPrograma = nome;

	// Se a fila está vazia
	if(p ==NULL)
	{
		p = novo;
		return 0;
	}
	// Caso contrário vai para o final e insere
	for(b = p; b->prox != NULL; b = b->prox);

	b->prox = novo;
	return 0;
}

int inserirRT(Fila *p, int seg, int dur, char* nome)
{
	Fila *novo = (Fila *)malloc(sizeof(Fila));
	Fila *b, *aux;
	int tempoFinalAtual, tempoFinalNovo;

	if(seg+dur > 60)
	{
		printf("Tempo de duração do processo é maior que 60 segundos\n");
		return 1;	

	}

	novo->nomeDoPrograma = nome;
	novo->segundos = seg;
	novo->duracao = dur;
	
	// Se a fila está vazia
	if(p == NULL)
	{
		p = novo;
		return 0;
	}

	// Tentando encaixar o novo processo entre os já existentes
	for(b = p; b->prox != NULL; b = b->prox)
	{
		tempoFinalAtual = b->segundos + b->duracao;
		tempoFinalNovo = seg + dur;

		if( tempoFinalAtual < tempoFinalNovo && tempoFinalNovo < b->prox->segundos )
		{
			aux = b->prox;
			b->prox = novo;
			novo->prox = aux;
			return 0;
		}
	}

	// Caso não tenha conseguido encaixar, é pq ele conflitava com algum processo existente da fila
	return 1;
}

int inserirPR(Fila *p, int prio, char* nome)
{

	Fila *novo = (Fila *)malloc(sizeof(Fila));
	Fila *b, *aux;

	novo->nomeDoPrograma = nome;
	novo->prioridade = prio;

	// Se a fila está vazia
	if(p == NULL)
	{
		p = novo;
		return 0;
	}

	// Caso o processo a ser inserido tenha maior prioridade de todas
	if(p->prioridade > prio)
	{
		novo->prox = p;
		p = novo;	
		return 0;	
	}

	// Caso esteja no meio (entre maior e menor prioridades), procura seu lugar na fila
	for(b = p; b->prox != NULL; b = b->prox)
	{
		if(b->prox->prioridade > prio)
		{
			aux = b->prox;
			b->prox = novo;
			novo->prox = aux;
			return 0;		
		}
	}

	// Caso tenha a pior prioridade da fila, apenas vai para o final
	if(b->prox == NULL && b->prioridade < prio)
	{
		novo->prox = NULL;
		b->prox = novo;
		return 0;
	}

	// Se tudo der errado...
	return 1;
}

void removePrimeiro(Fila *p)
{
	Fila* aux = p;
	p = p->prox;

	free(aux);
}

int main()
{
	Fila *filaRR = NULL, *filaRT = NULL, *filaPR = NULL;

	int i = 0, j = 0, aux = 0;												// auxiliares
	int s = 0, d = 0, pol = 0;										// parametros para o escalonador
 	int prio = 0;													// 1 para REAL TIME, 2 para Prioridade, 0 para ROUND-ROBIN

	char parametro[TAM], nomeDoPrograma[TAM], *nomeAux;				// buff de texto do arquivo 
	char diretorioDosProgramas[] = "./";
	char character;													// buff de character do arquivo
	int tamanhoDoNome;

	FILE *lista;													// arquivo "exec.txt"

	int shdPrio, shdS, shdD, shdPol, shdPronto, shdNome;
	int *segundos, *duracao, *prioridade, *politica, *pronto;
	char *nome;
	

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
		int pid;
		char arg;

		for(EVER)
		{
			// Caso exista, adiciona processos nas respectivas filas
			if(*pronto == 1)
			{
				if(*politica == 0) // ROUND-ROBIN
				{
					printf("Sou RR\n");
					inserirRR(filaRR, nome);	
				}
				else if(*politica == 1) // REAL-TIME
				{
					printf("Sou RT\n");
					inserirRT(filaRT, *segundos, *duracao, nome);
				}
				else // PRIORIDADE
				{
					inserirPR(filaPR, *prioridade, nome);
				}
				*pronto = 0;
				printf("Alterado!\n");
			}

			if(filaRT != NULL)
			{
				pid = fork();
				if(pid != 0)
				{
					filaRT->pid = pid;
					//execv(filaRT->nomeDoPrograma, &arg);
				}
			}
			else if(filaPR != NULL)
			{

			}
			else if(filaRR != NULL)
			{

			}
			else
			{
				//printf("Não há processos para executar no momento!\n");
			}

		}
	}

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

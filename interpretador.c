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
	
	FIla *prox;

} typedef Fila;

void inserirRR(Fila *p, char* nome)
{
	Fila *novo = (Fila *)malloc(sizeof(Fila));
	Fila *b;

	novo->nomeDoPrograma = nome;

	for(b = p; b->prox != NULL; b = b->prox);
	b->prox = novo;
}

int inserirRT(Fila *p, int seg, int dur, char* nome)
{
	Fila *novo = (Fila *)malloc(sizeof(Fila));
	Fila *b;

	if(seg+dur > 60)
	{
		printf("Tempo de duração do processo é maior que 60 segundos\n");
		return -1;	
	}
	novo->nomeDoPrograma = nome;
	novo->segundos = seg;
	novo->duracao = dur;
	
	for(b = p; b->prox != NULL; b = b->prox)
	{
		if( (b->segundos <= seg 	      && (b->duracao + b->segundos) >= seg) ||
		    (b->segundos <= (seg + dur) && (b->duracao + b->segundos) >= (seg + dur)) ) // se o tempo inicial OU o tempo final estiver no intervalo, cancela o programa
			return -1;
	}
	b->prox = novo;

	return 0;
}

void inserirPR(Fila *p, int prio, char* nome)
{
	Fila *novo = (Fila *)malloc(sizeof(Fila));
	Fila *b, *aux;

	novo->nomeDoPrograma = nome;
	novo->prioridade = prio;

	for(b = p; b->prox != NULL; b = b->prox)
	{
		if(b->prioridade > prio)
		{
			novo->prox = b;	
			return;	
		}
		else if(b->prox->prioridade > prio)
		{
			aux = b->prox;
			b->prox = novo;
			novo->prox = aux;
			return;		
		}
	}
	b->prox = novo;
}

int main()
{
	Fila *filaRR, *filaRT, *filaPR;
	int i = 0, aux = 0;						// auxiliares
	int s = 0, d = 0, pol = 0;						// parametros para o escalonador
 	int prio = 0;									// 1 para REAL TIME, 2 para Prioridade, 0 para ROUND-ROBIN
	char parametro[TAM], nomeDoPrograma[TAM];		// buff de texto do arquivo 
	char character;									// buff de character do arquivo
	FILE *lista;									// arquivo "exec.txt"
	int shdPrio, shdS, shdD, shdPol, shdPronto, shdNome;
	int *segundos, *duracao, *prioridade, *politica, *pronto;
	char *nome;
	char *filaRT[TAM];
	char *filaRR[TAM];
	char *filaPR[TAM];
	int fimRT, fimRR;


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
	if (fork() != 0) // Interpretador de comandos
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
			printf("%c", character);
			if (aux == 0 && character != '\n') 
			{
				nomeDoPrograma[i] = character; // salvando o nome do programa

				if (nomeDoPrograma[i] == ' ')
				{
					aux++; // isso indica que o nome do programa terminou de ser lido
					i = 0;
				}
			}
			else if (character == '\n') 
			{
				nome = nomeDoPrograma;
				*prioridade = prio;
				*politica = pol; // O valor padrão é ROUND-ROBIN, ou seja, se não identificar nenhuma politica na linha de comando, ele vai passar como ROUND-ROBIN.
				*segundos = s;
				*duracao = d;
				*pronto = 1;

				sleep(1);
				
				fseek(lista, 5, SEEK_CUR); // Pulando o "Exec "


				// resetando variaveis....
				aux = 0;
				i = 0;
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
	else 
	{	
		for(EVER){
			if(*pronto == 1){
				if(*politica == 0) // ROUND-ROBIN
				{
					inserirRR(filaRR, nome);	
				}
				else if(*politica == 1) // REAL-TIME
				{
					inserirRT(filaRT, *segundos, *duracao, nome);
				}
				else // PRIORIDADE
				{
					inserirPR(filaPR, *prioridade, nome);
				}
				*pronto = 0;
				puts("Alterado!");
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

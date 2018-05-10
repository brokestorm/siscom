#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lista.h"


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
int inserirRR(Fila *p, char* nome, int count, int pid)
{
	No *novo = (No*) malloc(sizeof(No));
	int i;

	novo->nomeDoPrograma = (char*) malloc(TAM*sizeof(char));
	for(i = 0; nome[i] != '\0'; i++)
		novo->nomeDoPrograma[i] = nome[i];
	novo->nomeDoPrograma[i] = '\0';
		
	novo->prox = NULL;
	novo->pid = pid;
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
	int tempoFinalAtual, tempoFinalNovo, i;


	if(seg+dur > 60)
	{
		printf("Tempo de duração do processo %s é maior que 60 segundos, ele sera descartado.\n", nome);
		return NULL;	
	}

	novo->nomeDoPrograma = (char*) malloc(TAM*sizeof(char));
	for(i = 0; nome[i] != '\0'; i++)
		novo->nomeDoPrograma[i] = nome[i];
	novo->nomeDoPrograma[i] = '\0';
	
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
	int i;
	
	novo->nomeDoPrograma = (char*) malloc(TAM*sizeof(char));
	for(i = 0; nome[i] != '\0'; i++)
		novo->nomeDoPrograma[i] = nome[i];
	novo->nomeDoPrograma[i] = '\0';
	
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

void removeMeioPR(Fila* p, No *removido)
{
	No *b;
	
	for(b = p->inicial; b->prox != removido; b = b->prox ) ;
	
	b->prox = removido->prox;
	free(removido);
}
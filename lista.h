#define TAM 200

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


Fila* criaFila();

// Round Robin funciona como uma fila normal, apenas insere no final e remove no primeiro
int inserirRR(Fila *p, char* nome, int count, int pid);

// RT vamos modelar como uma fila circular, já que RT cicla pelos processos pra sempre. E é possivel inserir em qualquer lugar da fila
No* inserirRT(Fila *p, int seg, int dur, char* nome);

// PR é uma fila, onde só removemos o primeiro, porém é possível inserir em qualquer ponto
int inserirPR(Fila *p, int prio, char* nome);

void removePrimeiro(Fila *p);

void removeMeioPR(Fila* p, No *removido);
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/wait.h>
//#include <unistd.h>
#define EVER ;;
#define TAM 200

int main()
{
	int i = 0, aux = 0;									// auxiliares
	int segundo = 0, duracao = 0, prioridade = 0;		// parametros para o escalonador
 	int politica = 0;									// 1 para REAL TIME, 2 para Prioridade, 0 para ROUND-ROBIN
	char parametro[TAM], nomeDoPrograma[TAM];			// buff de texto do arquivo 
	char character;										// buff de character do arquivo
	FILE *lista;										// arquivo "exec.txt"

	if (fork() == 0) // Interpretador de comandos
	{
		lista = fopen("exec.txt", "r");
		if (lista == NULL) // verificando erros
		{
			printf("Ocorreu um erro ao criar um novo arquivo\n");
			exit(1);
		}

		fseek(lista, 5, SEEK_SET);

		for (EVER) 
		{
			fscanf(lista, "%c", &character); // lendo cada character do arquivo de texto
			if (aux == 0) 
			{
				nomeDoPrograma[i] = character; // salvando o nome do programa
				
				if (i >= 1) 
				{
					if (nomeDoPrograma[i] == ' ') 
					{
						aux++; // isso indica que o nome do programa terminou de ser lido
					}
				}
			}
			else if (character == '\n') 
			{
				sleep(1);
				// TRANSMITIR DADOS PARA O ESCALONADOR AQUI
				
				//(...)

				// resetando variaveis....
				aux = 0;
				i = 0;
				politica = 0;

			}
			else 
			{
				i = 0; // agora usando i para indexar o vetor "parametro"
				parametro[i] = character;

				if (parametro[i] == '=') 
				{ 
					if (parametro[i - 1] == 'I') 
					{ // parametro de segundos
						politica = 1; // REAL TIME
						fscanf(lista, "%d", &segundo);
					}
					else if (parametro[i - 1] == 'D') 
					{ 
						politica = 1; // REAL TIME (desnecessario?)
						fscanf(lista, "%d", &duracao);
					}
					else if (parametro[i - 1] == 'R') 
					{ 
						politica = 2; // prioridade
						fscanf(lista, "%d", &prioridade);
					}
				}
			}
			i++;
		}
		fclose(lista);
	}
	//else // Escalonador
	//{
	//	
	//}

	return 0;
}
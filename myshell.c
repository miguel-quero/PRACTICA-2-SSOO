#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h> 
#include "parser.h"
#include <errno.h>
#include <wait.h> 
#include <fcntl.h>
//gcc -Wextra -Wall myshell.c -o myshell libparser.a -static

int fin = 0; // Para finalizar el bucle del main

int verificarmandato(char* mandato){ // Comprueba que el mandato se encuentra en la minishell
	if(mandato != NULL){
		return 0;
	}else{
		return 1;
	}
}

void cd(tline * line){ //Se ejecuta cuando se elige el mandato cd
	char *dir; //esta variable almacena el directorio elegido
	char ruta[1024]; //almacena la ruta del directorio actual

	if (line->commands[0].argc==1){ // Si el usuario no pone nada, el directorio es $HOME
		dir=getenv("HOME");
	}else { //Si el usuario incluye una ruta de directorio, se almacena en dir
		dir=line->commands[0].argv[1];
	}

	if (chdir(dir)!=0) { //Si la funcion chdir devuelve un 0 se cambia el directorio, si es distinto a 0, muestra el error del if.
		printf("Error al cambiar de directorio: %s\n",strerror(errno));
		if(line->commands[0].argc>2){// Si el usuario pone 2 argumentos se muestra en pantalla el error
			fprintf(stderr, "Muchos argumentos.\n");
		}
	}
	printf("El directorio actual es: %s\n",getcwd(ruta,1024));
}

void Exit() { //Función para salir de la minishell de manera correcta
	printf("Saliendo de la minishell de manera ordenada.\n");
	printf("Prompt de la shell: msh>  \n");
	fin = 1;
    exit(0);
}

int main() {
	pid_t pid;
	char buf[1024]; // Almacena lo que el usuario introduce por la entrada estándar
	tline * line; // objeto tline para descomponer el buf en comandos
	signal(SIGINT,SIG_IGN); //Ignora CTRL+C
	int status; //estado hijo/s

	while(fin == 0){ /*Si la línea introducida no contiene ningún mandato o se ejecuta el mandato en background, 
		se volverá a mostrar el prompt a la espera de una nueva línea.*/
		printf("msh> ");

		if (fgets(buf,1024, stdin) == NULL) { //Por si el usuario pulsa crtl + D
			break;
        }

		line = tokenize(buf); // Transforma la entrada en un objeto line
		
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		} 

		if(line->ncommands == 1){ //Si se ejecuta un mandato
			if(strcmp(line->commands[0].argv[0],"exit")==0){ //Si se ejecuta exit
				Exit();
			} else if (strcmp(line->commands[0].argv[0], "cd")==0){// Si se ejecuta cd
				cd(line);
			}else{
				if(verificarmandato(line->commands[0].filename)==0){ // comprueba que el mandato es valido
					pid=fork();
					if(pid<0) {  /* Error */
							fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
							exit(1);
					}else if(pid==0){ //  Proceso Hijo
								execvp(line->commands[0].filename,line->commands[0].argv);
								//Si llega aquí es que se ha producido un error en el execvp
								printf("Error al ejecutar el comando: %s\n", strerror(errno));
								exit(1);
					}else{  /*Proceso padre
						WIFEXITED(estadoHijo) es 0 si el hijo ha terminado de una manera anormal. 
						Distinto de 0 si ha terminado porque ha hecho una llamada a la función exit().
						WEXITSTATUS(estadoHijo) devuelve el valor que ha pasado el hijo a la función exit(), siempre y cuando la 
						macro anterior indique que la salida ha sido por una llamada a exit(). */
						wait(&status);
						if(WIFEXITED(status)!=0)
						if(WEXITSTATUS(status)!=0)
							printf("El comando no se ejecutó correctamente\n");
					}
				}else{ //El mandato no se encuentra
						fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[0].argv[0]);
					}
			}
		}else if(line->ncommands>1){ // Si se ejecuta más de 1 mandato
			
		}
	}
	return 0;
}

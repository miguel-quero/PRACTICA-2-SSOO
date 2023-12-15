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


int verificarmandato(char* mandato){
	if(mandato != NULL){
		return 0;
	}else{
		return 1;
	}
}

void cd(tline * line){
	char *dir; // Variable de directorios 
	char buffer[512];
	
	if(line->commands[0].argc > 2) // No puedo hacer un cd a 2 directorios 
		{
		  fprintf(stderr,"Uso: %s directorio\n", line->commands[0].argv[0]);
		}
	if (line->commands[0].argc == 1) // Si vale 1 , no me pasan nada, osea nombre del programa.
	{
		dir = getenv("HOME");
		if(dir == NULL)
		{
			fprintf(stderr,"No existe la variable $HOME\n");	
		}
	}else {
		dir = line->commands[0].argv[1];
	}
	
	// Comprobar si es un directorio.
	if (chdir(dir) != 0) { // Sino es distinto de 0 lo hace normal el chdir 
		fprintf(stderr,"Error al cambiar de directorio: %s\n", strerror(errno)); // Los errores a llamada al sistema siempre se guardan en errno, y strerror explica el porque de errno.
	} else {
	printf( "El directorio actual es: %s\n", getcwd(buffer,-1));
	}
}

int main(void) {
	pid_t pid;
	char buf[1024];
	tline * line;

	while (1) {
		printf("msh> ");

		if (fgets(buf, 1024, stdin) == NULL) {
            break;
        }

		int status;
		signal(SIGINT,SIG_IGN); //Ignorar Ctrl+C
		
		line = tokenize(buf);

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
			if (strcmp(line->commands[0].argv[0], "cd") == 0){// Si se ejecuta cd
				cd(line);
			}else{
				pid = fork();
				if(pid < 0) {  /* Error */
						fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
						exit(1);
				}else if(pid == 0){ //  Proceso Hijo
					if(verificarmandato(line->commands[0].filename)==0){ // comprueba que el mandato es valido
							execvp(line->commands[0].filename,line->commands[0].argv);
							//Si llega aquí es que se ha producido un error en el execvp
							printf("Error al ejecutar el comando: %s\n", strerror(errno));
							exit(1);
					}else{ //El mandato no se encuentra
						fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[0].argv[0]);
					}
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
			}

		}else if(line->ncommands > 1){ // Si se ejecuta más de 1 mandato
			
		}
	}
	return 0;
}

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
	fin = 1; //finaliza el bucle del main
    exit(0); //sale de la minishell de manera ordenada
}

void siginthandler(){ //Función para tratar CTRL+C
	printf("\nmsh> "); //Se escribe la linea nueva con el prompt al detectar la señal, como en bash
	fflush(stdout); //Garantiza que los datos del búfer de salida se escriban en salida estándar 
}

void Umask(tline * line){ //Funcion para ejecutar umask
	if (line->commands->argc != 2) { //Si la entrada estándar no contiene un número se muestra el siguiente error indicando el uso del mandato
        fprintf(stderr, "Uso: %s <numero_octal>\n", line->commands->argv[0]);
    } else { //Si la entrada contiene un número
		mode_t mask = strtol(line->commands->argv[1],NULL,8); //Se utiliza para convertir un string en un número con una base específica, en este caso 8 (octal)
    	mode_t maskantigua = umask(mask); //Establece la máscara umask de tipo mode_t pasada por parámetro, y devuelve la máscara anterior
		printf("La máscara umask anterior era %#o.\n", maskantigua); 
    	printf("La máscara umask ahora es %#o.\n", mask);
	}
}

void redireccionEntrada(char * entrada){ //Función que redirecciona la entrada
	int fd = open(entrada,O_RDONLY|O_CREAT); //El archivo se abrirá para lectura y si no existe el archivo lo crea
	if(fd == -1){ //Si se produce un error al abrir el archivo, se muestra el error
		fprintf(stderr,"%s : Error. %s\n",entrada,strerror(errno)); 
	}else{ //Si se abrió el archivo de forma correcta, ahora la entrada estándar apunta al archivo
		dup2(fd,fileno(stdin)); //
	}	
}

void redireccionSalida(char * salida){ //Función que redirecciona la salida
	int fd = creat(salida,S_IRGRP|S_IWOTH|S_IRUSR|S_IWGRP|S_IROTH|S_IWUSR); //Crea el archivo o lo trunca si exite. Las flags utilizadas en resumen son para añadir permisos específicos al grupo, propietario o para otros.
	if(fd == -1){//Si se produce un error al abrir el archivo, se muestra el error
		fprintf(stderr,"%s : Error. %s\n",salida,strerror(errno));
	}else{  //Si se abrió el archivo de forma correcta, ahora la salida estándar apunta al archivo
		dup2(fd,fileno(stdout)); 
	}	
}

void redireccionError(char * error){ //Función que redirecciona la salida para errores
	int fd = creat(error,S_IWGRP|S_IWOTH|S_IRGRP|S_IRUSR|S_IROTH|S_IWUSR); //Lo mismo que al redireccionar la salida estándar,
	if(fd == -1){ //Si se produce un error al abrir el archivo, se muestra el error
		fprintf(stderr,"%s : Error. %s\n",error,strerror(errno)); 
	}else{ //Si se abrió el archivo de forma correcta, ahora la salida para errores apunta al archivo
		dup2(fd,fileno(stderr));
	}	
}

void reestablecerDescriptores(int entrada, int salida, int error, tline * line){ //Función para reestablecer los descriptores
	if(line->redirect_input!=NULL){ //Si la entrada es válida se reestablece a la entrada estándar
		dup2(entrada,fileno(stdin));	
	}
	if(line->redirect_error!=NULL){ //Si la salida para el error es válida se reestablece a la salida estándar de error
		dup2(error,fileno(stderr));	
	}
	if(line->redirect_output!=NULL){ //Si la salida es válida se reestablece a la salida estándar
		dup2(salida,fileno(stdout));	
	}
}

int main() {
	//Guardamos los descriptores de archivo predeterminados para luego reestablecerlos si se cambian en ejecución
	int Salida=dup(fileno(stdout));
	int Entrada=dup(fileno(stdin));
	int Error=dup(fileno(stderr));

	pid_t pid; //Identificador del proceso actual
	char buf[1024]; // Almacena lo que el usuario introduce por la entrada estándar
	tline * line; // objeto tline para descomponer el buf en comandos
	signal(SIGINT,siginthandler); //Tratar la señal CTRL+C con la funcion siginthandler
	int status; //estado hijo/s

	while(fin == 0){ /*Si la línea introducida no contiene ningún mandato o se ejecuta el mandato en background, 
		se volverá a mostrar el prompt a la espera de una nueva línea.*/
		printf("msh> ");

		if (fgets(buf,1024, stdin) == NULL) { //Si el usuario pulsa crtl + D, se cierra la minishell
			break;
        }

		line = tokenize(buf); // Transforma la entrada en un objeto line
		
		if (line==NULL){
			continue;
		}
		if (line->redirect_input != NULL){ //Redirecciona la entrada
			printf("redirección de entrada: %s\n", line->redirect_input);
			redireccionEntrada(line->redirect_input);
		}
		if (line->redirect_output != NULL) {//Redirecciona la salida
			printf("redirección de salida: %s\n", line->redirect_output);
			redireccionSalida(line->redirect_output);
		}
		if (line->redirect_error != NULL) { //Redirecciona la salida para el error
			printf("redirección de error: %s\n", line->redirect_error);
			redireccionError(line->redirect_error);
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		} 

		if(line->ncommands == 1){ //Si se ejecuta un mandato
			if(strcmp(line->commands[0].argv[0], "umask")==0){ //Si se ejecuta umask
				Umask(line);
			} else if(strcmp(line->commands[0].argv[0],"exit")==0){ //Si se ejecuta exit
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

		//Se utiliza para reestablecer los descriptores estándar
		reestablecerDescriptores(Entrada,Salida,Error,line);
	}
	return 0;
}

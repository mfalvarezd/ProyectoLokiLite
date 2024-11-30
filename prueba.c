#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    // Primer proceso hijo para ejecutar 'ls'
    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("Error al crear el primer proceso hijo");
        return 1;
    } else if (pid1 == 0) {
        // Proceso hijo 1
        char *args1[] = {"ls", "-l", NULL}; // Comando 'ls -l'
        execvp(args1[0], args1); // Ejecuta 'ls'
        perror("Error ejecutando execvp para ls"); // Solo se ejecuta si execvp falla
        exit(1);
    }

    // Segundo proceso hijo para ejecutar 'date'
    pid_t pid2 = fork();

    if (pid2 < 0) {
        perror("Error al crear el segundo proceso hijo");
        return 1;
    } else if (pid2 == 0) {
        // Proceso hijo 2
        char *args2[] = {"date", NULL}; // Comando 'date'
        execvp(args2[0], args2); // Ejecuta 'date'
        perror("Error ejecutando execvp para date"); // Solo se ejecuta si execvp falla
        exit(1);
    }

    // Proceso padre espera a que ambos hijos terminen
    waitpid(pid1, NULL, 0); // Espera a que termine el primer hijo
    waitpid(pid2, NULL, 0); // Espera a que termine el segundo hijo

    printf("Ambos procesos han terminado.\n");
    return 0;
}

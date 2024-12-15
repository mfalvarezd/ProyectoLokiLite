#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid1, pid2;

    // Crear el primer proceso hijo para el primer comando
    pid1 = fork();
    if (pid1 < 0) {
        perror("Error al crear el primer proceso");
        return 1;
    } else if (pid1 == 0) {
        // Proceso hijo: ejecutar el primer comando
        execlp("stress", "stress", "--cpu", "7", "--timeout", "15", (char *)NULL);
        perror("Error al ejecutar el primer comando");
        return 1; // Salir si exec falla
    }

    // Crear el segundo proceso hijo para el segundo comando
    pid2 = fork();
    if (pid2 < 0) {
        perror("Error al crear el segundo proceso");
        return 1;
    } else if (pid2 == 0) {
        // Proceso hijo: ejecutar el segundo comando
        execlp("stress", "stress", "--vm", "5", "--vm-bytes", "3G", "--timeout", "60", (char *)NULL);
        perror("Error al ejecutar el segundo comando");
        return 1; // Salir si exec falla
    }

    // Esperar a que ambos procesos hijos terminen
    waitpid(pid1, NULL, 0);  // Espera a que termine el primer proceso
    waitpid(pid2, NULL, 0);  // Espera a que termine el segundo proceso

    return 0;
}

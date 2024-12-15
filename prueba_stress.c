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
        return 1; 
    }

    pid2 = fork();
    if (pid2 < 0) {
        perror("Error al crear el segundo proceso");
        return 1;
    } else if (pid2 == 0) {
       
        execlp("stress", "stress", "--vm", "5", "--vm-bytes", "3G", "--timeout", "60", (char *)NULL);
        perror("Error al ejecutar el segundo comando");
        return 1; 
    }


    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);  

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int keep_running = 1;

// Manejo de señal para terminación
void handle_sigint(int sig) {
    keep_running = 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <servicio1> <servicio2> [tiempo_actualizacion]\n", argv[0]);
        return 1;
    }

    // Servicios a monitorear
    char *servicio1 = argv[1];
    char *servicio2 = argv[2];

    // Tiempo de actualización
    int tiempo_actualizacion = 5; // Valor por defecto
    if (argc > 3) {
        tiempo_actualizacion = atoi(argv[3]);
        if (tiempo_actualizacion <= 0) {
            fprintf(stderr, "El tiempo de actualización debe ser un valor positivo.\n");
            return 1;
        }
    }

    printf("Monitoreando servicios: %s, %s\n", servicio1, servicio2);
    printf("Tiempo de actualización: %d segundos\n", tiempo_actualizacion);

    // Configurar manejo de señal
    signal(SIGINT, handle_sigint);

    // Bucle de monitoreo
    while (keep_running) {
        // Comando para journalctl del primer servicio
        char *args1[] = {"journalctl", "-u", servicio1, "-n", "10", NULL};
        printf("Ejecutando comando para servicio: %s\n", servicio1);
        
        pid_t pid1 = fork();
        if (pid1 == 0) { // Proceso hijo
            execvp(args1[0], args1);
            perror("Error ejecutando execvp para servicio 1");
            exit(1);
        } else if (pid1 < 0) {
            perror("Error al crear proceso hijo");
            return 1;
        }
        int status1;
        waitpid(pid1, &status1, 0); // Espera a que termine el hijo
        if (WIFEXITED(status1) && WEXITSTATUS(status1) != 0) {
            fprintf(stderr, "El comando journalctl para %s terminó con error.\n", servicio1);
        }

        // Comando para journalctl del segundo servicio
        char *args2[] = {"journalctl", "-u", servicio2, "-n", "10", NULL};
        printf("Ejecutando comando para servicio: %s\n", servicio2);
        
        pid_t pid2 = fork();
        if (pid2 == 0) { // Proceso hijo
            execvp(args2[0], args2);
            perror("Error ejecutando execvp para servicio 2");
            exit(1);
        } else if (pid2 < 0) {
            perror("Error al crear proceso hijo");
            return 1;
        }
        int status2;
        waitpid(pid2, &status2, 0); // Espera a que termine el hijo
        if (WIFEXITED(status2) && WEXITSTATUS(status2) != 0) {
            fprintf(stderr, "El comando journalctl para %s terminó con error.\n", servicio2);
        }

        sleep(tiempo_actualizacion); // Espera antes de la siguiente actualización
    }

    printf("Monitoreo detenido.\n");
    return 0;
}

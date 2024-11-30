#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
}

void print_usage(const char *prog_name) {
    printf("Uso: %s <servicio1> <servicio2> [servicio3] ... [servicioN] [tiempo_actualizacion]\n", prog_name);
}

int main(int argc, char *argv[]) {
    // Validación de argumentos: debe haber al menos 3 argumentos (nombre del programa + 2 servicios)
    if (argc < 3) { // Debe haber al menos 2 servicios
        print_usage(argv[0]);
        return 1;
    }

    // Número de servicios a monitorear
    int num_servicios = argc - 2; // Restamos 2 para los nombres de los servicios
    char **servicios = argv + 1; // Los servicios comienzan en argv[1]

    // Tiempo de actualización
    int tiempo_actualizacion = 5; // Valor por defecto
    // Verificamos si se proporcionó un tiempo de actualización
    if (argc > 3) { // Solo si hay más de 3 argumentos, se puede considerar un tiempo de actualización
        tiempo_actualizacion = atoi(argv[argc - 1]); // Último argumento como tiempo de actualización
        if (tiempo_actualizacion <= 0) {
            fprintf(stderr, "El tiempo de actualización debe ser un valor positivo.\n");
            return 1;
        }
    }

    printf("Monitoreando servicios: ");
    for (int i = 0; i < num_servicios; i++) {
        printf("%s%s", servicios[i], (i < num_servicios - 1) ? ", " : "\n");
    }
    printf("Tiempo de actualización: %d segundos\n", tiempo_actualizacion);

    signal(SIGINT, handle_sigint);

    // Bucle de monitoreo
    while (keep_running) {
        for (int i = 0; i < num_servicios; i++) {
            // Comando para journalctl del servicio actual
            char *args[] = {"journalctl", "-u", servicios[i], "-n", "10", NULL};
            printf("Ejecutando comando para servicio: %s\n", servicios[i]);

            pid_t pid = fork();
            if (pid == 0) { // Proceso hijo
                execvp(args[0], args);
                perror("Error ejecutando execvp");
                exit(1);
            } else if (pid < 0) {
                perror("Error al crear proceso hijo");
                return 1;
            }

            int status;
            waitpid(pid, &status, 0); // Espera a que termine el hijo
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                fprintf(stderr, "El comando journalctl para %s terminó con error.\n", servicios[i]);
            }
        }

        sleep(tiempo_actualizacion); // Espera antes de la siguiente actualización
    }

    printf("Monitoreo detenido.\n");
    return 0;
}

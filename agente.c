#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>

int keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
}

void print_usage(const char *prog_name) {
    printf("Uso: %s <servicio1> <servicio2> [servicio3] ... [servicioN] [tiempo_actualizacion]\n", prog_name);
}

// Función para verificar si una cadena es un número válido
int es_numero(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) { // Debe haber al menos 2 servicios
        print_usage(argv[0]);
        return 1;
    }

    int tiempo_actualizacion = 5; // Valor por defecto
    int num_servicios = argc - 1; // Inicialmente, todos los argumentos se consideran servicios

    // Verificar si el último argumento es un número válido (tiempo de actualización)
    if (argc > 3 && es_numero(argv[argc - 1])) {
        tiempo_actualizacion = atoi(argv[argc - 1]);
        num_servicios--; // Reducir el número de servicios, ya que el último argumento no es un servicio
        if (tiempo_actualizacion <= 0) {
            fprintf(stderr, "El tiempo de actualización debe ser un valor positivo.\n");
            return 1;
        }
    }

    char **servicios = argv + 1; // Los servicios comienzan en argv[1]

    printf("Número de servicios a monitorear: %d\n", num_servicios);
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

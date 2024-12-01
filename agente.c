#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

int keep_running = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Niveles de prioridad
const char *prioridades[] = {"alert", "err", "notice", "info", "debug"};
#define NUM_PRIORIDADES 5

// Función para manejar la señal SIGINT
void handle_sigint(int sig) {
    keep_running = 0;
}

// Función para imprimir el uso del programa
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

// Estructura de datos para cada hilo
typedef struct {
    const char *servicio;
    int tiempo_actualizacion;
} ServicioHilo;

// Función para enviar alertas usando Twilio
void enviar_alerta(const char *servicio, const char *mensaje) {
    char comando[512];
    snprintf(comando, sizeof(comando),
             "curl -X POST https://api.twilio.com/2010-04-01/Accounts/<ACCOUNT_SID>/Messages.json "
             "-u <ACCOUNT_SID>:<AUTH_TOKEN> "
             "-d 'To=<DESTINO>' -d 'From=<ORIGEN>' -d 'Body=Alerta! Servicio %s: %s.'",
             servicio, mensaje);
    system(comando);
}

// Función de monitoreo para un servicio
void *monitorear_servicio(void *arg) {
    ServicioHilo *data = (ServicioHilo *)arg;
    const char *servicio = data->servicio;
    int tiempo_actualizacion = data->tiempo_actualizacion;

    int conteo_prioridades[NUM_PRIORIDADES] = {0};

    while (keep_running) {
        pthread_mutex_lock(&mutex);
        printf("\nMonitoreando servicio: %s\n", servicio);

        for (int p = 0; p < NUM_PRIORIDADES; p++) {
            // Comando journalctl para filtrar por prioridad
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("Error creando pipe");
                pthread_mutex_unlock(&mutex);
                return NULL;
            }

            pid_t pid = fork();
            if (pid == 0) { // Proceso hijo
                close(pipefd[0]); // Cierra lectura
                dup2(pipefd[1], STDOUT_FILENO); // Redirige stdout al pipe
                char *args[] = {"journalctl", "-p", prioridades[p], "-u", (char *)servicio, "-n", "10", NULL};
                execvp(args[0], args);
                perror("Error ejecutando execvp");
                exit(1);
            } else if (pid > 0) { // Proceso padre
                close(pipefd[1]); // Cierra escritura
                char buffer[1024];
                ssize_t bytes_leidos = read(pipefd[0], buffer, sizeof(buffer) - 1);
                if (bytes_leidos > 0) {
                    buffer[bytes_leidos] = '\0';
                    conteo_prioridades[p]++;
                    printf("Prioridad %s: %d mensajes\n", prioridades[p], conteo_prioridades[p]);
                }
                waitpid(pid, NULL, 0); // Espera al hijo
            }
        }

        // Verifica si se supera un threshold (ejemplo: 5 alertas)
      //  if (conteo_prioridades[0] > 5) {
       //     printf("¡Alerta crítica en servicio %s! Enviando notificación...\n", servicio);
       //     enviar_alerta(servicio, "Demasiadas alertas críticas");
      //  }

        pthread_mutex_unlock(&mutex);
        sleep(tiempo_actualizacion);
    }

    return NULL;
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

    // Crear hilos para cada servicio
    pthread_t hilos[num_servicios];
    ServicioHilo datos_hilos[num_servicios];

    for (int i = 0; i < num_servicios; i++) {
        datos_hilos[i].servicio = servicios[i];
        datos_hilos[i].tiempo_actualizacion = tiempo_actualizacion;

        if (pthread_create(&hilos[i], NULL, monitorear_servicio, &datos_hilos[i]) != 0) {
            perror("Error creando hilo");
            return 1;
        }
    }

    // Esperar a que terminen los hilos
    for (int i = 0; i < num_servicios; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("Monitoreo detenido.\n");
    return 0;
}

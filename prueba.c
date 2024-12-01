#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

int keep_running = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Niveles de prioridad
const char *prioridades[] = {"alert", "err", "notice", "info", "debug"};
#define NUM_PRIORIDADES 5

// Función para manejar la señal SIGINT
void handle_sigint(int sig) {
    keep_running = 0;
}

// Estructura para datos del hilo
typedef struct {
    const char *servicio;
    int tiempo_actualizacion;
    const char *ip_servidor;
    int puerto;
} ServicioHilo;

// Función para enviar datos al servidor
void enviar_datos_al_servidor(const char *ip_servidor, int puerto, const char *datos) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Crear socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(puerto);

    // Convertir IP a formato binario
    if (inet_pton(AF_INET, ip_servidor, &server_addr.sin_addr) <= 0) {
        perror("Dirección IP inválida o no soportada");
        close(sockfd);
        return;
    }

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar con el servidor");
        close(sockfd);
        return;
    }

    // Enviar datos
    send(sockfd, datos, strlen(datos), 0);
    close(sockfd);
}

// Función de monitoreo
void *monitorear_servicio(void *arg) {
    ServicioHilo *data = (ServicioHilo *)arg;
    const char *servicio = data->servicio;
    int tiempo_actualizacion = data->tiempo_actualizacion;
    const char *ip_servidor = data->ip_servidor;
    int puerto = data->puerto;

    int conteo_prioridades[NUM_PRIORIDADES] = {0};

    while (keep_running) {
        pthread_mutex_lock(&mutex);
        printf("Monitoreando servicio: %s\n", servicio);

        for (int p = 0; p < NUM_PRIORIDADES; p++) {
            // Comando journalctl para filtrar por prioridad
            char comando[256];
            snprintf(comando, sizeof(comando), "journalctl -p %s -u %s -n 10", prioridades[p], servicio);
            FILE *pipe = popen(comando, "r");
            if (pipe) {
                char buffer[1024];
                while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                    conteo_prioridades[p]++;
                }
                pclose(pipe);
            }
        }

        // Crear un mensaje para enviar al servidor
        char mensaje[512];
        snprintf(mensaje, sizeof(mensaje),
                 "{ \"servicio\": \"%s\", \"alertas\": %d, \"errores\": %d }",
                 servicio, conteo_prioridades[0], conteo_prioridades[1]);

        // Enviar datos al servidor
        enviar_datos_al_servidor(ip_servidor, puerto, mensaje);
        pthread_mutex_unlock(&mutex);

        sleep(tiempo_actualizacion);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Uso: %s <IP_servidor> <puerto> <tiempo_actualizacion> <servicio1> [servicio2]...\n", argv[0]);
        return 1;
    }

    const char *ip_servidor = argv[1];
    int puerto = atoi(argv[2]);
    int tiempo_actualizacion = atoi(argv[3]);
    int num_servicios = argc - 4;

    signal(SIGINT, handle_sigint);

    pthread_t hilos[num_servicios];
    ServicioHilo datos_hilos[num_servicios];

    for (int i = 0; i < num_servicios; i++) {
        datos_hilos[i].servicio = argv[i + 4];
        datos_hilos[i].tiempo_actualizacion = tiempo_actualizacion;
        datos_hilos[i].ip_servidor = ip_servidor;
        datos_hilos[i].puerto = puerto;

        if (pthread_create(&hilos[i], NULL, monitorear_servicio, &datos_hilos[i]) != 0) {
            perror("Error creando hilo");
            return 1;
        }
    }

    for (int i = 0; i < num_servicios; i++) {
        pthread_join(hilos[i], NULL);
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"  // Dirección IP del servidor
#define SERVER_PORT 8080       // Puerto del servidor
#define BUFFER_SIZE 1024       // Tamaño del buffer para enviar datos

int keep_running = 1;

// Función para manejar la señal SIGINT y detener el programa
void handle_sigint(int sig) {
    keep_running = 0;
}

// Función para imprimir el uso correcto del programa
void print_usage(const char *prog_name) {
    printf("Uso: %s <servicio1> <servicio2> ... [servicioN] [tiempo_actualizacion]\n", prog_name);
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

// Función para conectarse al servidor
int conectar_al_servidor() {
    int sock;
    struct sockaddr_in server_addr;

    // Crear el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[ERROR]: No se pudo crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[ERROR]: Dirección IP no válida");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[ERROR]: No se pudo conectar al servidor");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[INFO]: Conexión establecida con el servidor %s:%d\n", SERVER_IP, SERVER_PORT);
    return sock;
}

// Función para monitorear servicios y enviar datos al servidor
void monitorear_servicios(int server_sock, char **servicios, int num_servicios, int tiempo_actualizacion) {
    char buffer[BUFFER_SIZE];

    while (keep_running) {
        for (int i = 0; i < num_servicios; i++) {
            printf("[INFO]: Monitoreando servicio: %s\n", servicios[i]);

            // Simular recolección de datos con journalctl
            int alertas = rand() % 10; // Número aleatorio de alertas (simulación)
            int errores = rand() % 5; // Número aleatorio de errores (simulación)

            // Crear mensaje en formato JSON
            snprintf(buffer, sizeof(buffer),
                     "{ \"servicio\": \"%s\", \"alertas\": %d, \"errores\": %d }",
                     servicios[i], alertas, errores);

            // Enviar los datos al servidor
            if (send(server_sock, buffer, strlen(buffer), 0) == -1) {
                perror("[ERROR]: No se pudieron enviar los datos al servidor");
            } else {
                printf("[INFO]: Datos enviados: %s\n", buffer);
            }
        }

        sleep(tiempo_actualizacion); // Esperar el tiempo de actualización
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) { // Debe haber al menos 2 servicios
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int tiempo_actualizacion = 5; // Valor por defecto
    int num_servicios = argc - 1; // Inicialmente, todos los argumentos se consideran servicios

    // Verificar si el último argumento es un número válido (tiempo de actualización)
    if (argc > 3 && es_numero(argv[argc - 1])) {
        tiempo_actualizacion = atoi(argv[argc - 1]);
        num_servicios--; // Reducir el número de servicios, ya que el último argumento no es un servicio
        if (tiempo_actualizacion <= 0) {
            fprintf(stderr, "[ERROR]: El tiempo de actualización debe ser un valor positivo.\n");
            return EXIT_FAILURE;
        }
    }

    char **servicios = argv + 1; // Los servicios comienzan en argv[1]

    printf("[INFO]: Número de servicios a monitorear: %d\n", num_servicios);
    printf("[INFO]: Servicios a monitorear: ");
    for (int i = 0; i < num_servicios; i++) {
        printf("%s%s", servicios[i], (i < num_servicios - 1) ? ", " : "\n");
    }
    printf("[INFO]: Tiempo de actualización: %d segundos\n", tiempo_actualizacion);

    // Manejar la señal SIGINT para detener el programa
    signal(SIGINT, handle_sigint);

    // Conectar al servidor
    int server_sock = conectar_al_servidor();

    // Monitorear servicios y enviar datos al servidor
    monitorear_servicios(server_sock, servicios, num_servicios, tiempo_actualizacion);

    // Cerrar el socket al finalizar
    close(server_sock);
    printf("[INFO]: Monitoreo detenido. Conexión cerrada.\n");

    return EXIT_SUCCESS;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#define SERVER_IP "127.0.0.1"  // Dirección IP del servidor
#define SERVER_PORT 8080       // Puerto del servidor
#define BUFFER_SIZE 1024       // Tamaño del buffer para enviar datos
#define TIEMPO_ACTUALIZACION 5 // Tiempo de actualización constante
int keep_running = 1;

// CTRL+C PARA DETENER
void handle_sigint(int sig) {
    keep_running = 0;
}


int conectar_al_servidor() {
    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[ERROR]: No se puede crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[ERROR]: Dirección IP no valida");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Manejar coneccion al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[ERROR]: No se puede conectar al servidor");
        close(sock);
        exit(EXIT_FAILURE); // en caso de falla se terminara la ejecucion
    }

    printf("[INFO]: Conexion satisfactoria con el servidor %s:%d\n", SERVER_IP, SERVER_PORT);
    return sock;
}

float obtener_uso_cpu() {
    static long prev_idle = 0, prev_total = 0;
    long idle, total;
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("[ERROR]: No se puede leer /proc/stat");
        return -1.0;
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    long user, nice, system, idle_time;
    sscanf(buffer, "cpu %ld %ld %ld %ld", &user, &nice, &system, &idle_time);

    idle = idle_time;
    total = user + nice + system + idle;

    float usage = (float)(total - prev_total - (idle - prev_idle)) / (total - prev_total) * 100;
    prev_idle = idle;
    prev_total = total;

    return usage;
}

float obtener_uso_memoria() {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        perror("[ERROR]: No se puede obtener información de memoria");
        return -1.0;
    }
    return (float)(info.totalram - info.freeram) / info.totalram * 100;
}

float obtener_memoria_disponible() {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        perror("[ERROR]: No se puede obtener información de memoria");
        return -1.0;
    }
    return (float)info.freeram / info.totalram * 100; // Porcentaje de memoria disponible
}

float obtener_uso_disco(const char *ruta) {
    struct statvfs stat;
    if (statvfs(ruta, &stat) != 0) {
        perror("[ERROR]: No se puede obtener información del disco");
        return -1.0;
    }
    return (float)(stat.f_blocks - stat.f_bfree) / stat.f_blocks * 100;
}

float obtener_uso_swap() {
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        perror("[ERROR]: No se puede obtener información de swap");
        return -1.0;
    }
    return (float)(info.totalswap - info.freeswap) / info.totalswap * 100; // Porcentaje de uso de swap
}

void obtener_trafico_red(long *in, long *out) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        perror("[ERROR]: No se puede leer /proc/net/dev");
        *in = *out = -1;
        return;
    }

    char buffer[256];
    *in = *out = 0;

    fgets(buffer, sizeof(buffer), fp); // Ignorar la primera línea
    fgets(buffer, sizeof(buffer), fp); // Ignorar la segunda línea

    while (fgets(buffer, sizeof(buffer), fp)) {
        char interfaz[32];
        long bytes_in, bytes_out;
        sscanf(buffer, "%s %ld %*d %*d %*d %*d %*d %*d %ld", interfaz, &bytes_in, &bytes_out);
        *in += bytes_in;
        *out += bytes_out;
    }

    fclose(fp);
}


void recolectar_metricas(int server_sock) {
    char buffer[BUFFER_SIZE];

    while (keep_running) {
        float cpu_usage = obtener_uso_cpu();
        float memory_usage = obtener_uso_memoria();
        float available_memory = obtener_memoria_disponible();
        float disk_usage = obtener_uso_disco("/");
        float swap_usage = obtener_uso_swap();
        long network_in, network_out;
        obtener_trafico_red(&network_in, &network_out);

        snprintf(buffer, sizeof(buffer),
                 "{ \"cpu_usage\": %.2f, \"memory_usage\": %.2f, \"available_memory\": %.2f, \"disk_usage\": %.2f, \"network_in\": %ld, \"network_out\": %ld, \"swap_usage\": %.2f }",
                 cpu_usage, memory_usage, available_memory, disk_usage, network_in, network_out, swap_usage);

        // Enviar las métricas al servidor
        if (send(server_sock, buffer, strlen(buffer), 0) == -1) {
            perror("[ERROR]: No se pudieron enviar los datos al servidor");
        } else {
            printf("[INFO]: Datos enviados: %s\n", buffer);
        }

        sleep(TIEMPO_ACTUALIZACION);
    }
}


int main() {

    printf("[INFO]: Tiempo de actualizacion: %d segundos\n", TIEMPO_ACTUALIZACION);

    // Manejar la señal SIGINT para detener el programa
    signal(SIGINT, handle_sigint);

    // Conectar al servidor
    int server_sock = conectar_al_servidor();

    // Recolectar métricas y enviarlas al servidor
    recolectar_metricas(server_sock);

    // Cerrar el socket al finalizar
    close(server_sock);
    printf("[INFO]: Monitoreo detenido. Conexión cerrada.\n");

    return EXIT_SUCCESS;
}

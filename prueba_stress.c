#include <stdio.h>
#include <stdlib.h>

int main() {

    // Ejecutar el primer comando
    int result1 = system("stress --cpu 7 --timeout 15");
    if (result1 == -1) {
        perror("Error al ejecutar el comando stress --cpu");
        return 1;
    }

    // Ejecutar el segundo comando
    int result2 = system("stress --vm 5 --vm-bytes 3G --timeout 40");
    if (result2 == -1) {
        perror("Error al ejecutar el comando stress --vm");
        return 1;
    }
}

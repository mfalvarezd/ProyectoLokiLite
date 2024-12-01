#include <stdio.h>
#include <stdlib.h>

int main() {

    const char *comando = "hydra -l test -P /usr/share/wordlists/rockyou.txt ssh://localhost";
    int resultado = system(comando);
    if (resultado == -1) {
        perror("Error al ejecutar el comando");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

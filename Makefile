# Variables
CC = gcc
CFLAGS = -Wall -Wextra -g
AGENTE = agente
SERVIDOR = servidor
PRUEBA = prueba_stress 
# Regla por defecto
all: $(AGENTE) $(SERVIDOR) $(PRUEBA) 

# Compilar el agente
$(AGENTE): agente.o
	$(CC) $(CFLAGS) -o $(AGENTE) agente.o

# Compilar el servidor
$(SERVIDOR): servidor.o
	$(CC) $(CFLAGS) -o $(SERVIDOR) servidor.o

# Compilar el ejecutable de hydra
$(PRUEBA): prueba_stress.o
	$(CC) $(CFLAGS) -o $(PRUEBA) ejecutaprueba_stress

# Regla para compilar el agente objeto
agente.o: agente.c
	$(CC) $(CFLAGS) -c agente.c

# Regla para compilar el servidor objeto
servidor.o: servidor.c
	$(CC) $(CFLAGS) -c servidor.c

# Regla para compilar el ejecutable de hydra objeto
prueba_stress.o: prueba_stress.c
	$(CC) $(CFLAGS) -c prueba_stress.c

# Limpiar archivos generados
clean:
	rm -f $(AGENTE) $(SERVIDOR) $(PRUEBA) *.o

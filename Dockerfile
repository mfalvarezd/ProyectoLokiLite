# Usar Ubuntu 22.04 como imagen base
FROM ubuntu:22.04

# Actualizar el sistema e instalar las dependencias necesarias
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    gcc \
    make \
    curl \
    stress \
    libcurl4-openssl-dev && \
    rm -rf /var/lib/apt/lists/*
COPY . /home/projects/ProyectoLokiLite
# Definir el directorio de trabajo
WORKDIR /home/projects/ProyectoLokiLite

# Comando por defecto al iniciar el contenedor
CMD ["bash"]

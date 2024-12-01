import http.client
import urllib.parse
import base64
import os
import argparse
import ssl  # Importar el módulo ssl

# Resto de tu código...

# Crear la conexión con verificación de certificado deshabilitada
context = ssl._create_unverified_context()  # Crear un contexto SSL sin verificación
# Función para cargar las credenciales desde un archivo .env
def load_env(filename):
    credentials = {}
    with open(filename, 'r') as file:
        for line in file:
            key, value = line.strip().split('=')
            credentials[key] = value
    return credentials

# Configuración de argumentos
parser = argparse.ArgumentParser(description='Enviar un mensaje de WhatsApp a través de Twilio.')
parser.add_argument('message', type=str, help='El mensaje a enviar')

args = parser.parse_args()

# Cargar las credenciales
env_file = '.env'
credentials = load_env(env_file)
account_sid = credentials.get('TWILIO_ACCOUNT_SID')
auth_token = credentials.get('TWILIO_AUTH_TOKEN')

# URL y datos de la solicitud
url = 'api.twilio.com'
endpoint = '/2010-04-01/Accounts/{}/Messages.json'.format(account_sid)

data = {
    'To': 'whatsapp:+593986849600',
    'From': 'whatsapp:+14155238886',
    'Body': args.message  # Usar el mensaje recibido como parámetro
}

# Codificar los datos
encoded_data = urllib.parse.urlencode(data)

# Crear la conexión
conn = http.client.HTTPSConnection(url, context=context)  # Pasar el contexto a la conexión

# Configurar la autenticación básica
credentials = f"{account_sid}:{auth_token}"
encoded_credentials = base64.b64encode(credentials.encode('utf-8')).decode('utf-8')

# Configurar los encabezados
headers = {
    'Authorization': f'Basic {encoded_credentials}',
    'Content-Type': 'application/x-www-form-urlencoded'
}

# Realizar la solicitud POST
conn.request('POST', endpoint, body=encoded_data, headers=headers)

# Obtener la respuesta
response = conn.getresponse()
response_data = response.read().decode('utf-8')

# Verificar la respuesta
if response.status == 201:
    print("Mensaje enviado con éxito")
    print("Respuesta:", response_data)
else:
    print("Error al enviar el mensaje")
    print("Código de estado:", response.status)
    print("Respuesta:", response_data)

# Cerrar la conexión
conn.close()

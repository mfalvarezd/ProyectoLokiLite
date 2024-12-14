#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
int main(){
    
    CURL *curl;
    CURLcode res;
    const char *account_sid =getenv("TWILIO_ACCOUNT_SID");
    const char *auth_token = getenv("TWILIO_AUTH_TOKEN");
    if(!account_sid || !auth_token){
        fprintf(stderr,"Error: Las variables de entorno no estan configuradas");
        return 1;
    }
    const char *url ="https://api.twilio.com/2010-04-01/Accounts/";
    char full_url[256];
    snprintf(full_url,sizeof(full_url),"%s%s/Messages.json",url,account_sid);
    const char *post_fields ="To=whatsapp:593986849600&From=whatsapp:14155238886&Body=prueba";
    curl = curl_easy_init();
     
    if(curl){
        curl_easy_setopt(curl,CURLOPT_URL,full_url);

        curl_easy_setopt(curl,CURLOPT_USERNAME,account_sid);

        curl_easy_setopt(curl,CURLOPT_PASSWORD,auth_token);

        curl_easy_setopt(curl,CURLOPT_POSTFIELDS,post_fields);
        
        res = curl_easy_perform(curl);
        if (res != CURLE_OK){
            fprintf(stderr,"Error en curl: %s\n",curl_easy_strerror(res));
        }else{
            printf("Mensaje enviado con Exito");
        }

        curl_easy_cleanup(curl);
    }

   

    
    return 0;
}
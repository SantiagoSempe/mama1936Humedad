/*
 * File:   esp8266_mef.c
 * Author: g_fer
 *
 * Created on 21 de mayo de 2021, 14:07aasd
 */


#include <xc.h>
#include "esp8266_mef.h"
#include "esp8266.h"
#include "tick.h"
#include "user.h"
#include "uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ssid.h"
#include "dust.h"
#include "AHT10.h"




estado_esp8266_t estado_actual,estado_siguiente;
bool msg=1;
estadoMensaje_t estadoActualMensaje;
unsigned char send_msg[6]="START"; 


void actualizarESP8266MEF(){
    static bool connected=false;
    
    unsigned char* index;
    uint8_t i;
    char* aux;
    static unsigned char resp[40]; //me robe 10 de aca.

    static unsigned char espec[20];
    static uint8_t size;
    static tick_t timeout;
    static tick_t tinicio;
    unsigned char cmd[50];
    switch(estado_actual){ //Borrar case CHEKING  no funciona
        case IDDLE_ESP:
            if((connected == true) && (msg == true)){
                estado_actual = SENDING_PREPARE;
            }
            else if(connected==false){
                estado_actual=CONNECTING;
            }
            else{
                estado_actual = IDDLE_ESP;
            }
            break;
        case CONNECTING:
            sprintf(cmd,"AT+CWJAP_CUR=\"%s\",\"%s\"",SSID,PSWD);
            sendATCmd(cmd);
            tinicio=tickRead();
            timeout = VERY_LONG_TIMEOUT;
            strcpy(espec,"CONNECT");
            estado_actual=WAITING_RESPONSE;
            estado_siguiente=STARTING;
            break;
        case WAITING_RESPONSE:
            if(tickRead()-tinicio>timeout){
                estado_actual=CHECKING_RESPONSE;
            }
            break;    
        case CHECKING_RESPONSE:
            if(!checkResponse(resp,espec)){
                estado_actual=ERROR;
            }
            else{
                estado_actual=estado_siguiente;
            }
            break;
        case STARTING:
            sprintf(cmd,"AT+CIPSTART=0,\"%s\",\"%s\",%s",PROTOCOL,IP,PORT);
            sendATCmd(cmd);
            tinicio=tickRead();
            timeout = LONG_TIMEOUT;
            strcpy(espec,"CONNECT");
            estado_actual=WAITING_RESPONSE;
            estado_siguiente=CONNECTED;
            break;
        case CONNECTED:
            connected=true;
            estado_actual=IDDLE_ESP;
            break;
        case SENDING_PREPARE:
            sprintf(cmd,"AT+CIPSEND=0,%u",strlen(send_msg)+2);
            sendATCmd(cmd);
            tinicio=tickRead();
            timeout = STD_TIMEOUT;
            strcpy(espec,">");
            estado_actual=WAITING_RESPONSE;
            estado_siguiente=SENDING_MSG;
            break;
        case SENDING_MSG:
            strcpy(cmd,send_msg); //aca manda el mensaje
            sendATCmd(cmd);
            tinicio=tickRead();
            timeout = RESPONSE_TIMEOUT;
            strcpy(espec,"OK");
            estado_actual=WAITING_RESPONSE;
            estado_siguiente=LOOKING_RECV;
            break;
        case ERROR:
            if(estado_siguiente==SENDING_MSG){
                
                estado_actual=SENDING_PREPARE;
            }
            break;
        case PARSE_RESPONSE:
            if((index=strstr(resp,":"))!=NULL){
                for(i=0;i<size;i++){
                    resp[i]=*(index+i+1);
                }
                resp[i]=0;
//                sprintf(send_msg,"%2u",resp);//aca reemplaza la respuesta
                if(strcmp(resp,"OK")==0){
                    estado_actual=IDDLE_ESP;
                    msg=0;
                }
                else{
                    estado_actual=ERROR;
                }
                
            }
            else{
                estado_actual=ERROR;
            }
            break;
        case LOOKING_RECV:
            aux=strstr(bufferRx,"+IPD");
            if(aux!=NULL){
                memset(resp,0,sizeof(resp));
                memcpy(resp,aux,7);
                size=*(aux+7)-0x30;
                memcpy(resp+7,aux+7,size+2);
                memset(bufferRx,0,80);
                estado_actual=PARSE_RESPONSE;
            }
            else{
                estado_actual=ERROR;
            }
            break;
            
            
        default:
            iniciarESP8266MEF();
        return;
    }
}
void iniciarESP8266MEF(void){
    estado_actual=IDDLE_ESP;
}

void iniciarMEFMensaje(void){
    estadoActualMensaje=GENERATE_MSG;
}

void actualizarMEFMensaje(void){
    static uint8_t sensor=0;
    static tick_t tinicio;
    switch (estadoActualMensaje){
        case GENERATE_MSG:
            if(msg==0 && DUSTFlag == 1 && sensor == 0){
               sprintf(send_msg,"%s", resStrDUST);//aca reemplaza la respuesta
               //sprintf(send_msg, "10");
               msg = 1;
               DUSTFlag = 0;
               //sensor =1;
               tinicio=tickRead();
               estadoActualMensaje=IDLE;
            }
//            if(msg == 0 && AHTFlag == 1 && sensor == 1){
//               sprintf(send_msg,"A");//aca reemplaza la respuesta
//               msg = 1;
//               AHTFlag = 0;
//               sensor = 0;
//               tinicio=tickRead();
//               estadoActualMensaje=IDLE;
//            }
            break;
        case IDLE:
            if(tickRead()-tinicio>WAIT_TIME_MSG){
                estadoActualMensaje=GENERATE_MSG;
            }
            break;
        default:
            iniciarMEFMensaje();
        
    }
    
}

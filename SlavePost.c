/*
 * File:   SlavePost.c
 * Author: Jorge Cerón
 *
 * Created on 8 de agosto de 2022, 12:51 PM
 */
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSC oscillator: CLKOUT function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)
// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)
// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h> 
#include <stdio.h>
#include "spi.h"
#include "oscilador.h"
#include "adc.h"
#define _XTAL_FREQ 1000000
#define RS RC6
#define EN RC7
#define IN_MIN 0
#define IN_MAX 1023 
#define OUT_MIN 0
#define OUT_MAX 500

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint16_t ADRESH_1 = 0;
uint16_t ADRESLS_1 = 0;
uint16_t ADRESH_2 = 0;
uint16_t ADRESLS_2 = 0;
uint8_t ENVIO = 0;

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() SPI_slave(void){
    if(PIR1bits.ADIF){                      
        if(ADCON0bits.CHS == 0){                    // POT1
            ADRESH_1 = (adc_read()>>2) & 255; 
            ADRESLS_1 = adc_read()& 3;
        } else if(ADCON0bits.CHS == 1){
            ADRESH_2 = (adc_read()>>2) & 255;
            ADRESLS_2 = adc_read()& 3;
        }
        PIR1bits.ADIF = 0;                          // Limpiar bandera ADC
    }
    if (PIR1bits.SSPIF){
        ENVIO = spiRead();                          // Se lee lo que se desea enviar al Master
        if (ENVIO == 72){
            spiWrite(ADRESH_1);                     // "H" -> Envio de ADRESH POT1
        } else if (ENVIO == 76) {
            spiWrite(ADRESLS_1);                    // "L" -> Envio de ADRESL POT1
        } else if (ENVIO == 65){
            spiWrite(ADRESH_2);                     // "A" -> Envio de ADRESH POT2
        } else if (ENVIO == 66){
            spiWrite(ADRESLS_2);                    // "B" -> Envio de ADRESL POT2
        }
        PIR1bits.SSPIF = 0;
    }
    return;
}

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

void main(void) {
    int_osc_MHz(1);
    adc_init(0, 0, 0);
    setup(); 
    while(1){
        if (ADCON0bits.GO == 0){
            if(ADCON0bits.CHS == 0){        // Interrupción por AN0
                ADCON0bits.CHS = 0b0001;    // Cambio de AN0 a AN1
            }
            else if (ADCON0bits.CHS == 1){  // Interrupción por AN1
                ADCON0bits.CHS = 0b0000;    // Cambio de AN1 a AN0         
            }
             __delay_us(40);                // Sample time
            ADCON0bits.GO = 1;              // On
        }
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){       
    // Configuración de puertos
    ANSEL = 0b00000011;         // RA0 Y RA1 como analógicas, demás como I/O digitales
    ANSELH = 0b00000000;        // I/O digitales
    TRISAbits.TRISA5 = 1;       // SS como entrada
    TRISAbits.TRISA0 = 1;       // RA0 como entrada
    TRISAbits.TRISA1 = 1;       // RA1 como entrada
    PORTA = 0b00000000;         // Limpiar PORTA
    spiInit(SPI_SLAVE_SS_EN, SPI_DATA_SAMPLE_MIDDLE, SPI_CLOCK_IDLE_LOW, SPI_IDLE_2_ACTIVE);
    // Configuración de interrupciones
    INTCONbits.GIE = 1;         // Habilitar interrupciones globales
    PIR1bits.SSPIF = 0;         // Limpiar bandera de SPI
    PIE1bits.SSPIE = 1;         // Habilitar interrupciones de SPI
    INTCONbits.PEIE = 1;        // Habilitar interrupciones de perifericos
    return;
}
    
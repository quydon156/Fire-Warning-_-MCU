#include "stm32f10x.h"                  // Device header

//LCD
#include "systick_time.h"
#include "lcd_1602_drive.h"

// UART
#include "gp_drive.h"
#include "uart_drive.h"

// ADC
#include "adc_drive.h"
#include "msg_drive.h"
#include <time.h>
#include <stdio.h> 
#include <math.h>

// DIGITAL
#include <stdint.h>

// PWM
#include "timer_drive.h"

// Program
static int mode = 0;
static int warning_ADC = 0;
static int warning_FS = 0;
static int warning_CAM = 0;
// <----------LCD START---------->
// LIB require: gp_drive, i2c_drive, lcd_1602_drive, PCF8574_drive, systick_time.
/*
	I2C2
	PB10 -> SCL
	PB11 -> SDA

	I2C1
	PB6 -> SCL
	PB7 -> SDA
*/
static int showDHT = 1;
// update init for 4 bit data transfer 
// <----------LCD END------------>

// <----------UART START--------->
// LIB require: add more help_func, uart_drive
// If using USART1 clock speed 72Mhz, else 36Mhz
/*
	USART3 -> PB10 (Tx) and PB11(Rx)
	USART2 -> PA2 (Tx) and PA3(Rx)
	USART1 -> PA9 (Tx) and PA10(Rx)
*/
/*
UART Manager
0- count
1- signal
2- Bridge
3- Terminator should 1: Terminator / 0: Interrupt
4- terminator char 
5- time cst
6- time counter 

*/
/*
char USART_1_msg[250];
unsigned short USART_1_cnt = 0;
unsigned short USART_1_sig = 0;
unsigned short USART_1_bdg = 0;
*/
unsigned short uart_1_mgr[7]={0,0,0,0,0,0,0};

char USART_2_msg[250];
unsigned short USART_2_cnt = 0;
unsigned short USART_2_sig = 0;
unsigned short USART_2_bdg = 0;
unsigned short uart_2_mgr[7]={0,0,3,0,'\0',0,0};

char USART_3_msg[250];
unsigned short USART_3_cnt = 0;
unsigned short USART_3_sig = 0;
unsigned short USART_3_bdg = 0;
unsigned short uart_3_mgr[7]={0,0,2,0,'\0',0,0};

static char sigUART = 'K';
static char dataUART[15];
static char temp[5];
static char hum[5];
static int i = 0;
static int delayLCD = 0;
static char ImageProcess[3];
static int countImg = 0;

// <-----------UART END---------->

// <-----------ADC START--------->
// LIB require: adc_drive, gp_drive, help_func, msg_drive,systick_time, uart_drive to see response.
/*
PA0 -> ADC12_IN0
PA1 -> ADC12_IN1
PA2 -> ADC12_IN2
PA3 -> ADC12_IN3
PA4 -> ADC12_IN4
PA5 -> ADC12_IN5
PA6 -> ADC12_IN6
PA7 -> ADC12_IN7
PB0 -> ADC12_IN8
PB1 -> ADC12_IN9

PC0 -> ADC12_IN10
PC1 -> ADC12_IN11
PC2 -> ADC12_IN12
PC3 -> ADC12_IN13
PC4 -> ADC12_IN14
PC5 -> ADC12_IN15

ADC12_IN16 input channel which is used to convert the sensor output voltage into a digital value.

*/
char adc_channels[17] = {PA0};
int analog_rx[17];
char channels = 1;

// Variables
//int percent = 0;
static int cnv = 0;
// <------------ADC END---------->

// <-----------DIGITAL START--------->
// LIB require: gp_drive
static int flameSen = 1;
// <-----------DIGITAL END----------->

// <-------------PWM START----------->
// LIB require: timer_drive
/*
PA0 -> TIM2_CH1
PA1 -> TIM2_CH2
PA2 -> TIM2_CH3
PA3 -> TIM2_CH4

PA6 -> TIM3_CH1
PA7 -> TIM3_CH2
PB0 -> TIM3_CH3
PB1 -> TIM3_CH4

PA8 -> TIM1_CH1
PA9 -> TIM1_CH2
PA10 -> TIM1_CH3
PA11 -> TIM1_CH4


void TIM1_UP_IRQHandler(){}
void TIM2_IRQHandler(){}
void TIM3_IRQHandler(){}
void TIM4_IRQHandler(){}
*/
// <--------------PWM END------------>

int time_Warning = 0;

int main(void)
{
		
		systick_init();// initialize the delay function (Must initialize)
		
		// Init UART
		//UART_init(1,115200);
		UART_init(2,115200);
		UART_init(3,115200);
		DelayMs(100);
		
		// NOTE: Prequitesite 1: Init LCD after Init UART
		// Init LCD
		lcd_i2c_init(1);
		
		//ADC
		adc_multi_ch_init(adc1, channels, adc_channels);
	
		// INIT DIGITAL
		init_GP(PB,12,IN,I_PP); //input mode
		
		// PWM
		
		
		// FUNCTION PROGRAM
		dataUART[9] = '0';
		
	while(1)
	{
		// ADC
		adc_multi_ch_rx(adc1, channels, adc_channels, analog_rx);
		cnv = analog_rx[0];
		if (cnv >= 500)
		{
			warning_ADC = 1;
		}
		if (cnv < 500)
		{
			warning_ADC = 0;
		}
		
		// DIGITAL
		flameSen = R_GP(PB, 12);
		if (flameSen == 0)
		{
			warning_FS = 1;
		}
		if	(flameSen == 1)
		{
			warning_FS = 0;
		}
		
		// GET DATA VIA UART
		//ImageProcess[0] = '0';
		sigUART = UART_RX(3);
		UART_TX(2,sigUART);			//TEST
		// SET FRAME DATA
		if(sigUART != 'E')
		{
			dataUART[i] = sigUART;
		}
		// BREAK FRAME
		if(sigUART == 'E')
		{
			ImageProcess[0] = dataUART[9];
			i = 0;
		}
		i++;
		// DATA FROM IMAGE PROCESSING
		if (ImageProcess[0] == 'F'){
			//countImg++;
			warning_CAM = 1;
		}
		if (ImageProcess[0] == 'N'){
			//countImg = 0;
			warning_CAM = 0;
		}
		/*
		if(countImg == 3){
			warning_CAM = 1;
		}
		if(countImg == 3){
			warning_CAM = 0;
		}
		*/
		if (showDHT == 0){
			if (warning_ADC == 0 & warning_FS == 0 & warning_CAM == 0){
				showDHT = 1;
				lcd_i2c_init(1);
			}
		}
		
		// DHT
		if (delayLCD > 180 & sigUART == 'E'){
			//param[0] = dataUART[0]	// NULL character
			hum[0] = dataUART[1];
			hum[1] = dataUART[2];
			hum[2] = dataUART[3];
			hum[3] = dataUART[4];
			temp[0] = dataUART[5];
			temp[1] = dataUART[6];
			temp[2] = dataUART[7];
			temp[3] = dataUART[8];
			
			lcd_i2c_msg(1,1, 0,"NHIET DO:");
			lcd_i2c_msg(1,1, 14,"oC");
			lcd_i2c_msg(1,2, 0,"DO AM:");
			lcd_i2c_msg(1,2,12,"%");
			lcd_i2c_msg(1,1,10,temp);
			lcd_i2c_msg(1,2,7,hum);
			delayLCD =0;
		}
		delayLCD++;
		/*
		if(showDHT == 1){
			lcd_i2c_msg(1,1, 0,"NHIET DO:");
			lcd_i2c_msg(1,1, 14,"oC");
			lcd_i2c_msg(1,2, 0,"DO AM:");
			lcd_i2c_msg(1,2,12,"%");
		}
		// NORMAL MODE
		if(mode == 0) {
			showDHT = 1;
		}
		*/
		// WARNING MODE
		if(warning_ADC == 1 || warning_FS == 1 || warning_CAM == 1) {
			delayLCD = 0;
			// CALLING->>>>
			//USER1
			UART_SEND(2,"ATD +84961085803;\r\n");
			DelayMs(5000);
			UART_SEND(2,"ATH\r\n");
			DelayMs(100);
			//USER2
			UART_SEND(2,"ATD +84364435299;\r\n");
			DelayMs(5000);
			UART_SEND(2,"ATH\r\n");
			DelayMs(100);
			
			// SENDING MESSAGES->>>>
			//USER1
			UART_SEND(2,"AT+CMGF=1\r\n");
			DelayMs(500);
			UART_SEND(2,"AT+CMGS=\"+84961085803\"\r\n");
			DelayMs(500);
			UART_SEND(2,"PHAT HIEN NGUY HIEM CHAY NO");
			DelayMs(300);
			UART_TX(2,0x1A);
			DelayMs(100);
			UART_SEND(2,"ATH\r\n");
			DelayMs(100);
			
			// SHOWING LCD
			showDHT = 0;
			lcd_i2c_init(1);
			lcd_i2c_msg(1,1,4,"NGUY HIEM");
			if(warning_ADC == 1){
				lcd_i2c_msg(1,2,0,"CANH BAO KHI GAS");
			} 
			if(warning_FS == 1){
				lcd_i2c_msg(1,2,0,"CANH BAO CO LUA");
			}
			if(warning_CAM == 1){
				lcd_i2c_msg(1,2,0,"PHAT HIEN CO LUA");
			}

			dataUART[9] = 'N';
			
		// PWM
			// BUZZER
			timer_pwm_micro(PA,11,5000,500);
			TDelay_Milli(500);
			timer_pwm_micro(PA,11,5000,1000);
			TDelay_Milli(500);
			
			// SERVO SG90
				// SET ANGEL
			servo_180_deg(PA, 8, 0);
			TDelay_Milli(500);					//500
			//servo_180_deg(PA, 8, 0);
			//TDelay_Milli(500);
			
		} else {
			servo_180_deg(PA, 8, 160);
		}
		//if (mode == 1 
	}
}

// FUNCTION INTERRUPT UART

void USART2_IRQHandler()
{
	UART_ISR(2,uart_2_mgr, USART_2_msg);
}

void USART3_IRQHandler()
{
	UART_ISR(3,uart_3_mgr, USART_3_msg);
}

void SysTick_Handler(void)
{
	systick_int(uart_1_mgr,uart_2_mgr,uart_3_mgr);
}
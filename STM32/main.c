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

static char sigUART = 'K';
static char dataUART[15];
static char temp[5];
static char hum[5];
static int i = 0;
static int delayLCD = 0;
static char ImageProcess[3];
static int countImg = 0;
static int trigg=0;

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
static char adc_channels[17] = {PA0};
static int analog_rx[17];
static char channels = 1;

// Variables
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


// FUNCTION INTERRUPT UART

void SysTick_Handler(void)
{
	
}

// FUNCTION PROJECT
// SIM800L
void SIM800L (void)
{
	// CALLING->>>>
	//USER1
	UART_SEND(2,"ATD +84917668362;\r\n");
	DelayMs(15000);
	UART_SEND(2,"ATH\r\n");
	DelayMs(1000);
	//USER2
	UART_SEND(2,"ATD +84916121100;\r\n");
	DelayMs(15000);
	UART_SEND(2,"ATH\r\n");
	DelayMs(100);
			
	// SENDING MESSAGES->>>>
	//USER1
	/*
	UART_SEND(2,"AT+CMGF=1\r\n");
	DelayMs(500);
	UART_SEND(2,"AT+CMGS=\"+84916121100\"\r\n");
	DelayMs(500);
	if(warning_ADC == 1)
	{
		UART_SEND(2,"CANH BAO!! PHAT HIEN CO KHI GAS");
	} else if(warning_FS == 1)
	{
		UART_SEND(2,"CANH BAO!! PHAT HIEN CO LUA");
	} else if(warning_CAM == 1)
	{
		UART_SEND(2,"CANH BAO!! CAMERA NHAN DIEN THAY LUA");
	}
	DelayMs(300);
	UART_TX(2,0x1A);
	DelayMs(100);
	UART_SEND(2,"ATH\r\n");
	DelayMs(100);
	*/
	dataUART[9] = 'N';
	ImageProcess[0] = 'N';
}

// WARNING TO LCD
void WARNING_LCD(void)
{
	// SHOWING LCD
	showDHT = 0;
	if(warning_ADC == 1){
		lcd_i2c_init(1);
		lcd_i2c_msg(1,1,4,"NGUY HIEM");
		lcd_i2c_msg(1,2,0,"CANH BAO KHI GAS");
	} 
	if(warning_FS == 1){
		lcd_i2c_init(1);
		lcd_i2c_msg(1,1,4,"NGUY HIEM");
		lcd_i2c_msg(1,2,0,"CANH BAO CO LUA");
	}
	if(warning_CAM == 1){
		lcd_i2c_init(1);
		lcd_i2c_msg(1,1,4,"NGUY HIEM");
		lcd_i2c_msg(1,2,0,"NHAN DIEN CO LUA");
	}
}

// WARNING ACTION
void WARNING_ACTION(void)
{
	// PWM
	// BUZZER
	//timer_pwm_micro(PA,9,1000,600);
	W_GP(PA,9,1);
	TDelay_Milli(500);
			
	//SERVO SG90
	// SET ANGEL
	servo_180_deg(PA, 8, 160);
	TDelay_Milli(500);					//500
}

// GET DATA UART
void GetData_UART(void)
{
	// GET DATA VIA UART
	//ImageProcess[0] = '0';
	sigUART = UART_RX(3);
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
	delayLCD++;
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
}

// RESET STATUS
void Reset_Status(void)
{
	if (showDHT == 0){
		if (warning_ADC == 0 & warning_FS == 0 & warning_CAM == 0){
			showDHT = 1;
			lcd_i2c_init(1);
		}
	}
}

void Trigger(void)
{
	if (warning_ADC == 1 || warning_FS == 1 || warning_CAM == 1)
	{
		WARNING_LCD();
		WARNING_ACTION();
		SIM800L();
		trigg = 1;
	}
}

void UnTrigger(void)
{
	if (warning_ADC == 0 & warning_FS == 0 & warning_CAM == 0)
	{
		if(trigg == 1)
		{
			servo_180_deg(PA, 8, 0);
			W_GP(PA,9,0);
			trigg = 0;
		}
	}
}
// GET DATA ADC
void GetData_ADC(void)
{
	adc_multi_ch_rx(adc1, channels, adc_channels, analog_rx);
	cnv = analog_rx[0];
	if (cnv >= 500)
		{
			warning_ADC = 1;
			Trigger();
		} else
		{
			warning_ADC = 0;
			UnTrigger();
			//servo_180_deg(PA, 8, 0);
		}
}

// GET DATA Flame Sensor
void GetData_Dig(void)
{
	flameSen = R_GP(PB, 12);
	if (flameSen == 0)
	{
		warning_FS = 1;
		Trigger();
	}
	if	(flameSen == 1)
	{
		warning_FS = 0;
		UnTrigger();
		//servo_180_deg(PA, 8, 0);
		
	}
}

// GET DATA IMAGE PROCESSING
void GetData_IMG(void)
{
	// DATA FROM IMAGE PROCESSING
	if (ImageProcess[0] == 'F'){
		//countImg++;
		warning_CAM = 1;
		Trigger();
	}
	if (ImageProcess[0] == 'N'){
	//countImg = 0;
		warning_CAM = 0;
		UnTrigger();
	}
}

void Mode1(void)
{

}
int main(void)
{
		systick_init();// initialize the delay function (Must initialize)
		
		// Init UART
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

		
		// FUNCTION PROGRAM
		init_GP(PA,9,OUT50,O_GP_PP);
		dataUART[9] = '0';
		servo_180_deg(PA, 8, 0);
		
	while(1)
	{
		// MQ-2 Sensor
		GetData_ADC();
		
		// Flame Sensor
		GetData_Dig();
		// DHT Sensor
		
		// DHT
		GetData_UART();
		GetData_IMG();

		// Mode Warning
		Reset_Status();
	}
}
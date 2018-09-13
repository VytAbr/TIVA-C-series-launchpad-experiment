// Tiva launchpad testing
// made by:
// Vytautas Abromavicius

#include "ADCSWTrigger.h"
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "Nokia5110.h"
#include "UART.h"

#define wait_time 200

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

void Delay(void);
void delay_long(unsigned long halfsecs);

volatile unsigned long ADCvalue;
unsigned char String1[10];
volatile unsigned long Counts = 0;
volatile unsigned long Total_time = 30000;
volatile unsigned long triggeris = 0;
volatile unsigned long zema_itampa = 15000;
volatile unsigned long auksta_itampa = 20000;
volatile unsigned long spalva = 0x01;
volatile int skirtumas = 10;
volatile int greitis = 0;
char uart_busena = 0;

void PortB_Init(void){ 
	volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000002;     //activate Port B clock
  delay = SYSCTL_RCGC2_R;           //delay for clock
  GPIO_PORTB_AMSEL_R = 0x00;        //deaktivate analog
  GPIO_PORTB_PCTL_R = 0x00;   //clear register
  GPIO_PORTB_DIR_R = 0x07;          // PB0 - PB2 as out
  GPIO_PORTB_AFSEL_R = 0x00;        //Alt functions off
  GPIO_PORTB_DEN_R = 0xFF;          //port on
}

void UART_ConvertUDec1(unsigned long n){
	unsigned int temp;
	
  if (n > 9999)	{
		String1[0] = '-';
		String1[1] = '-';
		String1[2] = '.';
		String1[3] = '-';
		String1[4] = '-';
		String1[5] = '\0';
	}
	else	{
		String1[2] = '.';
		temp = n/1000;
		if (temp == 0) String1[0] = '0';
		else String1[0] = temp + 0x30;
	
		n = n%1000;
		temp = n/100;
		if (temp == 0) String1[1] = '0';
		else String1[1] = temp + 0x30;

		n = n%100;
		temp = n/10;
		if (temp < 1)	String1[3] = '0';
		else String1[3] = temp + 0x30;

		n = n%10;	
		temp = n;
		if (temp < 1)	String1[4] = '0';
		else String1[4] = temp + 0x30;
		String1[5] = ' ';
		String1[6] = 'C';
		String1[7] = '\0';	
	}
}

void SysTick_Handler(void){	
	if (auksta_itampa > 29000) skirtumas = -1 * (greitis+1);
	if (auksta_itampa < 50)	skirtumas = 1 * (greitis+1);
	auksta_itampa += skirtumas;
	
	if (triggeris == 0) {
		zema_itampa = Total_time - auksta_itampa; 	
		NVIC_ST_RELOAD_R= zema_itampa;
		triggeris = 1;
		GPIO_PORTB_DATA_R &= ~0x07;
	}
	else	{
		NVIC_ST_RELOAD_R= auksta_itampa;
		triggeris = 0;	
		GPIO_PORTB_DATA_R |= spalva;	
	}

}

void SysTick_Init(unsigned long period){
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = period-1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                              // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = 0x07;
  EnableInterrupts();
}

void red_out(void){
	spalva = 0x01;
	Nokia5110_SetCursor(8, 4);
  Nokia5110_OutString("1");
	if (uart_busena != 1)	{
		UART_OutString("\n\n\r Busena 1");	
		UART_OutString("\n\r Branduolio temperatura: ");
		UART_OutString(String1);
	}
	uart_busena = 1;
}

void green_out(void){
	spalva = 0x02;
	Nokia5110_SetCursor(8, 4);
  Nokia5110_OutString("2");
	if (uart_busena != 2)	{
		UART_OutString("\n\n\r Busena 2");	
		UART_OutString("\n\r Branduolio temperatura: ");
		UART_OutString(String1);
	}
	uart_busena = 2;
}

void yellow_out(void){
	spalva = 0x04;
	Nokia5110_SetCursor(8, 4);
  Nokia5110_OutString("3");
	if (uart_busena != 3)	{
		UART_OutString("\n\n\r Busena 3");	
		UART_OutString("\n\r Branduolio temperatura: ");
		UART_OutString(String1);
	}
	uart_busena = 3;
}

void all_out(void){
	spalva = 0x07;
	Nokia5110_SetCursor(8, 4);
  Nokia5110_OutString("4");
	if (uart_busena != 4)	{
		UART_OutString("\n\n\r Busena 4");
		UART_OutString("\n\r Branduolio temperatura: ");
		UART_OutString(String1);	
	}
	uart_busena = 4;
}

struct State {
  void (*CmdPt)(void);   // output function
  unsigned long Time;    // wait time, 12.5ns units
  unsigned long Next[3];}; 
typedef const struct State StateType;
#define R0  0
#define G0  1
#define Y0  2
#define RGY0 3
StateType FSM[9]={
  {&red_out,    wait_time,{R0,G0,RGY0}},      // Turn ON PB0(red) led
  {&green_out,  wait_time,{G0,Y0,RGY0}},       //Turn ON PB1(green) led
  {&yellow_out, wait_time,{Y0,R0,RGY0}},     // Turn ON PB2(yellow) led
  {&all_out,  wait_time,{RGY0,R0,RGY0}}};   // PB0:2 leds ON

int main(void)	{
	unsigned long volatile delay;
	unsigned long adc01, adc02;
	unsigned long S; // index into current state	
  unsigned long Input;
	unsigned int count11;
	float temperature;
	float sum_temperature;
		
  PLL_Init();                           // 80 MHz
	PortB_Init();					//PORTB0:2 as outputs
	Nokia5110_Init();				
	UART_Init();              // initialize UART
  Nokia5110_Clear();
  Nokia5110_SetCursor(1, 0);
  Nokia5110_OutString("Namu darbas");
	Nokia5110_SetCursor(1, 1);
  Nokia5110_OutString("  Moore'o");
	Nokia5110_SetCursor(0, 2);
  Nokia5110_OutString("buviu masina");	
	Nokia5110_SetCursor(1, 4);
  Nokia5110_OutString("Busena 0");
	Nokia5110_SetCursor(1, 5);
  Nokia5110_OutString("T =");

  String1[0] = '-';
	String1[1] = '-';
	String1[2] = '.';
	String1[3] = '-';
	String1[4] = '-';
	String1[5] = '\0';
  ADC0_InitSWTriggerSeq3_Ch1();         // ADC initialization PE2/AIN1

	Counts = 0;
	triggeris = 0;
	SysTick_Init(Total_time);        // initialize SysTick timer
  EnableInterrupts();
  S = R0;
	sum_temperature = 0;
	count11 = 0;
	//Default information to Nokia screen
  UART_OutString("Sveiki atvyke");
	UART_OutString("\n\r Iterptiniu sistemu inzinerijos");
	UART_OutString("\n\r Namu darbas");
	UART_OutString("\n\r Tiva launchpad tyrimas");
  while(1){
	ADC_In21(&adc01, &adc02);
		
		adc02 = adc02 / 100;
		greitis = adc02;
		
		temperature = 147.5 - ((75 * 3.2 * adc01)/ 4096);
		temperature = temperature * 100;
		sum_temperature += temperature;
		count11++;
		if (count11 >= 5)		{
			sum_temperature = sum_temperature /count11;
			UART_ConvertUDec1(sum_temperature);
			Nokia5110_SetCursor(5, 5);
			Nokia5110_OutString( String1 );
			count11 = 0;
			sum_temperature = 0;		
		}

		if((GPIO_PORTB_DATA_R&0x40))		{
			Delay();
			Input = 1;	
		}
		else if((GPIO_PORTB_DATA_R&0x80))		{
			Delay();
			Input = 2;	
		}
		else Input = 0;

		(FSM[S].CmdPt)();           // call output function
		delay_long(FSM[S].Time);
		S = FSM[S].Next[Input];     // next
  }
}
	
void Delay(void){
	unsigned long volatile time;
  time = 72724*200/91;  // 0.1sec
  while(time){
		time--;
  }
}

void delay_long(unsigned long time){
  unsigned long i;
  while(time > 0){
    i = 10000;
    while(i > 0){
      i--;
    }
    time--;
  }
}

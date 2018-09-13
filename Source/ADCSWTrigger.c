// ADC library

#include "tm4c123gh6pm.h"

// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: software trigger
// SS3 1st sample source: Ain1 (PE2)
// SS3 interrupts: flag set on completion but no interrupt requested
void ADC0_InitSWTriggerSeq3_Ch1(void)
{
	
	volatile unsigned long delay;
	
	SYSCTL_RCGC0_R |= 0x00010000;   // 6) activate ADC0 
	//SYSCTL_RCGCADC_R |= 0x00000001;
			
  SYSCTL_RCGC2_R |= SYSCTL_RCGCGPIO_R4;   // 1) activate clock for Port E
	
  delay = SYSCTL_RCGC2_R;         //    allow time for clock to stabilize
	//delay = SYSCTL_RCGCGPIO_R;
	//delay = SYSCTL_RCGCGPIO_R;
	
  GPIO_PORTE_DIR_R &= ~0x06;      // 2) make PE2, PE1 input
	
  GPIO_PORTE_AFSEL_R |= 0x06;     // 3) enable alternate function on PE2
	
  GPIO_PORTE_DEN_R &= ~0x06;      // 4) disable digital I/O on PE2
	
//	GPIO_PORTE_PCTL_R = GPIO_PORTE_PCTL_R&0xFFFFF00F;
  GPIO_PORTE_AMSEL_R |= 0x06;     // 5) enable analog function on PE2
	
	//ADC0_PC_R &= ~0xF;              // 8) clear max sample rate field
 // ADC0_PC_R |= 0x1;               //    configure for 125K samples/sec
 SYSCTL_RCGC0_R &= ~0x00000300;
	
  ADC0_SSPRI_R = 0x3210;          // 9) Sequencer 3 is highest priority
	
  ADC0_ACTSS_R &= ~0x0004;        // 10) disable sample sequencer 2
  ADC0_EMUX_R &= ~0x0F00;         // 11) seq2 is software trigger
  ADC0_SSMUX2_R = 0x0012;         // 12) set channels for SS2
  ADC0_SSCTL2_R = 0x0068;         // 13) no D0 END0 IE0 TS0 D1 END1 IE1 TS1 D2 TS2, yes END2 IE2
 // ADC0_IM_R &= ~0x0004;           // 14) disable SS2 interrupts
  ADC0_ACTSS_R |= 0x0004;         // 15) enable sample sequencer 2
	

}






//------------ADC0_InSeq3------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
unsigned long ADC0_InSeq3(void){  
	unsigned long result;
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
  ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
  return result;
}


void ADC_In21(unsigned long *ain2, unsigned long *ain1){
  ADC0_PSSI_R = 0x0004;            // 1) initiate SS2
  while((ADC0_RIS_R&0x04)==0){};   // 2) wait for conversion done
  *ain2 = ADC0_SSFIFO2_R&0xFFF;    // 3A) read first result
  *ain1 = ADC0_SSFIFO2_R&0xFFF;    // 3B) read second result
  ADC0_ISC_R = 0x0004;             // 4) acknowledge completion
}



#include "init.h"

int int_200_arrived = 0;
int int_400_arrived = 0;
int fuel_level, coolant_temp;
int stability_control_is_alive = 1;
unsigned char doors_status = 3; //00000011


void LED_RESET(void){
	/* turn off all the LED */
    LED0 = 1;
    LED1 = 1;
    LED2 = 1;
    LED3 = 1;
    LED4 = 1;
    LED5 = 1;
    LED6 = 1;
    LED7 = 1;
}


void main(void)
{
    /* board initialization */
    Init();
    LED_RESET();
     
	PIT_ConfigureTimer(0, 200); //Channel 0 configure  PIT timer for 200ms
	PIT_ConfigureTimer(1, 400); //Channel 1 configure PIT timer for 400ms
	
	/* start PIT timer for channel 0 and 1 */
	PIT_StartTimer(0);
	PIT_StartTimer(1);

    for(;;)
    {
		if(int_200_arrived) {
			/* Read photo registor (light sensor) and send through bus*/
			CAN_0.BUF[0].DATA.B[0] = ADC_0.CDR[2].B.CDATA;
			CAN_0.BUF[0].DATA.B[1] =  ADC_0.CDR[2].B.CDATA >> 8;		
			CAN_0.BUF[0].CS.B.CODE = 12; //Transmit remote frame unconditionally
			
			/* send doors status SW3, SW4 switch (left door and right door sensor respectively)*/
			if(SW3 == 0){ //if left door is open
				doors_status &= 254; //11111110 
			}
			
			else{ //left door is closed
				doors_status |= 1; //00000001
			} 
				
			if(SW4 == 0){ //right door is open
				doors_status &= 253; //11111101
			}
			
			else{	// right door is closed
					doors_status |= 2; //00000010
			} 
			

			CAN_0.BUF[1].DATA.B[0] = doors_status; //write the status of the door		
			CAN_0.BUF[1].CS.B.CODE = 12; // transmit remote frame unconditionally 

			// send alive signal
			CAN_0.BUF[4].CS.B.CODE = 12;
			
			// send error signal
			if(stability_control_is_alive == 0) {
				CAN_0.BUF[5].CS.B.CODE = 12;
				LED7 = ~LED7;
			} 

			else {
				stability_control_is_alive = 0;
				LED7 = 1;
			}
			
			int_200_arrived = 0;
		}
		
		
		if(int_400_arrived) {
			/* send fuel level warning message */	
			if(fuel_level <= 102){ // fuel level reaches 1/10 th of the maximum
				CAN_0.BUF[2].DATA.B[0] = 0x00;
			}
			else{
				CAN_0.BUF[2].DATA.B[0] = 0x01;	
			}
			CAN_0.BUF[2].CS.B.CODE = 12;
			
			/* send coolant temp warning message */	
			if(coolant_temp >= 920){ // temperature reaches 9/10 th of the maximum
				CAN_0.BUF[3].DATA.B[0] = 0x00;
			}
			else{
				CAN_0.BUF[3].DATA.B[0] = 0x01;
			}
			CAN_0.BUF[3].CS.B.CODE = 12;
			
			int_400_arrived = 0;
		}
		
    }
}

/********************************************************************
 *                      Interrupt Functions                         *
 ********************************************************************/  

void PITCHANNEL0(void)
{		
	int_200_arrived = 1;
	
    PIT.CH[0].TFLG.B.TIF = 1;
}

void PITCHANNEL1(void)
{	
	int_400_arrived = 1;
	
    PIT.CH[1].TFLG.B.TIF = 1;
}

void CANMB0003(void)
{
/* No modifications needed here */
/* Receive interrupts are being cleared here */
    CAN_0.IFRL.B.BUF00I = 1;
    CAN_0.IFRL.B.BUF01I = 1;
    CAN_0.IFRL.B.BUF02I = 1;
    CAN_0.IFRL.B.BUF03I = 1;
}

void CANMB0407(void)
{
	//Receive data from another ECU
	if(CAN_0.IFRL.B.BUF05I) {
		switch(CAN_0.RXFIFO.ID.B.STD_ID) {
			case 0x401:
				fuel_level = (CAN_0.RXFIFO.DATA.B[1] << 8) | CAN_0.RXFIFO.DATA.B[0];
				LED0 = ~LED0;
				break;
			case 0x501:
				coolant_temp = (CAN_0.RXFIFO.DATA.B[1] << 8) | CAN_0.RXFIFO.DATA.B[0];
				LED1 = ~LED1;
				break;
			case 0x204:
				stability_control_is_alive = 1;
				LED2 = ~LED2;
				break;
		}
	}

    /* clear flags as last step here! */
    /* don't change anything below! */
    CAN_0.IFRL.B.BUF04I = 1;
    CAN_0.IFRL.B.BUF05I = 1;
    CAN_0.IFRL.B.BUF06I = 1;
    CAN_0.IFRL.B.BUF07I = 1;
}

void CANMB0811(void)
{
/* No modifications needed here */
/* transmit interrupts are being cleared here */

    CAN_0.IFRL.B.BUF08I = 1;
    CAN_0.IFRL.B.BUF09I = 1;
    CAN_0.IFRL.B.BUF10I = 1;
    CAN_0.IFRL.B.BUF11I = 1;
}

void CANMB1215(void)
{
/* No modifications needed here */
/* transmit interrupts are being cleared here */
    CAN_0.IFRL.B.BUF12I = 1;
    CAN_0.IFRL.B.BUF13I = 1;
    CAN_0.IFRL.B.BUF14I = 1;
    CAN_0.IFRL.B.BUF15I = 1;
}



/********************************************************************
 *                   Interrupt Vector Table                         *
 ********************************************************************/
#pragma interrupt Ext_Isr
#pragma section IrqSect RX address=0x040
#pragma use_section IrqSect Ext_Isr

void Ext_Isr() {
    switch(INTC.IACKR.B.INTVEC)
    {
        case 59:
            PITCHANNEL0();
            break;
        case 60:
            PITCHANNEL1();
        case 68:
            CANMB0003();
            break;
        case 69:
            CANMB0407();
            break;
        case 70:
            CANMB0811();
            break;
        case 71:
            CANMB1215();
            break;        
        default:
            break;
    }
    /* End of Interrupt Request */
    INTC.EOIR.R = 0x00000000;
}

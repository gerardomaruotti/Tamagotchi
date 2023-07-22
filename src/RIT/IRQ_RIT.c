/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_RIT.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    RIT.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include "lpc17xx.h"
#include "RIT.h"
#include "../GLCD/GLCD.h"
#include "../timer/timer.h"

extern int hh;
extern int mm;
extern int ss;
extern int animation;
extern int happiness;
extern int satiety;
extern int happinessCounter;
extern int satietyCounter;
extern int deathFlag;
extern int cuddleFlag;
extern int clickSound;
extern int eatSound;

volatile int choice = -1;

/******************************************************************************
** Function name:		RIT_IRQHandler
**
** Descriptions:		REPETITIVE INTERRUPT TIMER handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void RIT_IRQHandler (void)
{
	static int J_left = 0;
	static int J_right = 0;
	static int J_select = 0;
	
	ADC_start_conversion();//volume control
	
	if((LPC_GPIO1->FIOPIN & (1<<25)) == 0){
		disable_RIT();	//it disables RIT to avoid using the joystick when an option is selected 
		disable_timer(0);
		J_select++;
		switch(J_select) {
			case 1:
				if(happiness <=0 || satiety <=0) {//if any of the two values is 0 it means the user chose to restart the game
					deathFlag = 0;
					cuddleFlag = -1;
					hh = 0;
					mm = 0;
					ss = 0;
					animation = 0;
					happiness = 10;
					satiety = 5;
					LCD_Clear(White);
					drawGUI("Age: 00:00:00", animation, happiness, satiety);
					enable_timer(0);
					enable_RIT();
					break;
				}
				clickSound = 0;
				enable_RIT();
				break;
			default:
				enable_timer(0);
				enable_RIT();
				break;
		}
	} else{
			J_select=0;
	}
	
	if((LPC_GPIO1->FIOPIN & (1<<27)) == 0){	
		J_left++;
		switch(J_left) {
			case 1:
				if(happiness <=0 || satiety <=0) {//if any of the two values is 0 it means the user cannot choose between the two options anymore 
					break;
				}
				drawOptions(1, Red);//the selected option is highligthed
				drawOptions(2, Black);
				choice = 0;
				break;
			default:
				break;
		}
	} else{
			J_left=0;
	}
	
	if((LPC_GPIO1->FIOPIN & (1<<28)) == 0){	
		J_right++;
		switch(J_right) {
			case 1:
				if(happiness <=0 || satiety <=0) {//if any of the two values is 0 it means the user cannot choose between the two options anymore
					break;
				}
				drawOptions(1, Black);
				drawOptions(2, Red);//the selected option is highligthed
				choice = 1;
				break;
			default:
				break;
		}
	} else{
			J_right=0;
	}
	reset_RIT();
  LPC_RIT->RICTRL |= 0x1;	/* clear interrupt flag */
  return;
}
/******************************************************************************
**                            End Of File
******************************************************************************/

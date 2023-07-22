/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_timer.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    timer.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "lpc17xx.h"
#include "timer.h"
#include "../RIT/RIT.h"
#include "../GLCD/GLCD.h" 
#include "../TouchPanel/TouchPanel.h"

extern int happiness;
extern int satiety;
extern int choice;

volatile int hh = 0;	//hours to display
volatile int mm = 0;	//minutes to display
volatile int ss = 0;	//seconds to display
volatile int animation = 0;	//type of character pose to draw (0 idle, 1, animation, 2 dead)
volatile int happinessCounter = 0; //used to calculate when to decrease the value of happiness (increased by one every second inside timer0 until it reaches 5)
volatile int satietyCounter = 0;	//used to calculate when to decrease the value of happiness (increased by one every second inside timer0 until it reaches 5)
volatile int deathFlag = 0;	//used to determine if the character is dead and the gameover interface is displayed on screen (it avoids the time and other variables to change)
volatile int cuddleFlag = -1;//used to determine if the screen has been touched and if the cuddle animation is playing
volatile int volume = 1;//variable used to control the volume
volatile int clickSound = -1;
volatile int eatSound = -1;
volatile int eatNote = 1;
volatile int deadSound = -1;
volatile int deadNote = 3;
volatile int cuddleSound = -1;
volatile int cuddleNote = 5;

NOTE sound_effects[] = 
{
	//click
	{a3, time_semicroma},
	//eating
	{c3b, time_semicroma},
	{d4, time_croma},
	//death
	{g3, time_croma},
	{f3, time_semicroma*2},
	//cuddle
	{a2b, time_semicroma},
	{d4, time_croma},
};

uint16_t SinTable[45] =
{
    410, 467, 523, 576, 627, 673, 714, 749, 778,
    799, 813, 819, 817, 807, 789, 764, 732, 694, 
    650, 602, 550, 495, 438, 381, 324, 270, 217,
    169, 125, 87 , 55 , 30 , 12 , 2  , 0  , 6  ,   
    20 , 41 , 70 , 105, 146, 193, 243, 297, 353
};

/******************************************************************************
** Function name:		select
**
** Descriptions:		it selects the correct option after the user selects between meal and snack
**
** parameters:			choice: the user choice after selecting the option
** Returned value:		None
**
******************************************************************************/
void select(int choice) {
	if(choice == 0) {
		satiety++;
		if(satiety > 5)
			satiety = 5;
		satietyCounter = 0;
	} else if(choice == 1) {
		happiness+=2;
		if(happiness > 10)
			happiness = 10;
		happinessCounter = 0;
	}
	return;
}

/******************************************************************************
** Function name:		updateTime
**
** Descriptions:		it formats the time to correctly display hours, minutes and seconds on screen
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void updateTime(void) {
	if(ss < 60) {
		ss++;
	} else {
		ss = 0;
		if(mm < 60) {
			mm++;
		} else {
			ss = 0;
			mm = 0;
			hh++;
		}
	}
}

/******************************************************************************
** Function name:		Timer0_IRQHandler
**
** Descriptions:		Timer/happinessCounter 0 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TIMER0_IRQHandler (void)
{
	
	char time[32];
		
	if(deathFlag) {					// if gameover interface is displayed the other variables don't need to be updated
		LPC_TIM0->IR = 1;			/* clear interrupt flag */
		return;
	}
	
	happinessCounter++;			//increases by one every second until it reaches 5
	satietyCounter++;				//increases by one every second until it reaches 5
	
	if(happinessCounter >= 5) {//happiness is decreased by one value and the counter is set to 0
		happiness--;
		happinessCounter = 0;
	}
	
	if(satietyCounter >= 5) {//satiety is decreased and the counter is set to 0
		satiety--;
		satietyCounter = 0;
	}
	
	if(happiness <= 0 || satiety <= 0) {//check if any of the two variables are 0 and in that case the gameover procedure starts
		deathFlag = 1;
		deadSound = 0;
		sprintf(time, "Age: %02d:%02d:%02d", hh, mm, ss);	//formats the hours, minutes and seconds to be displayed on screen
		periodicRefresh(time, animation, happiness, satiety);//it triggers the gameover interface to be displayed
		LPC_TIM0->IR = 1;			/* clear interrupt flag */
		return;
	}
	
	if(cuddleFlag > -1 &&  cuddleFlag != 2) {
		cuddleFlag++;
		updateTime();
		sprintf(time, "Age: %02d:%02d:%02d", hh, mm, ss);	//formats the hours, minutes and seconds to be displayed on screen
		GUI_Text(60, 10, (uint8_t *) time, Black, White);		
		periodicRefresh(time, 3, happiness, satiety);//it updates the statistics on screen (happiness, satiety, age and character pose)
		LPC_TIM0->IR = 1;			/* clear interrupt flag */
		return;
	} else if(cuddleFlag == 2){
		happiness+=2;
		if(happiness > 10)
			happiness = 10;
		happinessCounter = 0;
		cuddleFlag = -1;
		enable_RIT();
	}
	disable_timer(1);
	updateTime();
	sprintf(time, "Age: %02d:%02d:%02d", hh, mm, ss);	//formats the hours, minutes and seconds to be displayed on screen
	GUI_Text(60, 10, (uint8_t *) time, Black, White);
	animation = !animation;	//to change between the two poses every second
	periodicRefresh(time, animation, happiness, satiety);//it updates the statistics on screen (happiness, satiety, age and character pose)
	enable_timer(1);
  LPC_TIM0->IR = 1;			/* clear interrupt flag */
  return;
}

/******************************************************************************
** Function name:		Timer1_IRQHandler
**
** Descriptions:		Timer/happinessCounter 1 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TIMER1_IRQHandler (void)
{
	
	if(getDisplayPoint(&display, Read_Ads7846(), &matrix )){
		if((display.y >= 175 && display.y <= 225) && (display.x >= 95 && display.x <= 145)){
			if(!deathFlag && eatSound < 0) {
				cuddleFlag = 0;
				cuddleSound = 0;
				drawCharacter(95, 175, 3);
				disable_RIT();
			}
		}
	} else{
		//do nothing if touch returns values out of bounds
	}
	
	if(clickSound == 0) {
		if(!isNotePlaying()) {
			playNote(sound_effects[0]);
			clickSound++;
		}
	} else if(clickSound >= 1) {
		eatAnimation(choice);
		clickSound = -1;
		eatSound = 0;
	}
	
	if(eatSound != -1 && eatSound < 2) {
		if(!isNotePlaying()) {
			playNote(sound_effects[eatNote]);
			eatSound++;
			eatNote++;
		}
	} else if(eatSound >= 2) {
			eatAnimation(choice+2);
			select(choice);
			choice = -1;
			drawOptions(1, Black);//once the animation is played, the options are set back to thei normal state (instead of the select state they were before)
			drawOptions(2, Black);
			eatSound = -1;
			eatNote = 1;
			enable_timer(0);
	}
	
	if(deadSound != -1 && deadSound < 2) {
		disable_timer(0);
		if(!isNotePlaying()) {
			playNote(sound_effects[deadNote]);
			deadSound++;
			deadNote++;
		}
	} else if(deadSound >= 2) {
		deadSound = -1;
		deadNote = 3;
		enable_timer(0);
	}
	
	if(cuddleSound != -1 && cuddleSound < 2) {
		if(!isNotePlaying()) {
			playNote(sound_effects[cuddleNote]);
			cuddleSound++;
			cuddleNote++;
		}
	} else if(cuddleSound >= 2) {
		cuddleSound = -1;
		cuddleNote = 5;
	}
	
  LPC_TIM1->IR = 1;			/* clear interrupt flag */
  return;
}

/******************************************************************************
** Function name:		Timer2_IRQHandler
**
** Descriptions:		Timer/Counter 2 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TIMER2_IRQHandler (void)
{
	static int sineticks=0;
	/* DAC management */
	static int currentValue;
	currentValue = SinTable[sineticks];
	currentValue -= 410;
	currentValue /= 1;
	currentValue += 410;
	currentValue *= volume;
	if(currentValue > 1024)
		currentValue = 1024;
	LPC_DAC->DACR = currentValue <<6;
	sineticks++;
	if(sineticks==45) sineticks=0;
	
	LPC_TIM2->IR =  1 ;			/* clear interrupt flag */	
	return;
}

/******************************************************************************
** Function name:		Timer3_IRQHandler
**
** Descriptions:		Timer/Counter 3 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TIMER3_IRQHandler (void)
{
	disable_timer(2);
	LPC_TIM3->IR =  1 ;			/* clear interrupt flag */	
	return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/

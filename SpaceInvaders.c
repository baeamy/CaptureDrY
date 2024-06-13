// SpaceInvaders.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the ECE319K Lab 10

// Last Modified: 1/2/2023 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php

// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// buttons connected to PE0-PE3
// 32*R resistor DAC bit 0 on PB0 (least significant bit)
// 16*R resistor DAC bit 1 on PB1
// 8*R resistor DAC bit 2 on PB2 
// 4*R resistor DAC bit 3 on PB3
// 2*R resistor DAC bit 4 on PB4
// 1*R resistor DAC bit 5 on PB5 (most significant bit)
// LED on PD1
// LED on PD0


#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "Random.h"
#include "TExaS.h"
#include "../inc/ADC.h"
#include "Images.h"
#include "../inc/wave.h"
#include "Timer1.h"
#include "Array.h"
#include "Timer0.h"

#define WALL_COLOR 0xF369  

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

struct sprite{
	uint8_t x, y, w, h;
};

typedef struct sprite sprite_t;

sprite_t yerr = {55, 157, 18, 18};
sprite_t prev = {55, 157, 18, 18};
sprite_t sophia_sprite = {79, 27, 18, 18};
sprite_t malav_sprite = {2, 27, 18, 18};
sprite_t wheel_sprite1  = {105, 23, 18, 18}; // top right
sprite_t wheel_sprite2 = {29, 52, 18, 18}; // middle left
sprite_t handle_sprite = {82, 156, 18, 18}; // bottom right

void Button_Init(){
	volatile int delay;
	SYSCTL_RCGCGPIO_R |= 0x10; 
	delay = SYSCTL_RCGCGPIO_R;
	delay = SYSCTL_RCGCGPIO_R;
	delay = SYSCTL_RCGCGPIO_R;
	delay = SYSCTL_RCGCGPIO_R;
	GPIO_PORTE_DIR_R &= ~ 0x0F;
	GPIO_PORTE_DEN_R |= 0x0F;
}

uint8_t direction(void){
	uint8_t button_in_direction, direction_number;
	button_in_direction = (GPIO_PORTE_DATA_R & 0x0F);
	// up
	if (button_in_direction == 0x01){
		direction_number = 1; 
	}
	// right
	else if (button_in_direction == 0x02){
		direction_number = 2;
	}
	// down
	else if (button_in_direction == 0x04){
		direction_number = 3;
	}
	// left
	else if (button_in_direction == 0x08){
		direction_number = 4;
	}
	// stationary
	else {
		direction_number = 0;
	}
	return direction_number;
}

uint32_t check_collision (uint8_t direction_number, uint32_t *arr){
	uint32_t save_x1, save_y1, save_x2, save_y2, valid, index1, index2;
	uint32_t drY_can_move_up, drY_can_move_down, drY_can_move_left, drY_can_move_right;	
	// move up
	save_x1 = yerr.x;
	save_y1 = (yerr.y - 1) - 17 - 1;
	save_x2 = yerr.x + 17;
	save_y2 = (yerr.y - 1) - 17 - 1;
	index1 = (159 - save_y1)*128 + save_x1;
	index2 = (159 - save_y2)*128 + save_x2;
	if (track[index1] == WALL_COLOR || track[index2] == WALL_COLOR){
		drY_can_move_up = 0;
	} else {
		drY_can_move_up = 1;	
	}
	// move right	
	save_x1 = yerr.x + 17 + 1;
	save_y1 = (yerr.y + 1); // change this to +1 if the -1 doesn't work
	save_x2 = yerr.x + 17 + 1;
	save_y2 = (yerr.y + 1) - 17;
	index1 = (159 - save_y1)*128 + save_x1;
	index2 = (159 - save_y2)*128 + save_x2;
	if (track[index1] == WALL_COLOR || track[index2] == WALL_COLOR){
		drY_can_move_right = 0;
	} else {
		drY_can_move_right = 1;
	}
	// move down
	save_x1 = yerr.x;
	save_y1 = (yerr.y + 1) + 1;
	save_x2 = yerr.x + 17;
	save_y2 = (yerr.y + 1) + 1;
	index1 = (159 - save_y1)*128 + save_x1;
	index2 = (159 - save_y2)*128 + save_x2;
	if (track[index1] == WALL_COLOR || track[index2] == WALL_COLOR){
		drY_can_move_down = 0;
	} else {
		drY_can_move_down = 1;
	}
	// move left
	save_x1 = yerr.x - 1;
	save_y1 = (yerr.y + 1);
	save_x2 = yerr.x - 1;
	save_y2 = (yerr.y + 1) - 17;
	index1 = (159 - save_y1)*128 + save_x1;
	index2 = (159 - save_y2)*128 + save_x2;
	if (track[index1] == WALL_COLOR || track[index2] == WALL_COLOR){
		drY_can_move_left = 0;
	} else {
		drY_can_move_left = 1;
	}
	// find if chosen direction is valid
	if (direction_number == 1){
		valid = drY_can_move_up;
	}
	else if (direction_number == 2){
		valid = drY_can_move_right;
	}
	else if (direction_number == 3){
		valid = drY_can_move_down;
	}
	else if (direction_number == 4){
		valid = drY_can_move_left;
	}
	return valid;
}

void applymove(int8_t dir){
		// up
		if (dir == 1){
			yerr.y--;
		}
		// right
		else if (dir == 2){
			yerr.x++;
		}
		// down
		else if (dir == 3){ 
			yerr.y++;
		}
		// left
		else if (dir == 4){ 	
			yerr.x--;
		}
		// stationary
		else if (dir == 0){
			yerr.x = yerr.x;
			yerr.y = yerr.y;
		} 
}

uint8_t collide_wheel1_result = 0, collide_wheel2_result = 0, collide_handle_result = 0;
void check_collision_wheel_handle(void){
	// check for wheel1 (top right)
	if (collide_wheel1_result != 1){
		if (yerr.y < wheel_sprite1.y - 17 || yerr.y - 17 > wheel_sprite1.y || yerr.x > wheel_sprite1.x + 17 || yerr.x + 17 < wheel_sprite1.x){
			collide_wheel1_result = 0;
		} else {
			Wave_Shoot();
			collide_wheel1_result = 1;
			ST7735_DrawBitmap(wheel_sprite1.x, wheel_sprite1.y, black, wheel_sprite1.w, wheel_sprite1.h);
		}
	}
	// check for wheel2 (middle left)
	if (collide_wheel2_result != 1){
		if (yerr.y < wheel_sprite2.y - 17 || yerr.y - 17 > wheel_sprite2.y || yerr.x > wheel_sprite2.x + 17 || yerr.x + 17 < wheel_sprite2.x){
			collide_wheel2_result = 0;
		} else {
			Wave_Shoot();
			collide_wheel2_result = 1;
			ST7735_DrawBitmap(wheel_sprite2.x, wheel_sprite2.y, black, wheel_sprite2.w, wheel_sprite2.h);
		}
	}
	// check for handle
	if (collide_handle_result != 1){
		if (yerr.y < handle_sprite.y - 17 || yerr.y - 17 > handle_sprite.y || yerr.x > handle_sprite.x + 17 || yerr.x + 17 < handle_sprite.x){
			collide_handle_result = 0;
		} else {
			Wave_Shoot();
			collide_handle_result = 1;
			ST7735_DrawBitmap(handle_sprite.x, handle_sprite.y, black, handle_sprite.w, handle_sprite.h);
		}
	}
}

void update_character(void){
	uint32_t arr[2];
	// get the current direction
	uint8_t dir = direction(); 
	
	// check for collision
	if (check_collision (dir, arr) == 1){
	// update the character position
		applymove(dir); 
	}
}	

uint8_t collide_malav_result = 0, collide_sophia_result = 0;
void check_TA_DrY_collide (){
	if (yerr.y < malav_sprite.y - 17 || yerr.y - 17 > malav_sprite.y || yerr.x > malav_sprite.x + 17 || yerr.x + 17 < malav_sprite.x){
		collide_malav_result = 0;
	} else {
		collide_malav_result = 1;
	}
	if (yerr.y < sophia_sprite.y - 17 || yerr.y - 17 > sophia_sprite.y || yerr.x > sophia_sprite.x + 17 || yerr.x + 17 < sophia_sprite.x){
		collide_sophia_result = 0;
	} else {
		collide_sophia_result = 1;
	}
}

uint8_t Timer1_flag;
void Timer1A_Handler(){
		update_character();
	// move malav's sprite
	if (malav_sprite.x == 2 && malav_sprite.y != 133){
		malav_sprite.y++;
	}
	else if (malav_sprite.y == 133 && malav_sprite.x != 52){
		malav_sprite.x++;
	}
	else if (malav_sprite.x == 52 && malav_sprite.y != 27){
		malav_sprite.y--;
	}
	else if (malav_sprite.y == 27 && malav_sprite.x != 2){
		malav_sprite.x--;
	}
	// move sophia's sprite
	if (sophia_sprite.x == 79 && sophia_sprite.y != 150){
		sophia_sprite.y++;
	}
	else if (sophia_sprite.y == 150 && sophia_sprite.x != 105){
		sophia_sprite.x++;
	}
	else if (sophia_sprite.x == 105 && sophia_sprite.y != 27){
		sophia_sprite.y--;
	}
	else if (sophia_sprite.y == 27 && sophia_sprite.x != 79){
		sophia_sprite.x--;
	}
	check_TA_DrY_collide();
	check_collision_wheel_handle();
	Timer1_flag = 1;
	TIMER1_ICR_R = TIMER_ICR_TATOCINT; // acknowledging timer
}

void delay(void){
	uint32_t i;
	for (i = 0; i < 10000000; i++){
		for (i = 0; i < 10000000; i++){
		}
	}
}

int main(void){
	uint32_t ADC_value, i;
	DisableInterrupts();
  TExaS_Init(SCOPE);       // Bus clock is 80 MHz 
  Output_Init();
	ADC_Init();
	Wave_Init();
	Button_Init();
	Timer1_Init(5000000, 0); 
	ST7735_InitR(INITR_BLACKTAB);
	EnableInterrupts();
	// pick your language
	ST7735_DrawBitmap(0, 159, blacktrack, 128, 160);
	ST7735_OutString ("Use the slidepot to");
	ST7735_OutString ("\n");
	ST7735_OutString ("pick a language:");
	ST7735_OutString ("\n");
	ST7735_OutString ("English - Left");
	ST7735_OutString ("\n");
	ST7735_OutString ("Spanish - Right");
	ST7735_OutString ("\n");
	delay();
	ADC_value = ADC_In();
	if (ADC_value >= 2048){
		ST7735_OutString ("Here we go!");
		ST7735_OutString ("\n");
		delay();
	} else {
		ST7735_OutString ("Vamonos!");
		ST7735_OutString ("\n");
		delay();
	}
	ST7735_DrawBitmap(0, 159, track, 128, 160);
	ST7735_DrawBitmap(105, 23, wheel, 18, 18);
	ST7735_DrawBitmap(29, 52, wheel, 18, 18);
	ST7735_DrawBitmap(82, 156, handle, 18, 18);
	while (1){
		if (collide_handle_result != 1 || collide_wheel1_result != 1 || collide_wheel2_result != 1){
			if (Timer1_flag == 1 && collide_malav_result == 0 && collide_sophia_result == 0){
			// updating malav's sprite in main
				ST7735_DrawBitmap(malav_sprite.x, malav_sprite.y, malav, malav_sprite.w, malav_sprite.h);
			// updating sophia's sprite in main
				ST7735_DrawBitmap(sophia_sprite.x, sophia_sprite.y, sophia, sophia_sprite.w, sophia_sprite.h);
			// updating yerr's sprite in main
				ST7735_DrawBitmap(yerr.x, yerr.y, DrY, yerr.w, yerr.h);	
			// redrawing the wheel/handle sprite
				if (collide_wheel1_result == 0){
					ST7735_DrawBitmap(wheel_sprite1.x, wheel_sprite1.y, wheel, wheel_sprite1.w, wheel_sprite1.h);
				}
				if (collide_wheel2_result == 0){
				ST7735_DrawBitmap(wheel_sprite2.x, wheel_sprite2.y, wheel, wheel_sprite2.w, wheel_sprite2.h);
				}
				if (collide_handle_result == 0){
					ST7735_DrawBitmap(handle_sprite.x, handle_sprite.y, handle, handle_sprite.w, handle_sprite.h);
				}
				Timer1_flag = 0;
				// game end path
			} else {
				// Dr. Y collided with malav/sophia
				if (collide_malav_result == 1 || collide_sophia_result == 1){
					Wave_Killed();
					ST7735_DrawBitmap(0, 159, blacktrack, 128, 160);
					uint8_t count = collide_handle_result + collide_wheel1_result + collide_wheel2_result;
					ST7735_SetCursor(0,1);
					if (ADC_value >= 2048){
						ST7735_OutString ("Dr. Y has been");
						ST7735_OutString ("\n");
						ST7735_OutString ("captured.");
						ST7735_OutString ("\n");
						if (count == 1){
							ST7735_OutString ("count = 1");
						}
						if (count == 2){
							ST7735_OutString ("count = 2");
						}
						ST7735_OutString ("\n");
						ST7735_OutString ("Try again? Reset!");
					while(1){}
					} else {
						ST7735_OutString ("Dr. Y ha sido");
						ST7735_OutString ("\n");
						ST7735_OutString ("capturado.");
						ST7735_OutString ("\n");
						if (count == 1){
							ST7735_OutString ("contar = 1");
						}
						if (count == 2){
							ST7735_OutString ("contar = 2");
						}
						ST7735_OutString ("\n");
						ST7735_OutString ("Otra vez? Reset!");
					while(1){}
				}
			}
				// Dr. Y collected all bike components
				else if ((collide_handle_result == 1) && (collide_wheel1_result == 1) && (collide_wheel2_result == 1)){
					ST7735_DrawBitmap(0, 159, blacktrack, 128, 160);
					ST7735_SetCursor(1,1);
				if (ADC_value >= 2048){
					ST7735_OutString ("Dr. Y is free!");
					ST7735_OutString ("\n");
					ST7735_OutString ("We love you Dr. Y <3");
					ST7735_OutString ("\n");
					ST7735_OutString ("count = 3");
					while(1){}
				} else {
					ST7735_OutString ("Dr. Y es gratis!");
					ST7735_OutString ("\n");
					ST7735_OutString ("Te amo Dr. Y <3");
					ST7735_OutString ("\n");
					ST7735_OutString ("contar = 3");
					while(1){}
					}
				}
			}
		}
	}
}
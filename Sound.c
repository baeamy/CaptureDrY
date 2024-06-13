#include <stdint.h>
#include "Sound.h"
#include "DAC.h"
#include "../inc/.tm4c123gh6pm.h"
#include "Timer0.h"

uint32_t Length;
const uint8_t *SoundPt;


void SoundTask(void){
	if (Length != 0){
		DAC_Out(*SoundPt/4); // 8 bit sound, 6-bit DAC
		SoundPt++;
		Length--;
	} else {
		NVIC_DIS0_R = 1<<19; 
	}
}
	
void Sound_Init(void){
	Length = 0; // counter. // 0 means no sound
	DAC_Init;
	Timer0_Init(&SoundTask, 80000000/11000); // 11 kHz
};

void Sound_Play(const uint8_t *pt, uint32_t count){
	Length = count;
	SoundPt = pt;
	NVIC_EN0_R = 1<<19;
};

void Sound_Shoot(void){
	Sound_Play(shoot, 4080);
};

void Sound_Killed(void){
	Sound_Play(killed, 4080);
};
	
void Sound_Explosion(void){
	Sound_Play(explosion, 4080);
};
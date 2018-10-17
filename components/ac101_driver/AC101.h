/*
 * AC101.h
 *
 *  Created on: 2018年1月9日
 *      Author: ai-thinker
 */

#ifndef MAIN_AC101_H_
#define MAIN_AC101_H_

#include "stdint.h"
#define AC101_ADDR	0X1a   //0011010
#define PA_EN_PIN   21
#define PA_EN_PIN_SEL      (1ULL<<PA_EN_PIN)

void I2C_init();
int AC101_init();
void mic_init(void);
uint16_t AC101_read_Reg(uint8_t reg) ;
esp_err_t AC101_Write_Reg(uint8_t reg, uint16_t val);


void init_gpio_PA(int en);
void enable_PA(int en);
void codec_mute(int en);


#endif /* MAIN_AC101_H_ */

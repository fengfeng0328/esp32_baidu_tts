/*
 * AC101.c
 *
 *  Created on: 2018年1月9日
 *      Author: ai-thinker
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "DriverUtil.h"
#include "driver/i2s.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "AC101.h"
#define I2C_MASTER_SCL_IO    32    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    33    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency */
#define I2S_NUM         (0)
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
static char *AC101_TAG = "AC101";
#define I2C_CHECK(a, str)  if((a)) {                                             \
        ESP_LOGE(AC101_TAG,"%s:%d (%s):%d", __FILE__, __LINE__, __FUNCTION__, str);      \
		}
		
#define SPKOUT_CTRL			0x58

void AC101_i2c_master_init() {
	int i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = (gpio_num_t) I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = (gpio_num_t) I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	i2c_param_config((i2c_port_t) i2c_master_port, &conf);
	i2c_driver_install((i2c_port_t) i2c_master_port, conf.mode,
	I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

uint16_t AC101_read_Reg(uint8_t reg) {
	uint16_t val = 0;
	uint8_t data_rd[2];
	i2c_example_master_read_slave(AC101_ADDR,reg, data_rd, 2);
	ESP_LOGI(AC101_TAG, "reg=%x,data_rd[0]=%x,data_rd[1]=%x\r\n",reg, data_rd[0],
			data_rd[1]);
	val=(data_rd[0]<<8)+data_rd[1];
	return val;
}
esp_err_t AC101_Write_Reg(uint8_t reg, uint16_t val)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	uint8_t send_buff[4];
	send_buff[0] = (AC101_ADDR << 1);
	send_buff[1] = reg;
	send_buff[2] = (val>>8) & 0xff;
	send_buff[3] = val & 0xff;
    i2c_master_start(cmd);
    i2c_master_write(cmd, send_buff, 4, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
esp_err_t ac101_set_spk_volume(uint8_t volume)
{
	uint16_t res;
	esp_err_t ret;
	volume = volume/2;
	res = AC101_read_Reg(SPKOUT_CTRL);
	res &= (~0x1f);
	volume &= 0x1f;
	res |= volume;
	ret = AC101_Write_Reg(SPKOUT_CTRL,res);
	return ret;
}


int AC101_init() {
	AC101_i2c_master_init();
	esp_err_t Res;
	Res = AC101_Write_Reg(0x0, 0x123);					//soft reset AC101
	if (ESP_OK != Res) {
		printf("CODEC ERR\r\n");
		return 1;							    		//reset failed, WM8978 exception
	} else {        
		printf("CODEC OK\r\n");
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	I2C_CHECK(AC101_Write_Reg(0x58, 0xe880),1);

	//Enable the PLL from 256*44.1KHz MCLK source
	I2C_CHECK(AC101_Write_Reg(0x01, 0x014f), 2);
	I2C_CHECK(AC101_Write_Reg(0x02, 0x8120), 3);

	//Clocking system
	I2C_CHECK(AC101_Write_Reg(0x03, 0x8b08), 4);
	I2C_CHECK(AC101_Write_Reg(0x04, 0x800c), 5);
	I2C_CHECK(AC101_Write_Reg(0x05, 0x800c), 6);
	I2C_CHECK(AC101_Write_Reg(0x06, 0x3000), 7);

	//AIF config
	I2C_CHECK(AC101_Write_Reg(0x10, 0x8850), 8);
	I2C_CHECK(AC101_Write_Reg(0x11, 0xc000), 9);	//
	I2C_CHECK(AC101_Write_Reg(0x12, 0xc000), 9);
	I2C_CHECK(AC101_Write_Reg(0x13, 0x2200), 9);	//

	I2C_CHECK(AC101_Write_Reg(0x52, 0xc444), 10);
	I2C_CHECK(AC101_Write_Reg(0x51, 0x2000), 11);
	//I2C_CHECK(AC101_Write_Reg(0x51, 0x3060), 11);
	I2C_CHECK(AC101_Write_Reg(0x40, 0x8000), 12);
	I2C_CHECK(AC101_Write_Reg(0x50, 0xbbc0), 14);

	//Path Configuration
	I2C_CHECK(AC101_Write_Reg(0x4c, 0xcc00), 10);
	I2C_CHECK(AC101_Write_Reg(0x48, 0x8000), 11);
	I2C_CHECK(AC101_Write_Reg(0x54, 0x0081), 12);
	I2C_CHECK(AC101_Write_Reg(0x53, 0xf080), 13);

	//* Enable Headphoe output   注意使用耳机时，最后开以下寄存器
	I2C_CHECK(AC101_Write_Reg(0x53, 0xff80), 14);
	I2C_CHECK(AC101_Write_Reg(0x56, 0xc3c1), 15);
	I2C_CHECK(AC101_Write_Reg(0x56, 0xcb00), 16);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	I2C_CHECK(AC101_Write_Reg(0x56, 0xfbc0), 17);

	//* Enable Speaker output
	I2C_CHECK(AC101_Write_Reg(0x58, 0xeabd), 18);
//	I2C_CHECK(AC101_Write_Reg(0x4a, 0x0040), 18);
	ac101_set_spk_volume(20);
	init_gpio_PA(1);

	return 0;
}

int AC101_init_16k() {
	AC101_i2c_master_init();
	esp_err_t Res;
	Res = AC101_Write_Reg(0x0, 0x123);					//soft reset AC101
	if (ESP_OK != Res) {
		printf("CODEC ERR\r\n");
		return 1;							    		//reset failed, WM8978 exception
	} else {
		printf("CODEC OK\r\n");
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	I2C_CHECK(AC101_Write_Reg(0x58, 0xe880),1);

	//Enable the PLL from 256*44.1KHz MCLK source
	I2C_CHECK(AC101_Write_Reg(0x01, 0x014f), 2);
	I2C_CHECK(AC101_Write_Reg(0x02, 0x8120), 3);

	//Clocking system
	I2C_CHECK(AC101_Write_Reg(0x03, 0x8b08), 4);
	I2C_CHECK(AC101_Write_Reg(0x04, 0x800c), 5);
	I2C_CHECK(AC101_Write_Reg(0x05, 0x800c), 6);
	I2C_CHECK(AC101_Write_Reg(0x06, 0x3000), 7);

	//AIF config
	I2C_CHECK(AC101_Write_Reg(0x10, 0x8850), 8);
	I2C_CHECK(AC101_Write_Reg(0x11, 0xc000), 9);	//
	I2C_CHECK(AC101_Write_Reg(0x12, 0xc000), 9);
	I2C_CHECK(AC101_Write_Reg(0x13, 0x2200), 9);	//

	I2C_CHECK(AC101_Write_Reg(0x52, 0xccc4), 10);
	I2C_CHECK(AC101_Write_Reg(0x51, 0x2020), 11);
	//I2C_CHECK(AC101_Write_Reg(0x51, 0x3060), 11);
	I2C_CHECK(AC101_Write_Reg(0x40, 0x8000), 12);
	I2C_CHECK(AC101_Write_Reg(0x50, 0xbbc3), 14);

	//Path Configuration
	I2C_CHECK(AC101_Write_Reg(0x4c, 0xcc00), 10);
	I2C_CHECK(AC101_Write_Reg(0x48, 0x8000), 11);
	I2C_CHECK(AC101_Write_Reg(0x54, 0x0081), 12);
	I2C_CHECK(AC101_Write_Reg(0x53, 0xf080), 13);

	//* Enable Headphoe output   注意使用耳机时，最后开以下寄存器
	I2C_CHECK(AC101_Write_Reg(0x53, 0xff80), 14);
	I2C_CHECK(AC101_Write_Reg(0x56, 0xc3c1), 15);
	I2C_CHECK(AC101_Write_Reg(0x56, 0xcb00), 16);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	I2C_CHECK(AC101_Write_Reg(0x56, 0xfbc0), 17);

	//* Enable Speaker output
	I2C_CHECK(AC101_Write_Reg(0x58, 0xeabd), 18);
//	I2C_CHECK(AC101_Write_Reg(0x4a, 0x0040), 18);
	init_gpio_PA(1);
	return 0;
}

void mic_init(void)
{
	AC101_i2c_master_init();
	esp_err_t Res;
	Res = AC101_Write_Reg(0x0, 0x123);					//soft reset AC101
	if (ESP_OK != Res) {
		printf("CODEC_ERR\r\n");
		return;							    		//reset failed, WM8978 exception
	} else {
		printf("CODEC_OK\r\n");
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	I2C_CHECK(AC101_Write_Reg(0x58, 0xe880),1);
	//Enable the PLL from 256*44.1KHz MCLK source
	I2C_CHECK(AC101_Write_Reg(0x01, 0x014f), 2);
	I2C_CHECK(AC101_Write_Reg(0x02, 0x8120), 3);
	//Clocking system
	I2C_CHECK(AC101_Write_Reg(0x03, 0x8b08), 4);
	I2C_CHECK(AC101_Write_Reg(0x04, 0x8008), 5);
	I2C_CHECK(AC101_Write_Reg(0x05, 0x8008), 6);
	I2C_CHECK(AC101_Write_Reg(0x06, 0x3000), 7);
	//AIF config
	I2C_CHECK(AC101_Write_Reg(0x10, 0x8850), 8);
	I2C_CHECK(AC101_Write_Reg(0x11, 0xc000), 9);
	I2C_CHECK(AC101_Write_Reg(0x13, 0x2200), 9);
	//Path Configuration
	I2C_CHECK(AC101_Write_Reg(0x52, 0xccc4), 10);
	I2C_CHECK(AC101_Write_Reg(0x51, 0x2020), 11);		//MIC1 2

	I2C_CHECK(AC101_Write_Reg(0x40, 0x8000), 12);
	I2C_CHECK(AC101_Write_Reg(0x50, 0xbbc3), 14);		
}

void init_gpio_PA(int en)
{
	gpio_config_t io_conf;  
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = PA_EN_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
	//gpio_set_level(PA_EN_PIN, 0);
	enable_PA(en);
}
void enable_PA(int en)
{
	if(en > 0)
	{
		gpio_set_level(PA_EN_PIN, 1);
	}else
	{
		gpio_set_level(PA_EN_PIN, 0);
	}
}

void codec_mute(int en)
{
	enable_PA(en);
	uint16_t reg_val = AC101_read_Reg(0x56);
	
	if(en >0)
	{
		enable_PA(0);
		reg_val &= (~(3<<12));
	}
	else
	{
		enable_PA(1);
		reg_val |= (3 << 12);
	}
	AC101_Write_Reg(0x56,reg_val);
}






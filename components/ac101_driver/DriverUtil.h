/*
 * DriverUtil.h
 *
 *  Created on: 2017.1.23
 *      Author: Jack
 */


#ifndef __DRIVER_UTIL_H
#define __DRIVER_UTIL_H

#include "stdint.h"
#include "esp_log.h"
uint8_t IIC_Write_One_Byte(uint8_t DevAddr,uint8_t RegAddr,uint8_t Data);
uint8_t IIC_Write_two_Bytes(uint8_t DevAddr, uint8_t RegAddr, uint16_t Data);
uint16_t IIC_read_two_Bytes(uint8_t DevAddr, uint8_t RegAddr);
esp_err_t i2c_example_master_read_slave(uint8_t DevAddr, uint8_t reg,uint8_t* data_rd, size_t size);
#endif

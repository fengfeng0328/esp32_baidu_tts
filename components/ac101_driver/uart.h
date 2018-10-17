/*
 * uart.h
 *
 *  Created on: 2017年12月9日
 *      Author: ai-thinker
 */

#ifndef MAIN_UART_H_
#define MAIN_UART_H_
#include <stdint.h>
#include "stdbool.h"
typedef struct
{
  char *at_cmdName;
  int8_t at_cmdLen;
  void (*at_testCmd)(uint8_t id);
  void (*at_queryCmd)(uint8_t id);
  void (*at_setupCmd)(uint8_t id, char *pPara);
  void (*at_exeCmd)(uint8_t id);
}at_funcationType;

void echo_task();                         //接收串口数据
void at_recvTask(char *str);              //处理接受到的串口数据，并相应对应的at命令
void at_OTA(uint8_t id);                  //工作分区切换
void ota_adress(uint8_t id, char *pPara); //固件升级
void at_connect(uint8_t id, char *pPara);//连接到网络

#endif /* MAIN_UART_H_ */

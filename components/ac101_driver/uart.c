#include "uart.h"
#include "esp_err.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"
#include "ctype.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
//#include "apps/dhcpserver_options.h"
#include "lwip/dns.h"
#include "lwip/netif.h"

#define ECHO_TEST_TXD  (1)
#define ECHO_TEST_RXD  (3)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (128)
int uart_num = UART_NUM_0;
static const char *TAG = "uart";

void ota_adress(uint8_t id, char *pPara);
#define MAKE_AT_FUNCTION(NAME, TEST_FUNCTION, QUERY_FUNCTION, SETUP_FUNCTION, EXEC_FUNCTION) \
    { \
        (NAME), \
        sizeof(NAME) - 1, \
        (TEST_FUNCTION),\
        (QUERY_FUNCTION), \
        (SETUP_FUNCTION), \
        (EXEC_FUNCTION), \
    }

static at_funcationType at_custom_cmd[] = {
		MAKE_AT_FUNCTION("+OTA", at_OTA, NULL, NULL, NULL),
		};

#define MAKE_AT_COUNT (sizeof(at_custom_cmd) / sizeof(at_funcationType))

void print_err() {
	printf("error\r\n");
}
//检测是否是字符串并且去除引号
bool get_str(char *str) {
	if (str == NULL)
		return false;
	if ((str[0] == '"') && (str[strlen(str) - 1] = '"')) {

		for (int i = 0; i < strlen(str) - 1; i++) {
			str[i] = str[i + 1];
		}
		str[strlen(str) - 2] = 0;
		printf("string=%s\r\n", str);
		return true;
	} else {
		return false;
	}

}

/*从字符串的中间截取n个字符*/
char mid(char *dst, char *src, int n, int m) /*n为长度，m为位置*/
{
	char *p = src;
	char *q = dst;
	int len = strlen(src);
	if (n > len)
		n = len - m; /*从第m个到最后*/
	if (m < 0)
		m = 0; /*从第一个开始*/
	if (m > len)
		return NULL;
	p += m;
	while (n--)
		*(q++) = *(p++);
	*(q++) = '\0'; /*有必要吗？很有必要*/
	return dst;
}


void at_OTA(uint8_t id)
{
	printf("PARTION_SET_OK\r\n");
}

void echo_task() {
	/* Configure parameters of an UART driver,
	 * communication pins and install the driver */
	uart_config_t uart_config = { .baud_rate = 115200, .data_bits =
			UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits =
			UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE };
	uart_param_config(uart_num, &uart_config);
	uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS,
	ECHO_TEST_CTS);
	uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

	uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
	while (1) {
		// Read data from the UART
		int len = uart_read_bytes(uart_num, data, BUF_SIZE,
				20 / portTICK_RATE_MS);
		// Write data back to the UART
		if (len > 0) {
			uart_write_bytes(uart_num, (const char *) data, len);
			at_recvTask((const char*) data);
		}
		memset(data, 0, BUF_SIZE);
	}
}

void sendStr(char *str) {
	uart_write_bytes(uart_num, (const char*) str, strlen(str));
}


void at_recvTask(char *str) {
	if ((str[strlen(str) - 1] == '\n') && (str[strlen(str) - 2] == '\r')) {
		if ((memcmp(str, "AT", 2) == 0)) {
			for (int i = 0; i < MAKE_AT_COUNT; i++) {
				if (strstr(str, at_custom_cmd[i].at_cmdName) != NULL) {
					if (str[2 + at_custom_cmd[i].at_cmdLen] == '=') {
						int len = 3 + at_custom_cmd[i].at_cmdLen;
						char *buf = (char *) malloc(64);
						mid(buf, str, strlen(str) - 2 - len, len);
						at_custom_cmd[i].at_setupCmd(123, buf);
						free(buf);
						return;
					} else if (str[2 + at_custom_cmd[i].at_cmdLen] != '='
							&& (str[2 + at_custom_cmd[i].at_cmdLen] != '?')
							&& (at_custom_cmd[i].at_testCmd != NULL)) {
						at_custom_cmd[i].at_testCmd(132);
						return;
					}
				}
			}
			sendStr("error\r\n");
		} else {
			sendStr("error\r\n");
		}
	} else {
		sendStr("error\r\n");
	}
}

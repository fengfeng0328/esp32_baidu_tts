//
// Created by fu on 3/5/18.
//

#ifndef SPEECH_C_DEMO_TTSHTTP_H
#define SPEECH_C_DEMO_TTSHTTP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "common.h"
#include "stdlib.h"

struct http_result {
    int has_error;
	char *format; // 音频的格式，aue参数设置
    FILE *fp; // 保存结果的文件
};

QueueHandle_t data_que;

struct tts_info {
	int len;
	char data[2048];	// 需要与components/curl/include/curl/curl.h宏CURL_MAX_WRITE_SIZE一致
};


size_t writefunc_data(void *ptr, size_t size, size_t nmemb, struct http_result *result);

size_t header_callback(char *buffer, size_t size, size_t nitems, struct http_result *result);

#endif //SPEECH_C_DEMO_TTSHTTP_H

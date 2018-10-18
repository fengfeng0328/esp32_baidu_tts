#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2s.h"
#include "../components/ac101_driver/AC101.h"
#include "../components/ac101_driver/recoder.h"
#include "../components/ac101_driver/uart.h"

#include "nvs_flash.h"
#include "sdkconfig.h"

#include <curl/curl.h>
//#include <memory.h>
#include "string.h"
#include "common.h"
#include "ttsmain.h"
#include "token.h"
#include "ttscurl.h"

#include "simple_wifi.h"

extern QueueHandle_t data_que;

#define CONFIG_AC101_I2S_DATA_IN_PIN 35

const char TTS_SCOPE[] = "audio_tts_post";
const char API_TTS_URL[] = "http://tsn.baidu.com/text2audio"; // 可改为https

RETURN_CODE fill_config(struct tts_config *config) {
    // 填写网页上申请的appkey 如 g_api_key="g8eBUMSokVB1BHGmgxxxxxx"
    char api_key[] = "4E1BG9lTnlSeIf1NQFlrSq6h";
    // 填写网页上申请的APP SECRET 如 $secretKey="94dc99566550d87f8fa8ece112xxxxx"
    char secret_key[] = "544ca4657ba8002e3dea3ac2f5fdd241";

    // text 的内容为"欢迎使用百度语音合成"的urlencode,utf-8 编码
    // 可以百度搜索"urlencode"
    char text[] = "欢迎使用百度语音,我是小度,请问有什么可以帮助你";

    // 发音人选择, 0为普通女声，1为普通男生，3为情感合成-度逍遥，4为情感合成-度丫丫，默认为普通女声
    int per = 0;
    // 语速，取值0-9，默认为5中语速
    int spd = 5;
    // #音调，取值0-9，默认为5中语调
    int pit = 5;
    // #音量，取值0-9，默认为5中音量
    int vol = 5;
    // 下载的文件格式, 3：mp3(default) 4： pcm-16k 5： pcm-8k 6. wav
	int aue = 4;

    // 将上述参数填入config中
    snprintf(config->api_key, sizeof(config->api_key), "%s", api_key);
    snprintf(config->secret_key, sizeof(config->secret_key), "%s", secret_key);
    snprintf(config->text, sizeof(text), "%s", text);
    config->text_len = sizeof(text) - 1;
    snprintf(config->cuid, sizeof(config->cuid), "1234567C");
    config->per = per;
    config->spd = spd;
    config->pit = pit;
    config->vol = vol;
	config->aue = aue;

	// aue对应的格式，format
	const char formats[4][4] = {"mp3", "pcm", "pcm", "wav"};
	snprintf(config->format, sizeof(config->format), formats[aue - 3]);

    return RETURN_OK;
}

RETURN_CODE run() {
    struct tts_config config;
    char token[MAX_TOKEN_SIZE];

    RETURN_CODE res = fill_config(&config);
    if (res == RETURN_OK) {
        // 获取token
        res = speech_get_token(config.api_key, config.secret_key, TTS_SCOPE, token);
        if (res == RETURN_OK) {
            // 调用识别接口
            run_tts(&config, token);
        }
    }

    return RETURN_OK;
}

// 调用识别接口
RETURN_CODE run_tts(struct tts_config *config, const char *token) {
    char params[200 + config->text_len * 9];
    CURL *curl = curl_easy_init(); // 需要释放
    char *cuid = curl_easy_escape(curl, config->cuid, strlen(config->cuid)); // 需要释放
    char *textemp = curl_easy_escape(curl, config->text, config->text_len); // 需要释放
	char *tex = curl_easy_escape(curl, textemp, strlen(textemp)); // 需要释放
	curl_free(textemp);
	char params_pattern[] = "ctp=1&lan=zh&cuid=%s&tok=%s&tex=%s&per=%d&spd=%d&pit=%d&vol=%d&aue=%d";
    snprintf(params, sizeof(params), params_pattern , cuid, token, tex,
             config->per, config->spd, config->pit, config->vol, config->aue);

	char url[sizeof(params) + 200];
	snprintf(url, sizeof(url), "%s?%s", API_TTS_URL, params);
    printf("test in browser: %s\n", url);
    curl_free(cuid);
  	curl_free(tex);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params);
    curl_easy_setopt(curl, CURLOPT_URL, API_TTS_URL);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5); // 连接5s超时
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60); // 整体请求60s超时
    struct http_result result = {1, config->format ,NULL};
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback); // 检查头部
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);  // 需要释放
    curl_easy_setopt(curl, CURLOPT_VERBOSE, ENABLE_CURL_VERBOSE);
    CURLcode res_curl = curl_easy_perform(curl);

    RETURN_CODE res = RETURN_OK;
    if (res_curl != CURLE_OK) {
        // curl 失败
        snprintf(g_demo_error_msg, BUFFER_ERROR_SIZE, "perform curl error:%d, %s.\n", res,
                 curl_easy_strerror(res_curl));
        res = ERROR_TTS_CURL;
    }
	if (result.fp != NULL) {
		fclose(result.fp);
	}
    curl_easy_cleanup(curl);
    return res;
}

static void audio_recorder_AC101_init()
{
	AC101_init();

	i2s_config_t i2s_config = {
	        .mode = I2S_MODE_MASTER |I2S_MODE_RX | I2S_MODE_TX,
	        .sample_rate = 16000,
	        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
	        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,                           //1-channels
	        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
	        .dma_buf_count = 32,
	        .dma_buf_len = 32 *2,
	        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
	    };

	i2s_pin_config_t pin_config_rx = {
	        .bck_io_num = CONFIG_AC101_I2S_BCK_PIN,
	        .ws_io_num = CONFIG_AC101_I2S_LRCK_PIN,
	        .data_out_num = CONFIG_AC101_I2S_DATA_PIN,
	        .data_in_num = CONFIG_AC101_I2S_DATA_IN_PIN
	    };

	int reg_val = REG_READ(PIN_CTRL);
	REG_WRITE(PIN_CTRL, 0xFFFFFFF0);
	reg_val = REG_READ(PIN_CTRL);
	PIN_FUNC_SELECT(GPIO_PIN_REG_0, 1); //GPIO0 as CLK_OUT1

	/* 注册i2s设备驱动 */
	i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
	/* 设置i2s引脚 */
	i2s_set_pin(I2S_NUM_0, &pin_config_rx);
	/* 停止i2s设备 */
	i2s_stop(I2S_NUM_0);
}

static void alexa__AC101_task(void *pvParameters)
{
	char buf[2048];
	int recv_len=0;
	i2s_start(I2S_NUM_0);
	while(1)
	{
		recv_len=i2s_read_bytes(I2S_NUM_0,buf,2048,0);
		i2s_write_bytes(I2S_NUM_0,buf,recv_len,0);
	}
}

static void tts_task(void *pvParameters) {
	data_que = xQueueCreate(200, sizeof(struct tts_info));	// 625k内存

	curl_global_init(CURL_GLOBAL_ALL);
	RETURN_CODE rescode = run();
	curl_global_cleanup();
	if (rescode != RETURN_OK) {
		fprintf(stderr, "ERROR: %s, %d", g_demo_error_msg, rescode);
	}

	i2s_start(I2S_NUM_0);
	while(1)
	{
		printf("## 2 ##");
		xQueueReceive(data_que,&tts_r,portMAX_DELAY);
		i2s_write_bytes(I2S_NUM_0,tts_r.data,tts_r.len,portMAX_DELAY);
	}

//	while(1);
//	printf("tts_task over!\n");
	vTaskDelete(NULL);
}

void app_main() {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES
			|| ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

#if EXAMPLE_ESP_WIFI_MODE_AP
	printf("ESP_WIFI_MODE_AP\n");
	wifi_init_softap();
#else
	printf("ESP_WIFI_MODE_STA\n");
	wifi_init_sta();
#endif /*EXAMPLE_ESP_WIFI_MODE_AP*/

	audio_recorder_AC101_init();
//	xTaskCreatePinnedToCore(&alexa__AC101_task, "alexa__AC101_task", 8096, NULL,
//			2, NULL, 1);
	xTaskCreatePinnedToCore(&tts_task, "tts", 8096 * 4, NULL, 2, NULL, 1);
}


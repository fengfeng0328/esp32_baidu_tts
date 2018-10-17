#include "recoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "math.h"
#include <string.h>

#define BUF_LEN     60
char *buff;                                   //存放从DMA获取到的原始IIS数据


//读取一次数据，然后将数据打印出来，完成后删除线程
void recoder_task(void)
{
    char buff[BUF_LEN];
    int recv_len=0;
    int i =0;
    unsigned short data;
    while(1)
    {
        recv_len = i2s_read_bytes(0,buff, BUF_LEN,portMAX_DELAY);
        i2s_write_bytes(0,buff,recv_len,portMAX_DELAY);
    }

 //   vTaskDelete(xHandle);
          
}

/*
说明：创建录音处理任务
参数：无
返回值：无
*/
void recorder_task_setup(void)
{
    xTaskCreate(&recoder_task, "recoder_task", 5000, NULL, 4, NULL);
}

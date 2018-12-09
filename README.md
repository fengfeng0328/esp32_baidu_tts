# esp32_baidu_tts
Baidu tts based on esp32

/* 移植百度语音合成TTS到ESP32平台，已实现TTS合成 */

TTS synthesis has been realized by transplanting baidu speech synthesis TTS to ESP32 platform


Hardware platform: Ai-Thinker ESP32-Aduio-Kit Audio Development Board

https://item.taobao.com/item.htm?spm=a1z10.5-c-s.w4002-16491566042.12.510625e7rCXkuY&id=578317054056


How to use Help:

1.Fill in the text that needs to be synthesized by voice

char text[] = "欢迎使用百度语音,我是小度,请问有什么可以帮助你";

2.To "make menuconfig" Configure The Wifi 

3.To "make -j2 flash monitor" && TTS speech is played from the speaker

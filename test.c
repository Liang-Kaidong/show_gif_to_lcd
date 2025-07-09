/*************************************************************************************************************
File name: test.c
Author: KD
Version: V_1.0
Build date: 2025-07-07
Description: demo.
Others: Usage requires preservation of original author attribution.
bug: you tell me!
*************************************************************************************************************/
#include <stdio.h>
#include "show_gif_to_lcd.h"

int main() {
    /* 在LCD(0,0)位置，以1024x600的大小播放test.gif，循环播放 */
    int ret = show_gif_to_lcd("test.gif", 0, 0, 1024, 600, 0);
    if (ret < 0) {
        /* 输出播放失败信息 */
        printf("播放失败\n");
        return 1;
    }
    /* 输出播放完成信息 */
    printf("播放完成，共显示%d帧\n", ret);
    return 0;
}
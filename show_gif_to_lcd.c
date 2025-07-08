/* 添加此宏定义以启用 timespec 和 nanosleep 支持 */
#define _POSIX_C_SOURCE 199309L  
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <time.h>
/* 引入gifdec库 */
#include "gifdec.h"  

/* LCD 设备路径 */
#define LCD_DEVICE "/dev/fb0"

/* 全局变量：LCD帧缓冲与尺寸 */
static uint16_t *lcd_buf = NULL;
static int lcd_width = 0;
static int lcd_height = 0;
static int lcd_fd = -1;

/* RGB888转RGB565（适配16位LCD） */
static uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

/* 初始化LCD：打开设备、映射帧缓冲 */
static int lcd_init(void) {
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

    lcd_fd = open(LCD_DEVICE, O_RDWR);
    if (lcd_fd < 0) {
        /* 输出错误信息 */
        fprintf(stderr, "无法打开LCD设备\n");
        return -1;
    }

    /* 获取LCD参数 */
    if (ioctl(lcd_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        /* 输出错误信息 */
        fprintf(stderr, "无法获取LCD参数\n");
        close(lcd_fd);
        return -1;
    }
    lcd_width = vinfo.xres;
    lcd_height = vinfo.yres;

    /* 检查是否为16位模式（RGB565） */
    if (vinfo.bits_per_pixel != 16) {
        /* 输出错误信息 */
        fprintf(stderr, "LCD需为16位(RGB565)模式\n");
        close(lcd_fd);
        return -1;
    }

    /* 映射帧缓冲 */
    if (ioctl(lcd_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        /* 输出错误信息 */
        fprintf(stderr, "无法获取帧缓冲信息\n");
        close(lcd_fd);
        return -1;
    }
    lcd_buf = (uint16_t *)mmap(NULL, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, lcd_fd, 0);
    if (lcd_buf == MAP_FAILED) {
        /* 输出错误信息 */
        fprintf(stderr, "无法映射帧缓冲\n");
        close(lcd_fd);
        return -1;
    }

    return 0;
}

/* 释放LCD资源 */
static void lcd_close(void) {
    if (lcd_buf != MAP_FAILED) munmap(lcd_buf, lcd_width * lcd_height * 2);
    if (lcd_fd >= 0) close(lcd_fd);
}

/* 显示一帧图像到LCD（x,y为起始坐标） */
static void lcd_draw_frame(uint8_t *frame_data, int frame_w, int frame_h, int x, int y, int width, int height) {
    int row, col;
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            /* 计算LCD坐标（确保不超出屏幕范围） */
            int lcd_x = x + col;
            int lcd_y = y + row;
            if (lcd_x >= lcd_width || lcd_y >= lcd_height) continue;

            /* 计算帧数据坐标 */
            int frame_x = (col * frame_w) / width;
            int frame_y = (row * frame_h) / height;

            /* 从帧数据中获取RGB（gifdec的帧数据为RGB888格式，按行存储） */
            uint8_t *pixel = frame_data + (frame_y * frame_w + frame_x) * 3;
            uint8_t r = pixel[0], g = pixel[1], b = pixel[2];

            /* 转换为RGB565并写入帧缓冲 */
            lcd_buf[lcd_y * lcd_width + lcd_x] = rgb888_to_rgb565(r, g, b);
        }
    }
}

/* 播放GIF主函数 */
int show_gif_to_lcd(const char *filename, int x, int y, int width, int height, int play_time) {
    /* 初始化LCD */
    if (lcd_init() < 0) return -1;

    /* 打开GIF文件 */
    gd_GIF *gif = gd_open_gif(filename);
    if (!gif) {
        /* 输出错误信息 */
        fprintf(stderr, "无法打开GIF文件: %s\n", filename);
        lcd_close();
        return -1;
    }

    int frame_count = 0;
    time_t start_time = time(NULL);

    while (1) {
        if (play_time > 0 && time(NULL) - start_time >= play_time) {
            break;
        }

        if (gd_get_frame(gif) != 1) {
            if (play_time == 0) {
                gd_rewind(gif);  /* 循环播放 */
                continue;
            } else {
                break;
            }
        }

        uint8_t *frame_buffer = (uint8_t *)malloc(gif->width * gif->height * 3);
        if (!frame_buffer) {
            /* 输出错误信息 */
            fprintf(stderr, "内存分配失败\n");
            break;
        }
        gd_render_frame(gif, frame_buffer);

        /* 显示当前帧到LCD */
        lcd_draw_frame(frame_buffer, gif->width, gif->height, x, y, width, height);

        free(frame_buffer);

        /* 按GIF指定的延迟等待（单位：毫秒） */
        int delay_ms = gif->gce.delay * 10;  /* gif->gce.delay单位为1/100秒 */
        if (delay_ms > 0) {
            struct timespec ts = {delay_ms / 1000, (delay_ms % 1000) * 1000000};
            nanosleep(&ts, NULL);
        }

        frame_count++;
    }

    /* 清理资源 */
    gd_close_gif(gif);
    lcd_close();
    return frame_count;
}
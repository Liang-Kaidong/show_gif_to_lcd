CC = arm-linux-gnueabihf-gcc
CFLAGS = -std=gnu99 -Wall

# 目标文件
OBJS = show_gif_to_lcd.o gifdec.o

# 生成静态库
libshow_gif_to_lcd.a: $(OBJS)
	ar rcs $@ $(OBJS)

# 生成测试程序
show_gif_to_lcd: test.c libshow_gif_to_lcd.a
	$(CC) $(CFLAGS) -o $@ test.c -L. -lshow_gif_to_lcd

# 编译源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# 清理
clean:
	rm -f *.o *.a show_gif_to_lcd
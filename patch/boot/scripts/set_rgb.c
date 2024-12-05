
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LED_NUM (255)
#define WS2812_PATH "/dev/ws2812-led"

int main(int argc, char *argv[]) {
	int fd;
	int rgb[MAX_LED_NUM];
	int ret;

	int led_num = argc - 1;
	if (led_num == 0) {
		printf( "ws2812: have not set the LED color\n");
		return 0;
	}
	if (led_num > MAX_LED_NUM) {
		printf( "ws2812: can only supports up to %d LEDs, The excess part will be ignored\n", MAX_LED_NUM);
		led_num = MAX_LED_NUM;
	}

	fd = open(WS2812_PATH, O_WRONLY);
	if(fd < 0){
		printf("ws2812: [%s] open err\n", WS2812_PATH);
		return -1;
	}
	for (int i = 0; i < led_num; i++) {
		rgb[i] = strtol(argv[i + 1], NULL, 16);
	}

	ret = write(fd, rgb, led_num * 4);
	if(ret < 0) {
		printf("ws2812: write error: %d, num: %d \n", ret, led_num);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

// gcc set_rgb.c -o set_rgb

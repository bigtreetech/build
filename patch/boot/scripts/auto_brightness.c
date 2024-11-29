#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/i2c-dev.h>

// Macros for array numbers
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define BH1750_I2C_ADDR    0x5C
#define BH1750_POWER_DOWN  0x00
#define BH1750_POWER_ON    0x01
#define BH1750_RESET       0x07 // must after POWER_ON
#define BH1750_MODE_H2     0x11 // high resolution mode 2, 120ms

#define BH1750_THRESHLD_MIN (30)
#define BH1750_THRESHLD_MAX (200)

typedef struct light_map {
    int bh1750_val;
    int pwm_percent;
} light_map_t;

const light_map_t light_map[] = {
    {BH1750_THRESHLD_MIN, 1},
    {47, 5},
    {64, 10},
    {81, 15},
    {98, 20},
    {115, 30},
    {132, 40},
    {149, 50},
    {166, 60},
    {183, 80},
    {BH1750_THRESHLD_MAX, 100},
};

#define PWM_TIME_NS (1000000) // 1KHz

int bh1750_init(const char * i2c_num) {
    char path[16];
    int fd;
    int ret;
    char bh1750_reg[] = {BH1750_POWER_ON, BH1750_RESET, BH1750_MODE_H2};

    snprintf(path, sizeof(path), "/dev/i2c-%s", i2c_num);
    fd = open(path, O_RDWR | O_NONBLOCK);
	if(fd < 0)	{
		printf("bh1750: open \"%s\" err!\n", path);
		return -1;
	}
    ret = ioctl(fd, I2C_SLAVE, BH1750_I2C_ADDR);
    if (ret < 0) {
        printf("bh1750: ioctl I2C_SLAVE[0x%X] failed: %d\n", BH1750_I2C_ADDR, ret);
        close(fd);
        return -1;
    }
    for(int i = 0; i < sizeof(bh1750_reg); i++)    {
        ret = write(fd, &bh1750_reg[i], 1);
        if (ret < 0) {
            printf("bh1750: init reg[0x%X] failed: %d\n", bh1750_reg[i], ret);
            close(fd);
            return -1;
        }
    }
    usleep(200000); // 200ms

    return fd;
}

int read_lights(int fd) {
    char r_buf[2];
    int r_len;

    r_len = read(fd, r_buf, 2);
    if (r_len != 2) {
        return -1;
    }
    return ((r_buf[0] << 8) | r_buf[1]) >> 1; // val / 2
}

int pwm_write_val(const char * chip, const char * ch, const char *attr, const int val) {
    char path[256];
    char w_buf[32];
    int w_len;
    int fd;
    int ret;

    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%s/pwm%s/%s", chip, ch, attr);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("pwm: chip[%s] ch[%s] attr[%s] open failed\n", chip, ch, attr);
        return -1;
    }

    snprintf(w_buf, sizeof(w_buf), "%d", val);
    w_len = strlen(w_buf);
    ret = write(fd, w_buf, w_len);
    if (ret != w_len) {
        printf("pwm: chip[%s] ch[%s] attr[%s] write error, [w_buf: %s, w_len: %d != ret : %d]\n", chip, ch, attr, w_buf, w_len, ret);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int pwm_setup(const char * chip, const char * ch, int freq_ns) {
    char path[256];
    int fd;
    int ret;

    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%s/pwm%s", chip, ch);
    if (access(path, F_OK)) { // 如果pwm目录不存在, 则导出
        snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%s/export", chip);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            printf("pwm: chip[%s] open failed\n", chip);
            return -1;
        }
        ret = write(fd, ch, 1);
        if (ret != 1) {
            printf("pwm: chip[%s] ch[%s] export failed\n", chip, ch);
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (pwm_write_val(chip, ch, "period", freq_ns)) {
        return -1;
    }
    // 100%
    if (pwm_write_val(chip, ch, "duty_cycle", freq_ns)) {
        return -1;
    }
    // pwm enable output
    if (pwm_write_val(chip, ch, "enable", 1)) {
        return -1;
    }

    return 0;
}

int pwm_change_level(const char * chip, const char * ch, int act, int target) {
    int ns = 0;
    if (act == target) {
        return act;
    }
    if (act > target) {
        act--;
    } else {
        act++;
    }
    ns = light_map[act].pwm_percent * PWM_TIME_NS / 100;
    pwm_write_val(chip, ch, "duty_cycle", ns);
    return act;
}

int main(int argc, char* argv[]) {
    const char * bh1750_i2c_num = argv[1];
    const char * pwm_chip = argv[2];
    const char * pwm_ch = argv[3];
    int bh1750_fd;
    int bh1750_data;
    int ret;

    const int max_level = ARRAY_SIZE(light_map) - 1;
    int act_level = max_level;
    int target_level = max_level;

    bh1750_fd = bh1750_init(bh1750_i2c_num);
    if (bh1750_fd < 0) {
        return -1;
    }

    ret = pwm_setup(pwm_chip, pwm_ch, PWM_TIME_NS); // 1KHz
    if (ret) {
        close(bh1750_fd);
        return -1;
    }

    #define LOOP_DELAY_US (20000)
    int bh1750_time_us = 0;
    while (1) {
        if (bh1750_time_us >= 200000) { // 200ms
            bh1750_data = read_lights(bh1750_fd);
            bh1750_time_us = 0;
            if (bh1750_data >= 0) {
                for (int i = 0; i < ARRAY_SIZE(light_map); i++) {
                    if ((bh1750_data > light_map[i].bh1750_val)
                        && (i != max_level)) {
                        continue;
                    }
                    target_level = i;
                    break;
                }
            }
        }

        if (act_level != target_level) {
            act_level = pwm_change_level(pwm_chip, pwm_ch, act_level, target_level);
        }
        usleep(LOOP_DELAY_US); // 20ms
        bh1750_time_us += LOOP_DELAY_US;
    }

    return 0;
}

// gcc auto_brightness.c -o auto_brightness

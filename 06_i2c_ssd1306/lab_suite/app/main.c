#include "graphics.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/i2c-dev.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stopped;
static int fd = -1;
static int trace_mode;
static const char *sysfs;
static unsigned transfers, fail_at;

static void stop_handler(int number) { (void)number; stopped = 1; }

static int write_exact(int target, const uint8_t *data, size_t count)
{
    ssize_t actual = write(target, data, count);
    if (actual == -1) return -1;
    if ((size_t)actual != count) { errno = EIO; return -1; }
    return 0; /* Never retry a partial I2C transaction as a new payload. */
}

static int send_wire(const uint8_t *data, size_t count)
{
    ++transfers;
    if (fail_at && transfers == fail_at) { errno = EIO; return -1; }
    if (!trace_mode) return write_exact(fd, data, count);
    printf("TX %u:", transfers);
    for (size_t i=0; i<count; ++i) printf(" %02X", data[i]);
    putchar('\n');
    return ferror(stdout) ? -1 : 0;
}

static int sysfs_write(const char *attribute, const uint8_t *data, size_t count)
{
    char path[4096];
    int n = snprintf(path, sizeof path, "%s/%s", sysfs, attribute);
    if (n < 0 || (size_t)n >= sizeof path) { errno=ENAMETOOLONG; return -1; }
    int out = open(path, O_WRONLY | O_CLOEXEC);
    if (out == -1) return -1;
    int result = write_exact(out, data, count);
    int saved = errno;
    if (close(out) == -1 && result == 0) return -1;
    errno = saved;
    return result;
}

static int command(const char *name, uint8_t value)
{
    if (sysfs)
        return sysfs_write("command", (const uint8_t *)name, strlen(name));
    uint8_t payload[] = {0x00, value};
    return send_wire(payload, sizeof payload);
}

static int contrast(uint8_t value)
{
    if (sysfs) {
        char number[8];
        int n=snprintf(number,sizeof number,"%u",value);
        return sysfs_write("contrast",(const uint8_t *)number,(size_t)n);
    }
    const uint8_t data[]={0x00,0x81,value};
    return send_wire(data,sizeof data);
}

static int flush_frame(const uint8_t *fb)
{
    if (sysfs) return sysfs_write("frame", fb, OLED_BYTES);
    const uint8_t window[] = {OLED_WINDOW_BYTES};
    if (send_wire(window, sizeof window) == -1) return -1;
    uint8_t packet[17] = {0x40};
    for (size_t offset=0; offset<OLED_BYTES; offset+=16) {
        memcpy(packet+1, fb+offset, 16);
        if (send_wire(packet, sizeof packet) == -1) return -1;
    }
    return 0;
}

static int initialize(void)
{
    if (sysfs)
        return sysfs_write("command", (const uint8_t *)"init", 4);
    const uint8_t init[] = {OLED_INIT_BYTES};
    return send_wire(init, sizeof init);
}

static void pause_ms(unsigned ms)
{
    if (trace_mode) return;
    struct timespec remaining = {.tv_sec=(time_t)(ms/1000),
                                .tv_nsec=(long)(ms%1000)*1000000L};
    while (!stopped && nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR) { stopped=1; break; }
    }
}

static int show_scene(const char *name, unsigned frame)
{
    uint8_t fb[OLED_BYTES];
    scene(fb,name,frame);
    return flush_frame(fb);
}

static int demo(void)
{
    if (command("all",0xA5) == -1) return -1;
    puts("DEMO all: entire panel ON (bypasses RAM)"); pause_ms(2000);
    if (command("ram",0xA4) == -1) return -1;
    const char *scenes[] = {"clear","fill","pages","checker","border","text"};
    for (size_t i=0; i<sizeof scenes/sizeof scenes[0] && !stopped; ++i) {
        printf("DEMO %s\n",scenes[i]);
        if (show_scene(scenes[i],0) == -1) return -1;
        pause_ms(2000);
    }
    if (!stopped) {
        puts("DEMO invert: inverted OLED text");
        if (command("invert",0xA7) == -1) return -1;
        pause_ms(2000);
    }
    if (command("normal",0xA6) == -1) return -1;
    puts("DEMO contrast: low then high");
    if (!stopped) {
        if (contrast(0x10)==-1) return -1;
        pause_ms(2000);
        if (contrast(0xCF)==-1) return -1;
        pause_ms(2000);
    }
    if (contrast(0x8F)==-1) return -1;
    puts("DEMO animate: moving 8x8 square");
    for (unsigned i=0; i<241 && !stopped; ++i) {
        if (show_scene("animate",i) == -1) return -1;
        pause_ms(30);
    }
    if (show_scene("clear",0) == -1) return -1;
    return command("off",0xAE);
}

static void usage(const char *program)
{
    printf("Usage: %s [--bus /dev/i2c-1 | --sysfs PATH | --trace] "
           "[--fail-at N (trace only)] TEST\n"
           "TEST: demo init all clear fill pages checker border text animate "
           "contrast on off invert normal ram\n"
           "Fixed SSD1306 128x32, address 0x3c. All tests except the five\n"
           "commands on/off/invert/normal/ram initialize and clear first.\n",program);
}

int main(int argc, char **argv)
{
    const char *bus="/dev/i2c-1", *test=NULL;
    unsigned backend_options=0;
    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i],"--help")) { usage(argv[0]); return 0; }
        if (!strcmp(argv[i],"--trace")) { trace_mode=1; ++backend_options; }
        else if ((!strcmp(argv[i],"--bus") || !strcmp(argv[i],"--sysfs")) && i+1<argc) {
            bool is_bus = !strcmp(argv[i],"--bus");
            ++backend_options;
            if (is_bus) bus=argv[++i]; else sysfs=argv[++i];
        } else if (!strcmp(argv[i],"--fail-at") && i+1<argc) {
            char *end; const char *number=argv[++i]; errno=0;
            unsigned long n=strtoul(number,&end,10);
            if (!*number || strspn(number,"0123456789")!=strlen(number) ||
                errno || *end || n==0 || n>1000000) {
                fputs("Invalid failure index\n",stderr); return 2;
            }
            fail_at=(unsigned)n;
        } else if (argv[i][0]=='-' || test) { usage(argv[0]); return 2; }
        else test=argv[i];
    }
    if (backend_options>1 || (fail_at && !trace_mode) || !test) {
        usage(argv[0]); return 2;
    }
    const char *names[]={"demo","init","all","clear","fill","pages","checker",
                         "border","text","animate","contrast","on","off","invert","normal","ram"};
    bool valid=false;
    for (size_t i=0; i<sizeof names/sizeof names[0]; ++i)
        if (!strcmp(test,names[i])) valid=true;
    if (!valid) { usage(argv[0]); return 2; }

    struct sigaction sa={0}; sa.sa_handler=stop_handler; sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT,&sa,NULL)==-1 || sigaction(SIGTERM,&sa,NULL)==-1) {
        perror("sigaction"); return 1;
    }
    if (!trace_mode && !sysfs) {
        fd=open(bus,O_RDWR|O_CLOEXEC);
        if (fd==-1) { perror("open adapter"); return 1; }
        /* Advisory: stops other copies of this lab, not arbitrary I2C tools. */
        if (flock(fd,LOCK_EX|LOCK_NB)==-1 || ioctl(fd,I2C_SLAVE,0x3c)==-1) {
            perror("lock/select address"); close(fd); return 1;
        }
    }
    int result=0;
    if (!strcmp(test,"on")) result=command("on",0xAF);
    else if (!strcmp(test,"off")) result=command("off",0xAE);
    else if (!strcmp(test,"invert")) result=command("invert",0xA7);
    else if (!strcmp(test,"normal")) result=command("normal",0xA6);
    else if (!strcmp(test,"ram")) result=command("ram",0xA4);
    else {
        result=initialize();
        if (!result) result=show_scene("clear",0);
        if (!result) result=command("on",0xAF);
        if (!result) {
            if (!strcmp(test,"demo")) result=demo();
            else if (!strcmp(test,"all")) result=command("all",0xA5);
            else if (!strcmp(test,"contrast")) {
                result=show_scene("text",0);
                if (!result) result=contrast(0x10);
                pause_ms(2000);
                if (!result) result=contrast(0xCF);
                pause_ms(2000);
                if (!result) result=contrast(0x8F);
            }
            else if (!strcmp(test,"animate")) {
                for (unsigned i=0; i<241 && !stopped && !result; ++i) {
                    result=show_scene("animate",i); pause_ms(30);
                }
            } else if (strcmp(test,"init")) result=show_scene(test,0);
        }
    }
    if (result==-1) perror("OLED operation (frame may be partially updated)");
    if (stopped) {
        if (command("off",0xAE)==-1) perror("shutdown OFF");
    }
    if (fd!=-1 && close(fd)==-1 && !result) { perror("close"); result=-1; }
    return stopped ? 130 : (result ? 1 : 0);
}

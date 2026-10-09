#include "system_info.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int print_uptime(FILE *output)
{
    FILE *input = fopen("/proc/uptime", "r");
    double uptime_seconds;

    if (input == NULL) {
        fprintf(stderr, "cannot open /proc/uptime: %s\n", strerror(errno));
        return -1;
    }

    if (fscanf(input, "%lf", &uptime_seconds) != 1) {
        fprintf(stderr, "cannot parse /proc/uptime\n");
        fclose(input);
        return -1;
    }

    if (fclose(input) != 0) {
        fprintf(stderr, "cannot close /proc/uptime: %s\n", strerror(errno));
        return -1;
    }

    fprintf(output, "uptime_seconds=%.2f\n", uptime_seconds);
    return 0;
}

int print_memory(FILE *output)
{
    FILE *input = fopen("/proc/meminfo", "r");
    char line[256];
    unsigned long total_kib = 0;
    unsigned long available_kib = 0;

    if (input == NULL) {
        fprintf(stderr, "cannot open /proc/meminfo: %s\n", strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), input) != NULL) {
        if (sscanf(line, "MemTotal: %lu kB", &total_kib) == 1)
            continue;
        (void)sscanf(line, "MemAvailable: %lu kB", &available_kib);
    }

    if (ferror(input)) {
        fprintf(stderr, "cannot read /proc/meminfo: %s\n", strerror(errno));
        fclose(input);
        return -1;
    }

    if (fclose(input) != 0) {
        fprintf(stderr, "cannot close /proc/meminfo: %s\n", strerror(errno));
        return -1;
    }

    if (total_kib == 0 || available_kib == 0) {
        fprintf(stderr, "required fields are missing from /proc/meminfo\n");
        return -1;
    }

    fprintf(output, "memory_total_kib=%lu\n", total_kib);
    fprintf(output, "memory_available_kib=%lu\n", available_kib);
    return 0;
}


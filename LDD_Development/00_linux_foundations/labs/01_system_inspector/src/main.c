#include "system_info.h"

#include <stdio.h>
#include <string.h>

static void print_usage(FILE *output, const char *program)
{
    fprintf(output, "Usage: %s {uptime|memory|all}\n", program);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage(stderr, argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "uptime") == 0)
        return print_uptime(stdout) == 0 ? 0 : 1;

    if (strcmp(argv[1], "memory") == 0)
        return print_memory(stdout) == 0 ? 0 : 1;

    if (strcmp(argv[1], "all") == 0) {
        if (print_uptime(stdout) != 0)
            return 1;
        return print_memory(stdout) == 0 ? 0 : 1;
    }

    fprintf(stderr, "unknown command: %s\n", argv[1]);
    print_usage(stderr, argv[0]);
    return 2;
}


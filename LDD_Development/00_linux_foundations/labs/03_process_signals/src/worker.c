#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested;

static void handle_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

int main(void)
{
    struct sigaction action = {0};
    unsigned int sequence = 0;

    action.sa_handler = handle_stop;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        fprintf(stderr, "worker: sigaction: %s\n", strerror(errno));
        return 1;
    }

    while (!stop_requested) {
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 100000000L};
        printf("heartbeat pid=%ld sequence=%u\n", (long)getpid(), sequence++);
        fflush(stdout);
        while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {
            if (stop_requested)
                break;
        }
    }
    printf("worker pid=%ld stopped\n", (long)getpid());
    return 0;
}


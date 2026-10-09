#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested;

static void handle_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static int report_status(int status)
{
    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("supervisor: worker exited code=%d\n", code);
        return code;
    }
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "supervisor: worker killed by signal=%d\n",
                WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return 1;
}

int main(int argc, char *argv[])
{
    struct sigaction action = {0};
    const struct timespec delay = {.tv_sec = 0, .tv_nsec = 20000000L};
    pid_t child;
    int status;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s WORKER_PATH\n", argv[0]);
        return 2;
    }
    action.sa_handler = handle_stop;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        fprintf(stderr, "supervisor: sigaction: %s\n", strerror(errno));
        return 1;
    }

    child = fork();
    if (child < 0) {
        fprintf(stderr, "supervisor: fork: %s\n", strerror(errno));
        return 1;
    }
    if (child == 0) {
        execl(argv[1], argv[1], (char *)NULL);
        fprintf(stderr, "supervisor: exec '%s': %s\n", argv[1],
                strerror(errno));
        _exit(127);
    }

    printf("supervisor: worker_pid=%ld\n", (long)child);
    fflush(stdout);
    for (;;) {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child)
            return report_status(status);
        if (result < 0 && errno != EINTR) {
            fprintf(stderr, "supervisor: waitpid: %s\n", strerror(errno));
            return 1;
        }
        if (stop_requested)
            break;
        (void)nanosleep(&delay, NULL);
    }

    printf("supervisor: forwarding SIGTERM\n");
    fflush(stdout);
    if (kill(child, SIGTERM) != 0 && errno != ESRCH) {
        fprintf(stderr, "supervisor: kill: %s\n", strerror(errno));
        return 1;
    }
    while (waitpid(child, &status, 0) < 0) {
        if (errno == EINTR)
            continue;
        fprintf(stderr, "supervisor: waitpid: %s\n", strerror(errno));
        return 1;
    }
    return report_status(status);
}


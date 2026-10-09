#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define COPY_BUFFER_SIZE 16384

static int write_all(int fd, const unsigned char *buffer, size_t length)
{
    size_t done = 0;
    while (done < length) {
        ssize_t result = write(fd, buffer + done, length - done);
        if (result > 0) {
            done += (size_t)result;
        } else if (result < 0 && errno == EINTR) {
            continue;
        } else {
            fprintf(stderr, "safecp: write: %s\n",
                    result == 0 ? "zero-byte write" : strerror(errno));
            return -1;
        }
    }
    return 0;
}

static int copy_data(int source_fd, int destination_fd)
{
    unsigned char buffer[COPY_BUFFER_SIZE];
    for (;;) {
        ssize_t count = read(source_fd, buffer, sizeof(buffer));
        if (count > 0) {
            if (write_all(destination_fd, buffer, (size_t)count) != 0)
                return -1;
        } else if (count == 0) {
            return 0;
        } else if (errno != EINTR) {
            fprintf(stderr, "safecp: read: %s\n", strerror(errno));
            return -1;
        }
    }
}

int main(int argc, char *argv[])
{
    bool force = false;
    const char *source;
    const char *destination;
    struct stat source_stat;
    struct stat destination_stat;
    int source_fd = -1;
    int destination_fd = -1;
    int status = 1;

    if (argc == 4 && strcmp(argv[1], "--force") == 0) {
        force = true;
        source = argv[2];
        destination = argv[3];
    } else if (argc == 3) {
        source = argv[1];
        destination = argv[2];
    } else {
        fprintf(stderr, "Usage: %s [--force] SOURCE DESTINATION\n", argv[0]);
        return 2;
    }

    source_fd = open(source, O_RDONLY | O_CLOEXEC);
    if (source_fd < 0 || fstat(source_fd, &source_stat) != 0) {
        fprintf(stderr, "safecp: source '%s': %s\n", source, strerror(errno));
        goto cleanup;
    }
    if (!S_ISREG(source_stat.st_mode)) {
        fprintf(stderr, "safecp: source is not a regular file\n");
        goto cleanup;
    }

    errno = 0;
    if (stat(destination, &destination_stat) == 0 &&
        source_stat.st_dev == destination_stat.st_dev &&
        source_stat.st_ino == destination_stat.st_ino) {
        fprintf(stderr, "safecp: source and destination are the same file\n");
        goto cleanup;
    }
    if (errno != 0 && errno != ENOENT) {
        fprintf(stderr, "safecp: destination '%s': %s\n", destination,
                strerror(errno));
        goto cleanup;
    }

    destination_fd = open(destination,
        O_WRONLY | O_CREAT | O_CLOEXEC | (force ? O_TRUNC : O_EXCL),
        source_stat.st_mode & 0777);
    if (destination_fd < 0) {
        fprintf(stderr, "safecp: destination '%s': %s\n", destination,
                strerror(errno));
        goto cleanup;
    }

    if (copy_data(source_fd, destination_fd) == 0)
        status = 0;

cleanup:
    if (destination_fd >= 0 && close(destination_fd) != 0)
        status = 1;
    if (source_fd >= 0 && close(source_fd) != 0)
        status = 1;
    return status;
}


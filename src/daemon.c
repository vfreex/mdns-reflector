/*
    This file is part of mDNS Reflector (mdns-reflector), a lightweight and performant multicast DNS (mDNS) reflector.
    Copyright (C) 2021 Yuxiang Zhu <me@yux.im>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "daemon.h"
#include "logging.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <signal.h>
#include <fcntl.h>
#include <libgen.h>

void daemonize(const char *program) {
    // Set file creation mode mask.
    umask(S_IWGRP | S_IRWXO);

    // Create a new session and become a session lead to lose controlling TTY.
    pid_t pid = fork();
    if (pid < 0) {
        log_err(LOG_ERR, "%s: can't fork", program);
        exit(EXIT_FAILURE);
    }
    if (pid) // parent process
        exit(EXIT_SUCCESS);
    setsid();

    // Open log
    openlog(program, LOG_PID | LOG_CONS, LOG_DAEMON);
    log_to_syslog = true;

    // Ignore SIGHUP
    struct sigaction sa = {.sa_handler = SIG_IGN, .sa_flags = 0};
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        log_err(LOG_ERR, "%s: can't ignore SIGHUP", program);
        exit(EXIT_FAILURE);
    }

    // Fork again to avoid obtaining controlling TTY again.
    pid = fork();
    if (pid < 0) {
        log_err(LOG_ERR, "%s: can't fork", program);
        exit(EXIT_FAILURE);
    }
    if (pid) // parent process
        exit(EXIT_SUCCESS);

    // Close stdin, stdout, and stderr.
    for (int i = 0; i < 3; ++i)
        close(i);

    // Change the working directory to "/".
    if (chdir("/") < 0) {
        log_err(LOG_ERR, "%s: can't chdir", program);
        exit(EXIT_FAILURE);
    }

    // Attach fd 0, 1, and 2 to /dev/null.
    int fd_in = open("/dev/null", O_RDWR);
    int fd_out = dup(fd_in);
    int fd_err = dup(fd_in);
    if (fd_in != STDIN_FILENO || fd_out != STDOUT_FILENO || fd_err != STDERR_FILENO) {
        log_err(LOG_ERR, "%s: unexpected file descriptors for stdin, stdout, and stderr", program);
        exit(EXIT_FAILURE);
    }
}

static int lockfile(int fd) {
    struct flock lock = {
            .l_start = 0,
            .l_len = 0,
            .l_pid = 0,
            .l_type = F_WRLCK,
            .l_whence = SEEK_SET,
    };
    return fcntl(fd, F_SETLK, &lock);
}

int create_and_lock_pid_file(const char *pid_file_path) {
    // Create and lock pid file.
    char path[MAXPATHLEN];
    snprintf(path, sizeof(path), "%s", pid_file_path);
    if (mkdir(dirname(path), 0755) < 0) {
        if (errno != EEXIST)
            return -1;
    }
    int fd = open(pid_file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        return -1;
    }
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) { // already running
            close(fd);
            return 1;
        }
        return -1;
    }
    if (ftruncate(fd, 0) < 0) {
        return -1;
    }
    char buffer[(CHAR_BIT * sizeof(int) / 3) + 3];
    int pid = getpid();
    int len = snprintf(buffer, sizeof(buffer), "%d", pid);
    if (write(fd, buffer, (size_t) len) < 0) {
        return -1;
    }
    return 0;
}

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

#include "options.h"
#include "daemon.h"
#include "logging.h"
#include "reflection_zone.h"
#include "reflector.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <syslog.h>
#include <sys/param.h>

static const char *DEFAULT_PID_FILE = "/var/run/mdns-reflector/mdns-reflector.pid";

static int parse_args(const char *program, int argc, char *argv[], struct options *options) {
    memset(options, 0, sizeof(struct options));
    strcpy(options->pid_file, DEFAULT_PID_FILE);
    options->log_level = LOG_WARNING;
    int ch;
    while ((ch = getopt(argc, argv, "hdfp:n64l:")) != -1) {
        switch (ch) {
            case 'h':
                options->help = true;
                break;
            case 'd':
                options->debug = true;
                options->foreground = true;
                options->no_pid_file = true;
                options->log_level = LOG_DEBUG;
                break;
            case 'f':
                options->foreground = true;
                break;
            case 'p':
                snprintf(options->pid_file, MAXPATHLEN, "%s", optarg);
                break;
            case 'n':
                options->no_pid_file = true;
                break;
            case '6':
                options->ipv6_only = true;
                break;
            case '4':
                options->ipv4_only = true;
                break;
            case 'l': {
                if (strcmp(optarg, "debug") == 0)
                    options->log_level = LOG_DEBUG;
                else if (strcmp(optarg, "info") == 0)
                    options->log_level = LOG_INFO;
                else if (strcmp(optarg, "warning") == 0)
                    options->log_level = LOG_WARNING;
                else if (strcmp(optarg, "error") == 0)
                    options->log_level = LOG_ERR;
                else {
                    fprintf(stderr, "Unsupported log level: %s\n", optarg);
                    errno = EINVAL;
                    return -1;
                }
                break;
            }
            case '?':
            default:
                errno = EINVAL;
                return -1;
        }
    }
    if (options->help)
        return 0;
    if (options->ipv6_only && options->ipv4_only) {
        fputs("ERROR: '-6' and '-4' are mutually exclusive.\n", stderr);
        return -1;
    }
    log_setlevel(options->log_level);
    if (!options->pid_file[0]) {
        strcpy(options->pid_file, DEFAULT_PID_FILE);
    }
    for (int i = 0; i < argc - optind; ++i) {
        const char *arg = argv[optind + i];
        bool separator = strcmp(arg, "--") == 0;
        if (!options->ipv4_only && (!options->rz_list6 || separator)) {
            // new IPv6 reflection zone
            struct reflection_zone *rz6 = new_reflection_zone(options->rz_list6 ? options->rz_list6->zone_index + 1 : 0,
                                                              options->rz_list6);
            if (!rz6) {
                log_err(LOG_ERR, "%s: can't malloc", program);
                return -1;
            }
            options->rz_list6 = rz6;
        }
        if (!options->ipv6_only && (!options->rz_list4 || separator)) {
            // new IPv4 reflection zone
            struct reflection_zone *rz4 = new_reflection_zone(options->rz_list4 ? options->rz_list4->zone_index + 1 : 0,
                                                              options->rz_list4);
            if (!rz4) {
                log_err(LOG_ERR, "%s: can't malloc", program);
                return -1;
            }
            options->rz_list4 = rz4;
        }
        if (separator)
            continue;
        unsigned int ifindex = if_nametoindex(arg);
        if (!ifindex) {
            log_msg(LOG_ERR, "%s: unknown interface %s", program, arg);
            return -1;
        }
        if (!options->ipv4_only) {
            // new IPv6 reflection interface
            struct reflection_if *rif = new_reflection_if(ifindex, arg, options->rz_list6);
            if (!rif) {
                log_err(LOG_ERR, "%s: can't malloc", program);
                return -1;
            }
        }
        if (!options->ipv6_only) {
            // new IPv4 reflection interface
            struct reflection_if *rif = new_reflection_if(ifindex, arg, options->rz_list4);
            if (!rif) {
                log_err(LOG_ERR, "%s: can't malloc", program);
                return -1;
            }
        }
    }
    // Check reflection zones
    if (!options->ipv4_only && !check_reflection_zone(options->rz_list6)) {
        return -1;
    }
    if (!options->ipv6_only && !check_reflection_zone(options->rz_list4)) {
        return -1;
    }
    return 0;
}

static void usage(const char *program, FILE *file) {
    fprintf(file, "mDNS Reflector version %s\n", "0.0.1-dev");
    fputs("Copyright (C) 2021 Yuxiang Zhu <me@yux.im>\n\n", file);

    fprintf(file, "usage: %s [OPTION]... <IFNAME> <IFNAME>...\n", program);
    fprintf(file, "   or: %s [OPTION]... <IFNAME> <IFNAME>... [-- <IFNAME> <IFNAME>...]...\n", program);
    fprintf(file, "Use '--' to separate reflection zones. A mDNS packet coming from an interface will only ");
    fprintf(file, "be reflected to other interfaces within the same zone.\n");
    fprintf(file, "\n");
    fprintf(file, "Examples:\n");
    fprintf(file, "  # Reflect between eth0 and eth1\n");
    fprintf(file, "  %s eth0 eth1\n", program);
    fprintf(file, "  # Reflect 2 zones. br-lan0, br-lan1 and br-lan2 are in one zone. br-lan3 br-lan4 are in the other zone.\n");
    fprintf(file, "  %s br-lan0 br-lan1 br-lan2 -- br-lan3 br-lan4\n", program);
    fprintf(file, "\n");
    fprintf(file, "Options\n");  // hdfp:n64l:
    fprintf(file, " -d\tdebug mode (implies -f -n -l debug)\n");
    fprintf(file, " -f\tforeground mode\n");
    fprintf(file, " -n\tdon't create PID file\n");
    fprintf(file, " -l\tset logging level (debug, info, warning, error; default is warning)\n");
    fprintf(file, " -p\tPID file path (default is %s)\n", DEFAULT_PID_FILE);
    fprintf(file, " -4\tIPV4 only mode (disable IPv6 support)\n");
    fprintf(file, " -6\tIPV6 only mode (disable IPv4 support)\n");
    fprintf(file, " -h\tshow this help\n");
    fprintf(file, "\n");
    fprintf(file, "See https://github.com/vfreex/mdns-reflector for updates, bug reports, and answers\n");


}

int main(int argc, char *argv[]) {
    char program[MAXPATHLEN];
    snprintf(program, sizeof(program), "%s", basename(argv[0]));
    struct options options;

    if (parse_args(program, argc, argv, &options) == -1) {
        fprintf(stderr, "\nRun '%s -h' for help.\n", program);
        return EXIT_FAILURE;
    }

    if (options.help) {
        usage(program, stdout);
        return EXIT_SUCCESS;
    }

    if (!options.foreground) {
        daemonize(program);
    }

    if (!options.no_pid_file) {
        int already_running = create_and_lock_pid_file(options.pid_file);
        if (already_running < 0) {
            log_err(LOG_ERR, "%s: can't create PID file %s", program, options.pid_file);
            return EXIT_FAILURE;
        } else if (already_running) {
            log_msg(LOG_ERR, "%s: another instance is running", program);
            return EXIT_FAILURE;
        }
    }

    int r = run_event_loop(&options);
    return r;
}

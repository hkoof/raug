/* raug - Run As User-Group 
 *
 * Copyright 2011 Heiko Noordhof <h.e.noordhof@rug.nl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#define MAXMSG   (512)
#define MAXVARGS (4096)

const char *progname = "raug";

static int verbose_flag = 0;
static int version_flag = 0;
static int help_flag = 0;


void message(const char *fmt, ...)
{
    va_list ap;
    char buf[MAXMSG];

    if (verbose_flag) {
        va_start(ap, fmt);
        vsnprintf(buf, MAXMSG, fmt, ap);
        fputs(buf, stdout);
        fflush(NULL);
        va_end(ap);
    }
}


int username2uid(const char *name)
{
    struct passwd *pw;

    errno = 0;
    pw = getpwnam(name);
    if (pw == NULL) {
        if (errno) 
            perror("Error: while converting user name to UID");
        else 
            fprintf(stderr, "Error: unknown user %s\n", name);
        exit(1);
    }
    return pw->pw_uid;
}


int groupname2gid(const char *name)
{
    struct group *grp;

    errno = 0;
    grp = getgrnam(name);
    if (grp == NULL) {
        if (errno) 
            perror("Error: while converting group name to GID");
        else 
            fprintf(stderr, "Error: unknown group %s\n", name);
        exit(1);
    }
    return grp->gr_gid;
}


int main(int argc, char *argv[]) 
{
    int i;
    int c = 0;
    int optidx = 0;

    uid_t uid = 0;
    gid_t gid = 0;   

    char *username = 0;
    char *groupname = 0;
    char *userenvvar = 0;
    char *groupenvvar = 0;

    char *execargv[MAXVARGS];

    static struct option long_options[] = {
        {"help",         no_argument,       &help_flag, 1},
        {"version",      no_argument,       &version_flag, 1},
        {"verbose",      no_argument,       &verbose_flag, 1},
        {"user",         required_argument, 0, 'u'},
        {"user-envvar",  required_argument, 0, 'U'},
        {"group",        required_argument, 0, 'g'},
        {"group-envvar", required_argument, 0, 'G'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "u:U:g:G:", long_options, &optidx)) != -1) {        
        switch (c) {
            case 0: break; /* current option being parsed is a flag-type option */
            case 'u': username = optarg;    break;
            case 'U': userenvvar = optarg;  break;
            case 'g': groupname = optarg;   break;
            case 'G': groupenvvar = optarg; break;
            case '?':
                /* getopt_long() already printed an error message. */
                fprintf(stderr, "Try `%s --help' for more information.\n", progname);
                exit(2);
            default: /* Should never happen! getopt() called wrongly or a 'case' is missing. */
                abort();
        }
    }

    if (version_flag) {
        printf("%s %s\n", progname, VERSION);
        exit(0);
    }

    if (help_flag) {
        printf("\
Usage: %s [OPTION]... COMMAND [ARG]...\n\
Runs COMMAND with arguments ARG... as arbitrary user and group.\n\
COMMAND must be full path to an executable file\n\
By default user and group are not changed, in which case this program\n\
is effectively a no-op.\n\
(needless to say, this program only works for root)\n\
\n\
Options:\n\
   -u --user=USER           run COMMAND as USER  (precedes --user-envvar)\n\
   -g --group=GROUP         run COMMAND as GROUP (precedes --group-envvar)\n\
   -U --user-envvar=ENVVAR  run COMMAND as user read from environment\n\
                            variable ENVVAR\n\
   -G --group-envvar=ENVVAR run COMMAND as user read from environment\n\
                            variable ENVVAR\n\
      --verbose             print to stdout what is done\n\
      --help                display this help and exit\n\
      --version             output version information and exit\n\
\n\
",
               progname);
        exit(2);
    }

    if (optind >= argc) {        
        fprintf(stderr, "No command specified.\n");
        fprintf(stderr, "Try `%s --help' for more information.\n", progname);
        exit(2);
    }

    if (getuid() != 0) {
        fprintf(stderr, "Running %s only makes sense for root, which you are not.\n", progname);
        exit(1);
    }

    /* Change GID first, since we may lose permission to do so if we change UID first. */

    /* Find out what GID we need to set */
    if (!groupname && groupenvvar) {
        message("Reading group name from environment variable %s.\n", groupenvvar);
        if ((groupname = getenv(groupenvvar)) == NULL) {
            fprintf(stderr, "Environment variable %s not found\n", groupenvvar);
            exit(1);
        }
    }
    if (groupname) {
        message("Group: %s\n", groupname);
        gid = groupname2gid(groupname);
        message("GID:   %d\n", gid);
        if (setgid(gid) == -1) {  /* Set new GID */
            perror("Error in setgid()");
            exit(1);
        }
    }

    /* Find out what UID we need to set */
    if (!username && userenvvar) {
        message("Reading user name from environment variable %s.\n", userenvvar);
        if ((username = getenv(userenvvar)) == NULL) {
            fprintf(stderr, "Environment variable %s not found\n", userenvvar);
            exit(1);
        }
    }
    if (username) {
        message("User: %s\n", username);
        uid = username2uid(username);
        message("UID:  %d\n", uid);
        if (setuid(uid) == -1) {  /* Set new UID */
            perror("Error in setuid()");
            exit(1);
        }
    }

    /* Process non-option arguments, i.e. put them in array suitable for execvp() */
    message("Executing: ");
    for (i = 0;; ++optind,++i) {
        if (optind >= argc) {
            execargv[i] = NULL;
            break;
        }
        if (i >= MAXVARGS-1) {
            fprintf(stderr, "Error: too many command line arguments (max. %d).\n", MAXVARGS);
            exit(1);
        }        
        execargv[i] = argv[optind];
        message("%s ", execargv[i]);
    }
    message("\n");

    /* Execute */
    execv(execargv[0], execargv);

    /* If we get here at all, execv() has failed. */
    fprintf(stderr, "Error executing %s: ", execargv[0]);
    perror("");
         
    return 1;
}

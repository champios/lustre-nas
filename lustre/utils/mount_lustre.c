/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 *  Copyright (C) 2002 Cluster File Systems, Inc.
 *   Author: Robert Read <rread@clusterfs.com>
 *   Author: Nathan Rutman <nathan@clusterfs.com>
 *
 *   This file is part of Lustre, http://www.lustre.org.
 *
 *   Lustre is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License as published by the Free Software Foundation.
 *
 *   Lustre is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Lustre; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <mntent.h>
#include <getopt.h>
#include <sys/utsname.h>

#include <linux/lustre_disk.h>
//#include <portals/ptlctl.h>
#include "obdctl.h"

int          verbose;
int          nomtab;
int          fake;
int          force;
static char *progname = NULL;

void usage(FILE *out)
{
        fprintf(out, "usage: %s <mdsnode>:/<mdsname>/<cfgname> <mountpt> "
                "[-fhnv] [-o mntopt]\n", progname);
        fprintf(out, "\t<mdsnode>: nid of MDS (config) node\n"
                "\t<mdsname>: name of MDS service (e.g. mds1)\n"
                "\t<cfgname>: name of client config (e.g. client)\n"
                "\t<mountpt>: filesystem mountpoint (e.g. /mnt/lustre)\n"
                "\t-f|--fake: fake mount (updates /etc/mtab)\n"
                "\t--force: force mount even if already in /etc/mtab\n"
                "\t-h|--help: print this usage message\n"
                "\t-n|--nomtab: do not update /etc/mtab after mount\n"
                "\t-v|--verbose: print verbose config settings\n");
        exit(out != stdout);
}

static int load_module(char *module_name)
{
        char buf[256];
        int rc;
        
        if (verbose)
                printf("loading %s\n", module_name);
        sprintf(buf, "/sbin/modprobe %s", module_name);
        rc = system(buf);
        if (rc) {
                fprintf(stderr, "%s: failed to modprobe %s: %s\n", 
                        progname, module_name, strerror(errno));
                fprintf(stderr, "Check /etc/modules.conf\n");
        }
        return rc;
}

static int load_modules()
{
        int rc = 0;

        rc = load_module("lustre");
        rc = load_module("mds");
        rc = load_module("ost");
        return rc;
}

static int check_mtab_entry(char *spec, char *mtpt, char *type)
{
        FILE *fp;
        struct mntent *mnt;

        if (force)
                return (0);

        fp = setmntent(MOUNTED, "r");
        if (fp == NULL)
                return(0);

        while ((mnt = getmntent(fp)) != NULL) {
                if (strcmp(mnt->mnt_fsname, spec) == 0 &&
                        strcmp(mnt->mnt_dir, mtpt) == 0 &&
                        strcmp(mnt->mnt_type, type) == 0) {
                        fprintf(stderr, "%s: according to %s %s is "
                                "already mounted on %s\n",
                                progname, MOUNTED, spec, mtpt);
                        return(1); /* or should we return an error? */
                }
        }
        endmntent(fp);

        return(0);
}

static int
update_mtab_entry(char *spec, char *mtpt, char *type, char *opts,
                  int flags, int freq, int pass)
{
        FILE *fp;
        struct mntent mnt;
        int rc = 0;

        mnt.mnt_fsname = spec;
        mnt.mnt_dir = mtpt;
        mnt.mnt_type = type;
        mnt.mnt_opts = opts ? opts : "";
        mnt.mnt_freq = freq;
        mnt.mnt_passno = pass;

        fp = setmntent(MOUNTED, "a+");
        if (fp == NULL) {
                fprintf(stderr, "%s: setmntent(%s): %s:",
                        progname, MOUNTED, strerror (errno));
                rc = 16;
        } else {
                if ((addmntent(fp, &mnt)) == 1) {
                        fprintf(stderr, "%s: addmntent: %s:",
                                progname, strerror (errno));
                        rc = 16;
                }
                endmntent(fp);
        }

        return rc;
}


/*****************************************************************************
 *
 * This part was cribbed from util-linux/mount/mount.c.  There was no clear
 * license information, but many other files in the package are identified as
 * GNU GPL, so it's a pretty safe bet that was their intent.
 *
 ****************************************************************************/
struct opt_map {
        const char *opt;        /* option name */
        int skip;               /* skip in mtab option string */
        int inv;                /* true if flag value should be inverted */
        int mask;               /* flag mask value */
};

static const struct opt_map opt_map[] = {
  { "defaults", 0, 0, 0         },      /* default options */
  { "rw",       1, 1, MS_RDONLY },      /* read-write */
  { "ro",       0, 0, MS_RDONLY },      /* read-only */
  { "exec",     0, 1, MS_NOEXEC },      /* permit execution of binaries */
  { "noexec",   0, 0, MS_NOEXEC },      /* don't execute binaries */
  { "suid",     0, 1, MS_NOSUID },      /* honor suid executables */
  { "nosuid",   0, 0, MS_NOSUID },      /* don't honor suid executables */
  { "dev",      0, 1, MS_NODEV  },      /* interpret device files  */
  { "nodev",    0, 0, MS_NODEV  },      /* don't interpret devices */
  { "async",    0, 1, MS_SYNCHRONOUS},  /* asynchronous I/O */
  { "auto",     0, 0, 0         },      /* Can be mounted using -a */
  { "noauto",   0, 0, 0         },      /* Can  only be mounted explicitly */
  { "nousers",  0, 1, 0         },      /* Forbid ordinary user to mount */
  { "nouser",   0, 1, 0         },      /* Forbid ordinary user to mount */
  { "noowner",  0, 1, 0         },      /* Device owner has no special privs */
  { "_netdev",  0, 0, 0         },      /* Device accessible only via network */
  { NULL,       0, 0, 0         }
};
/****************************************************************************/

static int parse_one_option(const char *check, int *flagp)
{
        const struct opt_map *opt;

        for (opt = &opt_map[0]; opt->opt != NULL; opt++) {
                if (strncmp(check, opt->opt, strlen(opt->opt)) == 0) {
                        if (opt->inv)
                                *flagp &= ~(opt->mask);
                        else
                                *flagp |= opt->mask;
                        return 1;
                }
        }
        return 0;
}

int parse_options(char *orig_options, int *flagp)
{
        int val;
        char *options, *opt, *opteq;

        options = malloc(strlen(orig_options) + 1);
        strcpy(options, orig_options);

        *flagp = 0;
        /* parsing ideas here taken from util-linux/mount/nfsmount.c */
        for (opt = strtok(options, ","); opt; opt = strtok(NULL, ",")) {
                if ((opteq = strchr(opt, '='))) {
                        val = atoi(opteq + 1);
                        *opteq = '\0';
                        /* All the network options have gone :)) */
                        fprintf(stderr, "%s: unknown option '%s'. "
                                "Ignoring.\n", progname, opt);
                } else {
                        if (parse_one_option(opt, flagp))
                                continue;

                        fprintf(stderr, "%s: unknown option '%s'\n",
                                progname, opt);
                }
        }
        free(options);
        return 0;
}


int main(int argc, char *const argv[])
{
        char *source, *target, *options = "";
        int i, nargs = 3, opt, rc, flags;
        struct lustre_mount_data lmd;
        static struct option long_opt[] = {
                {"fake", 0, 0, 'f'},
                {"force", 0, 0, 1},
                {"help", 0, 0, 'h'},
                {"nomtab", 0, 0, 'n'},
                {"options", 1, 0, 'o'},
                {"verbose", 0, 0, 'v'},
                {0, 0, 0, 0}
        };

        progname = strrchr(argv[0], '/');
        progname = progname ? progname + 1 : argv[0];

        printf("starting %s\n", progname);

        while ((opt = getopt_long(argc, argv, "fhno:v",
                                  long_opt, NULL)) != EOF){
                switch (opt) {
                case 1:
                        ++force;
                        printf("force: %d\n", force);
                        nargs++;
                        break;
                case 'f':
                        ++fake;
                        printf("fake: %d\n", fake);
                        nargs++;
                        break;
                case 'h':
                        usage(stdout);
                        break;
                case 'n':
                        ++nomtab;
                        printf("nomtab: %d\n", nomtab);
                        nargs++;
                        break;
                case 'o':
                        options = optarg;
                        nargs++;
                        break;
                case 'v':
                        ++verbose;
                        printf("verbose: %d\n", verbose);
                        nargs++;
                        break;
                default:
                        fprintf(stderr, "%s: unknown option '%c'\n",
                                progname, opt);
                        usage(stderr);
                        break;
                }
        }

        if (optind + 2 > argc) {
                fprintf(stderr, "%s: too few arguments\n", progname);
                usage(stderr);
        }

        source = argv[optind];
        target = argv[optind + 1];

        if (verbose) {
                for (i = 0; i < argc; i++)
                        printf("arg[%d] = %s\n", i, argv[i]);
                printf("source = %s, target = %s\n", source, target);
        }

        if (!force && check_mtab_entry(source, target, "lustre"))
                exit(32);

        rc = parse_options(options, &flags); 
        if (rc) {
                fprintf(stderr, "%s: can't parse options: %s\n",
                        progname, options);
                exit(1);
        }

        rc = access(target, F_OK);
        if (rc) {
                rc = errno;
                fprintf(stderr, "%s: %s inaccessible: %s\n", progname, target,
                        strerror(errno));
                return 1;
        }

        /* FIXME remove */
        if ((rc = load_modules())) {
                return rc;
        }

        if (!fake)
                /* flags and target get to lustre_get_sb, but not 
                   lustre_fill_super.  Lustre ignores the flags, but mount 
                   does not. */
                rc = mount(source, target, "lustre", flags, (void *)options);
        if (rc) {
                fprintf(stderr, "%s: mount(%s, %s) failed: %s\n", progname, 
                        source, target, strerror(errno));
                if (errno == ENODEV)
                        fprintf(stderr, "Are the lustre modules loaded?\n"
                             "Check /etc/modules.conf and /proc/filesystems\n");
                rc = 32;
        } else if (!nomtab) {
                rc = update_mtab_entry(source, target, "lustre", options,0,0,0);
        }

        return rc;
}

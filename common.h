#include "config.h"

#include <ncurses.h>
#include <term.h>  /* from ncurses.h */

#include <sys/param.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <limits.h>
#include <fnmatch.h>

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

typedef off_t size_type;

#include "color.h"
#include "dir.h"
#include "key.h"
#include "dir_window.h"
#include "file_window.h"
#include "info_window.h"
#include "status_line.h"
#include "ui.h"
#include "util.h"
#include "ops.h"
#include "filespec.h"
#include "ini.h"
#include "config_screen.h"
/* idcache.c from GNU fileutils 3.16 */
#include "idcache.h"
/* modechange.c and modechange.h from GNU fileutils 3.16 */
#include "modechange.h"

/* Editor/viewer used instead of "internal" because there is no internal
	 editor yet. You can specify a different one in the configuration screen. */
#define EDITOR "/usr/bin/pico"
#define VIEWER "/usr/bin/less"


extern int info_width; /* in main.c */
extern int bottom_height; /* in main.c */
extern int minifl_height; /* in main.c */

enum {
	NONE,
	QUIT_MODE,
	FILE_LIST_MODE,
	DIR_TREE_MODE,
	CONFIG_MODE
};

int current_mode;

enum {
	NORMAL_FORMAT,
	DATE_FORMAT,
	ATTRIBUTES_FORMAT
};

int file_display_format;

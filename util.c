/*  util.c - various utility functions
		Copyright (C) 1999 Peter Kelly <peter@area51.org.au>

		This program is free software; you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation; either version 2 of the License, or
		(at your option) any later version.

		This program is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with this program; if not, write to the Free Software Foundation,
		Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* None of the functions in this file depend on other parts of XTC. They are
 	intended to be useful stand-alone functions that can be used in any program. */

#include "common.h"

#define ismode(n,m) ((n & m) == m)

/*******************************************************************************
make_path~

Makes a directory and and directories above it that do not exist.
Returns an error message or NULL.
*******************************************************************************/

char *make_path(char *path) {

	char *error_message = NULL;
	char *parent_path;
	struct stat statbuf;
	
	if (!strcmp(path,"")) return NULL;  /* root dir */
	else if (!stat(path,&statbuf) && S_ISDIR(statbuf.st_mode)) return NULL;  /* dir already exists */
	else {
		parent_path = get_parent_path(path);
		error_message = make_path(parent_path);
		if (!error_message && mkdir(path,0777)) error_message = strerror(errno);
		free(parent_path);
		return error_message;
	}

}


/*******************************************************************************
get_relative_filename~

Returns a version of abs_name that is relative to the path base_path (both
base_path and abs_name must be given as absolute).
*******************************************************************************/

char *get_relative_filename(char *base_path, char *abs_name) {

	char *rel_name = NULL;
	char *base_path2;
	int chars_in_common = 0;
	int last_dir_start = 1;
	int extra_dirs_in_base_path = 0;
	int cur_dir;
	int cur_char;

	/* sanity check */
	if (!base_path || !abs_name || !strcmp(base_path,"") || !strcmp(abs_name,"") ||
			(base_path[0] != '/') || (abs_name[0] != '/')) return NULL;

	/* make sure the base path ends in '/' */
	if (base_path[strlen(base_path)-1] != '/') {
		base_path2 = malloc(strlen(base_path)+2);
		sprintf(base_path2,"%s/",base_path);
	}
	else base_path2 = strdup(base_path);

	/* find out where the paths start to differ */
	while (base_path2[chars_in_common] == abs_name[chars_in_common]) {
		chars_in_common++;
		if (base_path2[chars_in_common-1] == '/') last_dir_start = chars_in_common;
	}
	chars_in_common = last_dir_start;

	/* work out how many "../"'s we need */
	for (cur_char = chars_in_common; cur_char < strlen(base_path2); cur_char++) {
		if (base_path2[cur_char] == '/') extra_dirs_in_base_path++;
	}
	
	/* create the relative name */
	rel_name = malloc(extra_dirs_in_base_path*3+strlen(abs_name)-chars_in_common+1);
	for (cur_dir = 0; cur_dir < extra_dirs_in_base_path; cur_dir++)
		sprintf(&rel_name[cur_dir*3],"../");
	sprintf(&rel_name[extra_dirs_in_base_path*3],"%s",&abs_name[chars_in_common]);
	
	
	free(base_path2);
	
	return rel_name;

}

/*******************************************************************************
join_path_and_name~
*******************************************************************************/

char *join_path_and_name(char *path, char *name) {

	char *filename = malloc(strlen(path)+1+strlen(name)+1);

	if (!strcmp(path,"/")) sprintf(filename,"/%s",name);
	else sprintf(filename,"%s/%s",path,name);

	return filename;

}


/*******************************************************************************
files_are_same~
*******************************************************************************/

int files_are_same(char *filename1, char *filename2) {

	struct stat statbuf1, statbuf2;

	if (!strcmp(filename1,filename2) ||  /* (full) filenames are the same */
			(!lstat(filename1,&statbuf1) &&
 			!lstat(filename2,&statbuf2) &&
 			(statbuf1.st_ino == statbuf2.st_ino) &&  /* inode numbers are the same */
 			(statbuf1.st_dev == statbuf2.st_dev)))  /* src file and dest file are on same device */
		return TRUE;
	else
		return FALSE;

}

/*******************************************************************************
get_window_height~
*******************************************************************************/

int get_window_height(WINDOW *window) {

	int height, width;
	
	getmaxyx(window,height,width);
	return height;

}

/*******************************************************************************
get_window_width~
*******************************************************************************/

int get_window_width(WINDOW *window) {

	int height, width;
	
	getmaxyx(window,height,width);
	return width;

}

/*******************************************************************************
truncate_string~

Allocates memory for and returns a copy of the supplied string which is truncated
to the specified length.
*******************************************************************************/

char *truncate_string(char *string, int length) {

	char *truncated_string = strdup(string);

	if (strlen(truncated_string) > length) truncated_string[length] = 0;

	return truncated_string;

}

/*******************************************************************************
comma_number~
*******************************************************************************/

char *comma_number(long number) {

	static char numstring[12];

	sprintf(numstring,"###########");  /* just in case number is > 11 digits */
	if (number >= 1000000000)
		sprintf(numstring,"%ld",number);
	else if (number >= 1000000)
		sprintf(numstring,"%ld,%03ld,%03ld",number / 1000000,(number % 1000000) / 1000,number % 1000);
	else if (number >= 1000)
		sprintf(numstring,"%ld,%03ld",number / 1000,number % 1000);
	else
		sprintf(numstring,"%ld",number);
	return numstring;

}

/*******************************************************************************
get_full_path~
*******************************************************************************/

char *get_full_path(char *path, char *base_path) {

	static char current_path[PATH_MAX];
	static char full_path[PATH_MAX];
	char *return_path = NULL;

	getcwd(current_path,PATH_MAX);
	if (!path) return NULL;  /* no path specified */
	else if ((path[0] != '/') && !base_path) return NULL;  /* relative path, but no base specified */
	else if (base_path && chdir(base_path)) return NULL;  /* can't chdir to base path */
	else if (chdir(path)) return NULL;  /* can't change to path */

	/* changed to path successfully */
	getcwd(full_path,PATH_MAX);
	return_path = (char*) strdup(full_path);
	chdir(current_path);
	return return_path;

}

/*******************************************************************************
get_parent_path~
*******************************************************************************/

char *get_parent_path(char *path) {

	int cur_char = strlen(path)-1;
	char *return_path;

	if ((strlen(path) == 0) || !strchr(path,'/')) return strdup("");
	else {
		if ((cur_char > 0) && (path[cur_char] == '/')) cur_char--;
		while ((cur_char > 0) && (path[cur_char] != '/')) cur_char--;
		while ((cur_char > 0) && (path[cur_char] == '/')) cur_char--;
		cur_char++;
		return_path = malloc(cur_char+1);
		strncpy(return_path,path,cur_char);
		return_path[cur_char] = 0;
		return return_path;
	}

}

/*******************************************************************************
get_name~
*******************************************************************************/

char *get_name(char *path) {

	int cur_char = strlen(path)-1;
	char *return_name = NULL;

	if (strlen(path) == 0) return strdup("");
	else {
		if ((cur_char > 0) && (path[cur_char] == '/')) cur_char--;
		while ((cur_char > 0) && (path[cur_char] != '/')) cur_char--;
		cur_char++;
		return_name = strdup(&path[cur_char]);
		return return_name;
	}
	
}

/*******************************************************************************
mode_string~

Note: the string this function returns will be overwritten on the next call.
*******************************************************************************/

char *mode_string(mode_t mode) {

	static char pstring[11];
	
	pstring[0] = file_type_letter(mode);
	/* should put other things in here for links, devices etc. */
	pstring[1] = (ismode(mode,S_IRUSR) ? 'r' : '-');
	pstring[2] = (ismode(mode,S_IWUSR) ? 'w' : '-');
	pstring[3] = (ismode(mode,S_IXUSR) ? 'x' : '-');
	pstring[4] = (ismode(mode,S_IRGRP) ? 'r' : '-');
	pstring[5] = (ismode(mode,S_IWGRP) ? 'w' : '-');
	pstring[6] = (ismode(mode,S_IXGRP) ? 'x' : '-');
	pstring[7] = (ismode(mode,S_IROTH) ? 'r' : '-');
	pstring[8] = (ismode(mode,S_IWOTH) ? 'w' : '-');
	pstring[9] = (ismode(mode,S_IXOTH) ? 'x' : '-');
	pstring[10] = 0;

	/* suid/sgid/sticky code adapted from filemode.c in GNU fileutils */
	if (mode & S_ISUID) {
		if (pstring[3] != 'x') pstring[3] = 'S'; /* Set-uid, but not executable by owner.  */
		else pstring[3] = 's';
	}
	if (mode & S_ISGID) {
		if (pstring[6] != 'x') pstring[6] = 'S'; /* Set-gid, but not executable by group.  */
		else pstring[6] = 's';
	}
	if (mode & S_ISVTX) {
		if (pstring[9] != 'x') pstring[9] = 'T'; /* Sticky, but not executable by others.  */
		else pstring[9] = 't';
	}

	return pstring;

}


/*******************************************************************************
is_number~

From userspec.c in GNU fileutils

Return nonzero if STR represents an unsigned decimal integer,
otherwise return 0.
*******************************************************************************/

int is_number(char *str) {

	for (; *str; str++)
		if (!isdigit((int)*str))
			return FALSE;
	return TRUE;
	
}

/*******************************************************************************
get_uid~
*******************************************************************************/

int get_uid(char *user, uid_t *uid) {

	struct passwd *pwentry;

	if (is_number(user))
		pwentry = getpwuid(strtoul(user,NULL,10));
	else
		pwentry = getpwnam(user);
	if (pwentry) {
		*uid = pwentry->pw_uid;
		return TRUE;
	}
	else return FALSE;

}

/*******************************************************************************
get_gid~
*******************************************************************************/

int get_gid(char *group, gid_t *gid) {

	struct group *grentry;

	if (is_number(group))
		grentry = getgrgid(strtoul(group,NULL,10));
	else
		grentry = getgrnam(group);
	if (grentry) {
		*gid = grentry->gr_gid;
		return TRUE;
	}
	else return FALSE;

}


/*******************************************************************************
date_string~

Note: the string this function returns will be overwritten on the next call.
*******************************************************************************/

char *date_string(time_t time, char *format) {

	static char date_string[11];
	struct tm *tm;
	
	tm = localtime(&time);
	strftime(date_string,11,format,tm);
	return date_string;
	
}

/*******************************************************************************
time_string~

Note: the string this function returns will be overwritten on the next call.
*******************************************************************************/

char *time_string(time_t time, char *format) {

	static char time_string[12];
	struct tm *tm;

	tm = localtime(&time);
	strftime(time_string,12,format,tm);
	return time_string;
	
}


/*******************************************************************************
file_type_letter~

From filemode.c in GNU fileutils
*******************************************************************************/

char file_type_letter(long bits) {

#ifdef S_ISBLK
	if (S_ISBLK (bits)) return 'b';
#endif
	if (S_ISCHR (bits)) return 'c';
	if (S_ISDIR (bits)) return 'd';
	if (S_ISREG (bits)) return '-';
#ifdef S_ISFIFO
	if (S_ISFIFO (bits)) return 'p';
#endif
#ifdef S_ISLNK
	if (S_ISLNK (bits)) return 'l';
#endif
#ifdef S_ISSOCK
	if (S_ISSOCK (bits)) return 's';
#endif
#ifdef S_ISMPC
	if (S_ISMPC (bits)) return 'm';
#endif
#ifdef S_ISNWK
	if (S_ISNWK (bits)) return 'n';
#endif
#ifdef S_ISOFD
	if (S_ISOFD (bits)) return 'M';  /* Cray migrated dmf file.  */
#endif
#ifdef S_ISOFL
	if (S_ISOFL (bits)) return 'M';  /* Cray migrated dmf file.  */
#endif
	return '?';
	
}

/*******************************************************************************
file_type_string~
*******************************************************************************/

char *file_type_string(long bits, int abbreviated) {

	static char *typestring[9][2] = {{"Directory",       "Directory"},   /* typestring[0] */
 																	{"Block device",    "Block device"},/* typestring[1] */
 																	{"Character device","Char. device"},/* typestring[2] */
 																	{"Multiplexor",     "Multiplexor"}, /* typestring[3] */
 																	{"Symbolic link",   "Sym. link"},   /* typestring[4] */
 																	{"Socket",          "Socket"},      /* typestring[5] */
 																	{"FIFO",            "FIFO"},        /* typestring[6] */
 																	{"Regular file",    "Regular file"},/* typestring[7] */
 																	{"Unknown",         "Unknown"}};    /* typestring[8] */

	if (abbreviated) abbreviated = 1;
	
	switch(file_type_letter(bits)) {
		case 'd': return typestring[0][abbreviated];
		case 'b': return typestring[1][abbreviated];
		case 'c': return typestring[2][abbreviated];
		case 'm': return typestring[3][abbreviated];
		case 'l': return typestring[4][abbreviated];
		case 's': return typestring[5][abbreviated];
		case 'p': return typestring[6][abbreviated];
		case '-': return typestring[7][abbreviated];
		default: return typestring[8][abbreviated];
	}
}

/*******************************************************************************
get_fs_blocksize~

Stores the blocksize of the filesystem that contains the specified path in blocksize.
Returns 0 on success, -1 on failure (and errno will be set by statfs or statvfs)
*******************************************************************************/

int get_fs_blocksize(char *path, int *blocksize) {

#ifdef HAVE_STATVFS
 	struct statvfs statvfsbuf;

  if (!statvfs(path,&statvfsbuf)) {
	  *blocksize = statvfsbuf.f_bsize;
		return 0;
  }
	else
	  return 1;	
#else
	struct statfs statfsbuf;

  if (!statfs(path,&statfsbuf)) {
	  *blocksize = statfsbuf.f_bsize;
		return 0;
  }
	else
	  return 1;	
#endif
}

/*******************************************************************************
get_fs_freek

Stores the number of free kilobytes in *freek.
Returns 0 on success, -1 on failure (and errno will be set by statfs or statvfs).
*******************************************************************************/

int get_fs_freek(char *path, long *freek) {
	long block_ratio;

#ifdef HAVE_STATVFS
 	struct statvfs statvfsbuf;

  if (!statvfs(path,&statvfsbuf)) {
		block_ratio = statvfsbuf.f_bsize/1024;
		*freek = statvfsbuf.f_bavail*block_ratio;
		return 0;
  }
	else
	  return 1;	
#else
	struct statfs statfsbuf;

  if (!statfs(path,&statfsbuf)) {
		block_ratio = statfsbuf.f_bsize/1024;
		*freek = statfsbuf.f_bavail*block_ratio;
		return 0;
  }
	else
	  return 1;	
#endif

}

/*******************************************************************************
parse_chown_string~

Parses a string in the form of "owner:group" or "owner.group" and sets the
appropriate values for *uid and *gid (-1 for none).

Owner and group can be specified by either name or id, and must be a valid
user/group.

The uid/gid are set as follows:
	If "owner[:.]group", set both
	If "owner", set uid to correct value but gid to -1
	If "[:.]group", set uid to -1 and gid to correct value
	If "owner[:.]", set uid to correct value, set gid to the owner's login group

Returns an error message if either the user or group does not exist, otherwise
returns NULL.
*******************************************************************************/

char *parse_chown_string(char *chown_string, int *uid, int *gid) {

	char *owner, *group, *separator = NULL;
	int use_login_group = FALSE;
	char *error_message = NULL;
	struct passwd *pwentry;

	*uid = *gid = -1;
	owner = strdup(chown_string);
	
	if ((separator = strchr(owner,':')) || (separator = strchr(owner,'.'))) {
		group = malloc(strlen(owner)+1);
		sprintf(group,"%s",separator+1);
		*separator = 0;
		if (!strcmp(group,""))
			use_login_group = TRUE;
		else if (!get_gid(group,gid)) {
			*gid = -1;
			error_message = strdup("Group does not exist");
		}
		free(group);
	}
	if (strcmp(owner,"") && !get_uid(owner,uid)) {
		*uid = -1;
		error_message = strdup("User does not exist");
	}
	if (use_login_group && (pwentry = getpwuid(*uid))) {
		*gid = pwentry->pw_gid;
	}
	free(owner);
	
	return error_message;

}


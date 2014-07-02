/*  ops.c - various file operations
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
		
#include "common.h"


/*******************************************************************************
do_copy~

Copies src_filename to dest_filename, and upates the progress bar based on
*bytes_done and total_bytes.

If an error occurrs, the function returns an error message and if the file
has been written to, it is deleted.
If the operation is canceled, the function returns the string "canceled"
If the operation is successful, the function returns NULL
*******************************************************************************/

char *do_copy(char *src_filename, char *dest_filename, size_type *bytes_done, int total_bytes) {

	char *error_message = NULL;
	int filesize;
	int blocksize;
	char *buffer;
	int source_desc, dest_desc;
	int bytes_read;
	char symlink_dest[PATH_MAX+1];
	int symlink_length;
	struct utimbuf utimbuf;
	float multiplier = 100.0/total_bytes;
	struct stat statbuf;
	int canceled = FALSE;
	
	if (lstat(src_filename,&statbuf)) error_message = strerror(errno);  /* source file does not exist or is not accessible */
	else if (files_are_same(src_filename,dest_filename))
 		error_message = strdup("Source and destination files are the same");
	else if (S_ISLNK(statbuf.st_mode)) {  /* symbolic link */
		symlink_length = readlink(src_filename,symlink_dest,PATH_MAX);
		if (symlink_length < 0) error_message = strerror(errno);
		else {
			symlink_dest[symlink_length] = 0;
			if (symlink(symlink_dest,dest_filename)) error_message = strerror(errno);
#ifdef HAVE_LCHOWN /* in linux 2.2, chown follows symlinks so use lchown instead */
			lchown(dest_filename,statbuf.st_uid,statbuf.st_gid);
#else
			chown(dest_filename,statbuf.st_uid,statbuf.st_gid);
#endif
			/* note: the mtime/atime of the symlink is not changed here as utime() does not work
 				on symlinks, but rather the file they point to */
		}
	}
	else if (!S_ISREG(statbuf.st_mode)) error_message = strdup("Cannot copy non-regular file");
	else if (get_fs_blocksize(src_filename,&blocksize)) error_message = strerror(errno);
	else if ((source_desc = open(src_filename,O_RDONLY)) < 0) error_message = strerror(errno);
	else {
		if ((dest_desc = open(dest_filename,O_WRONLY | O_CREAT | O_TRUNC,statbuf.st_mode)) < 0) error_message = strerror(errno);
		else {
			filesize = statbuf.st_size;
			buffer = malloc(blocksize);
			nodelay(stdscr,TRUE);

			while (((bytes_read = read(source_desc,buffer,blocksize)) > 0) && !error_message && !canceled) {
				if (write(dest_desc,buffer,bytes_read) < 0) error_message = strerror(errno);
				else if (read_key() == F_KEY(10)) canceled = TRUE;
				else {
					*bytes_done += bytes_read;
					if (total_bytes > 0) status_line_progress_bar_set((int)(*bytes_done*multiplier));
				}
			}
			nodelay(stdscr,FALSE);
			free(buffer);
			close(dest_desc);
			chown(dest_filename,statbuf.st_uid,statbuf.st_gid);
			chmod(dest_filename,statbuf.st_mode);
			utimbuf.actime = statbuf.st_atime;
			utimbuf.modtime = statbuf.st_mtime;
			utime(dest_filename,&utimbuf);
			if (error_message || canceled) unlink(dest_filename);
		}
		close(source_desc);
	}
	
	if (canceled) error_message = ("canceled");

	return error_message;

}

/*******************************************************************************
do_move~

Moves src_filename to dest_filename, doing a copy+delete if the files are on
different devices.

If an error occurrs, the function returns an error message.
If the operation is canceled, the function returns the string "canceled"
If the operation is successful, the function returns NULL

Note that if a move between devices fails because the source can not be
deleted, then the dest is deleted, so it is as if the copy never took place.
*******************************************************************************/

char *do_move(char *src_filename, char *dest_filename, size_type *bytes_done, int total_bytes) {

	char *error_message = NULL;
	
	if (rename(src_filename,dest_filename)) {
		if (errno != EXDEV) 
			error_message = strerror(errno);
		else {  /* destination is another device, so copy then delete */
			error_message = do_copy(src_filename,dest_filename,bytes_done,total_bytes);
			if (!error_message &&  /* copy succeeded, so delete source file */
					unlink(src_filename)) {
				/* couldn't delete source file, so set error message and delete dest file
					(and assume it succeeds since we just wrote to it) */
				error_message = strerror(errno);
				unlink(dest_filename);
			}           
		}
	}

	return error_message;

}


/*******************************************************************************
do_chown~

Does a chown operation for a string in the form "owner:group" or "owner.group"
and returns the appropriate error message (if an error occurred) or NULL

Owner and group can be specified by either name or id, and must be a valid
user/group.

The results of the operation follow the same rules as chown(1):
	If "owner[:.]group", change both
	If "owner", only change owner
	If "[:.]group", only change group
	If "owner[:.]", change owner, and set group the the owner's login group
*******************************************************************************/

char *do_chown(char *filename, char *chown_string) {

	uid_t uid = -1;
	gid_t gid = -1;
	char *error_message = NULL;

	error_message = parse_chown_string(chown_string,&uid,&gid);

	if (!error_message && ((uid != -1) || (gid != -1)) &&
#ifdef HAVE_LCHOWN /* in linux 2.2, chown follows symlinks so use lchown instead */
		lchown(filename,uid,gid)
#else
		chown(filename,uid,gid)
#endif
		) error_message = strerror(errno);

	return error_message;

}

/*  dir.c - routines to manipulate logged files and directories
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


															/* file functions */

/*******************************************************************************
file_get_display_line~
*******************************************************************************/

char *file_get_display_line(file_info_struct *file, int length, int format) {

	char *line = NULL;
	char *display_name;
	char date_time_string[100];
	int filename_length;
	char format_string[100];
	int just_display_name = FALSE;

	if (!file || length < 1) return NULL;

	line = malloc(length+1);
	switch (format) {
		case ATTRIBUTES_FORMAT:
			filename_length = length-30;
			if (filename_length < 0) just_display_name = 1;
			else {
				sprintf(format_string,"%%-%ds %%8s %%8s %%10s ",filename_length);

				sprintf(line,format_string,truncate_string(file->name,filename_length),
								file->username,file->groupname,
								mode_string(file->mode));
			}
			break;
		case DATE_FORMAT:
			filename_length = length-34;
			if (filename_length < 0) just_display_name = 1;
			else {
				sprintf(date_time_string,"%s %s",date_string(file->mtime,config.misc.date_format),
								time_string(file->mtime,config.misc.time_format));
				display_name = truncate_string(file->name,filename_length);
				sprintf(format_string,"%%-%ds %%11s %%-22s ",filename_length);
				sprintf(line,format_string,display_name,comma_number(file->size),date_time_string);
				free(display_name);
			}
			break;
		default: /* normal */
			display_name = truncate_string(file->name,length);
			sprintf(format_string,"%%-%ds",length);
			sprintf(line,format_string,display_name);
			free(display_name);
	}
	if (just_display_name) {
		display_name = truncate_string(file->name,length);
		sprintf(line,"%s",display_name);
		free(display_name);
	}
	
	
	return line;
	
}





/*******************************************************************************
file_find_from_full_name~
*******************************************************************************/

file_info_struct *file_find_from_full_name(char *filename, dir_info_struct *rootdir) {

	char *path = get_parent_path(filename);
	dir_info_struct *dir = dir_find_from_path(path,rootdir);
	file_info_struct *file;
	file_info_struct *return_file = NULL;
	
	if (!dir) return NULL;
	if (!dir->files) return NULL;
	
	file = dir->files;
	/* this could probably be optimised by only checking the name */
	while (file && strcmp(file_get_full_name(file),filename)) file = file->next;
	if (file && !strcmp(file_get_full_name(file),filename)) return_file = file;

	free(path);
	return return_file;

}

/*******************************************************************************
file_get_full_name~
*******************************************************************************/

char *file_get_full_name(file_info_struct *file) {

	char *full_name, *path;
	
	path = dir_get_path(file->dir);
	
	full_name = (char*) malloc(strlen(path)+1+strlen(file->name)+1);
	if (!strcmp(path,"/")) sprintf(full_name,"/%s",file->name);
	else sprintf(full_name,"%s/%s",path,file->name);
	free(path);
	
	return full_name;

}






/*******************************************************************************
file_read_and_add~

Reads a file's info from disk and adds it to the appropriate dir.
*******************************************************************************/

file_info_struct *file_read_and_add(char *filename) {

	dir_info_struct *dir;
	file_info_struct *file;
	char *path = get_parent_path(filename);

	dir = dir_find_from_path(path,dir_window_rootdir);
	free(path);
	
	if (!dir) return NULL;
	if (!dir->logged) return NULL;

	file = file_read(filename);
	if (!file) return NULL;
	if (!dir_add_file(dir,file)) return NULL;

	return file;

}



/*******************************************************************************
file_free~
*******************************************************************************/

void file_free(file_info_struct *file) {

	free(file->name);
  /* don't need to free username or groupname as these are stored in the id cache (in idcache.c) */
	free(file);

}

/*******************************************************************************
file_update~
*******************************************************************************/

int file_update(file_info_struct *file) {

	struct stat statbuf;
	
	if (!file) return FALSE;
	if (lstat(file_get_full_name(file),&statbuf) < 0) return FALSE;
	if (S_ISDIR(statbuf.st_mode)) return FALSE;

	file->size = statbuf.st_size;
	file->mtime = statbuf.st_mtime;
	file->mode = statbuf.st_mode;
	file->uid = statbuf.st_uid;
	file->gid = statbuf.st_gid;
	file->username = getuser(file->uid); /* doesn't need to be freed as these */
	file->groupname = getgroup(file->gid); /* are stored in the id cache (in idcache.c) */

	return TRUE;

}



/*******************************************************************************
file_read~

Reads file info into a file structure. Does not place in a directory.
*******************************************************************************/

file_info_struct *file_read(char *filename) {

	struct stat statbuf;
	file_info_struct *file;

	if (!filename) return NULL;
	if (lstat(filename,&statbuf) < 0) return NULL;
	if (S_ISDIR(statbuf.st_mode)) return NULL;

	file = malloc(sizeof(file_info_struct));
	file->name = get_name(filename);
	file->dir = NULL;
	file->next = NULL;

	file->tagged = FALSE;
	file->size = statbuf.st_size;
	file->mtime = statbuf.st_mtime;
	file->mode = statbuf.st_mode;
	file->uid = statbuf.st_uid;
	file->gid = statbuf.st_gid;
	file->username = getuser(file->uid); /* doesn't need to be freed as these */
	file->groupname = getgroup(file->gid); /* are stored in the id cache (in idcache.c) */
		

	return file;
			
}






























































															/* dir functions */


/*******************************************************************************
dir_find_from_path~
*******************************************************************************/

dir_info_struct *dir_find_from_path(char *path, dir_info_struct *rootdir) {

	int cur_char;
	int failed = FALSE;
	char *dir_name;
	char *path_so_far;
	int dir_start = 1;

	dir_info_struct *dir, *subdir;
	
	if (!rootdir || ((path = get_full_path(path,NULL)) == NULL)) return NULL;

	if (!strcmp(path,"/")) return rootdir;

	dir = rootdir;

	dir_name = (char*) malloc(strlen(path)+1);
	path_so_far = (char*) malloc(strlen(path)+1);
	path_so_far[0] = path[0];
	for (cur_char = 1; (cur_char <= strlen(path)) && !failed; cur_char++) {
		if ((path[cur_char] == '/') || (cur_char == strlen(path))) {
			if (cur_char > dir_start) {
				path_so_far[cur_char] = 0;
				dir_name[cur_char-dir_start] = 0;
				
				if (!dir->subdirs) failed = TRUE;
				else {
					subdir = dir->subdirs;
					while (subdir && strcmp(subdir->name,dir_name)) subdir = subdir->next;
					if (!subdir) failed = TRUE;
					else {
						dir = subdir;
					}
				}
			}
			dir_start = cur_char+1;
			path_so_far[cur_char] = path[cur_char];
		}
		else {
			dir_name[cur_char-dir_start] = path[cur_char];
			path_so_far[cur_char] = path[cur_char];
		}
	}

	if (failed) return NULL;
	else return dir;
	
}

/*******************************************************************************
dir_get_path~

Returns the name of the directory relative to /, including the / character.
*******************************************************************************/

char *dir_get_path(dir_info_struct *dir) {

	char *path, *parent_path;
	
	if (!dir->parent) return strdup("/");

	parent_path = dir_get_path(dir->parent);
	path = malloc(strlen(parent_path)+1+strlen(dir->name)+1);
	if (!strcmp(parent_path,"/")) { sprintf(path,"/%s",dir->name); }
	else { sprintf(path,"%s/%s",parent_path,dir->name); }
	free(parent_path);
	return path;

}

/*******************************************************************************
dir_check_if_parent~
*******************************************************************************/

int dir_check_if_parent(dir_info_struct *parent, dir_info_struct *child) {

	if (child->parent == parent) return TRUE;
	else if (child->parent) return dir_check_if_parent(parent,child->parent);
	else return FALSE;

}

/*******************************************************************************
dir_update~
*******************************************************************************/

int dir_update(dir_info_struct *dir) {

	struct stat statbuf;
	
	if (!dir) return FALSE;
	if (lstat(dir_get_path(dir),&statbuf) < 0) return FALSE;
	if (!S_ISDIR(statbuf.st_mode)) return FALSE;
	
	dir->mtime = statbuf.st_mtime;
	dir->mode = statbuf.st_mode;
	dir->uid = statbuf.st_uid;
	dir->gid = statbuf.st_gid;
	dir->username = getuser(dir->uid); /* doesn't need to be freed as these */
	dir->groupname = getgroup(dir->gid); /* are stored in the id cache (in idcache.c) */

	return TRUE;

}

/*******************************************************************************
dir_remove_file~

Only meant for removing single files.
Does NOT free file's memory.
*******************************************************************************/

void dir_remove_file(dir_info_struct *dir, file_info_struct *file) {

	file_info_struct *file2;

	if (dir->files == file) dir->files = file->next;
	else if (dir->files) {
		file2 = dir->files;
		while (file2->next && (file2->next != file)) file2 = file2->next;
		if (file2->next == file) file2->next = file->next;
	}
	file->dir = NULL;
	file->next = NULL;

}

/*******************************************************************************
dir_insert_subdir~

Inserts a subdir into a dir, in the appropriate position (keeping alphabetic
order), and set's the subdir's parent to be dir.
*******************************************************************************/

void dir_insert_subdir(dir_info_struct *dir, dir_info_struct *subdir) {

	dir_info_struct* this_subdir = NULL;
	
	subdir->parent = dir;

	if (!dir->subdirs) {
		dir->subdirs = subdir;
		return;
	}
	
	if (strcasecmp(subdir->name,dir->subdirs->name) < 0) {
		subdir->next = dir->subdirs;
		dir->subdirs = subdir;
		return;
	}

	this_subdir = dir->subdirs;
	while (this_subdir->next && (strcasecmp(subdir->name,this_subdir->next->name) >= 0)) {
		this_subdir = this_subdir->next;
	}
	if (this_subdir->next) subdir->next = this_subdir->next;
	this_subdir->next = subdir;

}

/*******************************************************************************
dir_free~

Assumes the directory is NOT logged (i.e. does not free memory for subdirs or
files)
*******************************************************************************/

void dir_free(dir_info_struct *dir) {

	free(dir->name);
  /* don't need to free username or groupname as these are stored in the id cache (in idcache.c) */
	free(dir);

}

/*******************************************************************************
dir_remove_subdir~

Only meant for removing single subdirs.
Does not free any memory for subdir or it's subdirectories.
*******************************************************************************/

void dir_remove_subdir(dir_info_struct *dir, dir_info_struct *subdir) {

	dir_info_struct *this_subdir;

	if (!dir->subdirs) return;

	if (dir->subdirs == subdir)
		dir->subdirs = subdir->next;
	else {
		this_subdir = dir->subdirs;
		while (this_subdir->next && (this_subdir->next != subdir))
			this_subdir = this_subdir->next;
		if (this_subdir->next && (this_subdir->next == subdir))
			this_subdir->next = subdir->next;
	}
	subdir->next = NULL;
	subdir->parent = NULL;

}


/*******************************************************************************
dir_add_file~

Does NOT initialise file structure... only modifies file->next and file->dir
*******************************************************************************/

int dir_add_file(dir_info_struct *dir, file_info_struct *file) {

	if (!file) return FALSE;

	file->next = dir->files;
	dir->files = file;
	file->dir = dir;
	return TRUE;

}

/*******************************************************************************
dir_read~

Reads dir info into a dir structure. Does not place in directory hierachy.
*******************************************************************************/

dir_info_struct *dir_read(char *path) {

	dir_info_struct *dir;
	struct stat statbuf;

	if (lstat(path,&statbuf) < 0) return NULL;

	dir = (dir_info_struct*) malloc(sizeof(dir_info_struct));
	dir->logged = FALSE;
	dir->parent = NULL;
	dir->subdirs = NULL;
	dir->next = NULL;
	dir->files = NULL;
	dir->name = get_name(path);
	
	dir->mtime = statbuf.st_mtime;
	dir->mode = statbuf.st_mode;
	dir->uid = statbuf.st_uid;
	dir->gid = statbuf.st_gid;
	dir->username = getuser(dir->uid); /* doesn't need to be freed as these */
	dir->groupname = getgroup(dir->gid); /* are stored in the id cache (in idcache.c) */

	return dir;

}


/*******************************************************************************
dir_log~

Returns 0 on success, errno on failure.
*******************************************************************************/

int dir_log(dir_info_struct *dir) {

	DIR* this_dir;
	struct dirent *dir_entry_ptr;
	struct stat statbuf;
	char* abs_name;
	dir_info_struct* new_subdir = NULL;
	
	this_dir = opendir(dir_get_path(dir));
	if (this_dir) {
		while((dir_entry_ptr = readdir(this_dir)) != NULL) {
		
			abs_name = malloc(strlen(dir_get_path(dir))+1+strlen(dir_entry_ptr->d_name)+1);
			if (!strcmp(dir_get_path(dir),"/"))
				sprintf(abs_name,"/%s",dir_entry_ptr->d_name);    
			else sprintf(abs_name,"%s/%s",dir_get_path(dir),dir_entry_ptr->d_name);    
			if ((lstat(abs_name,&statbuf) >= 0) &&
					strcmp(dir_entry_ptr->d_name,".") &&
					strcmp(dir_entry_ptr->d_name,"..")) {
				if (S_ISDIR(statbuf.st_mode) &&
						!S_ISLNK(statbuf.st_mode)) {
					/* a directory */
					new_subdir = dir_read(abs_name);
					/* insert the directory into the proper sorted position */
					dir_insert_subdir(dir,new_subdir);
				}
				else {
				/* a link or file */
					dir_add_file(dir,file_read(abs_name));
				
				}
			}
		}
		closedir(this_dir);
		dir->logged = TRUE;
		return 0;
	}
	else return errno;

}


/*******************************************************************************
dir_unlog~
*******************************************************************************/

void dir_unlog(dir_info_struct* dir) {

	dir_info_struct *subdir, *next_subdir;
	file_info_struct *file, *next_file;

	/* note: the freeing here doesn't seem to work - the memory is still allocated
 		to the program, and logging/unlogging a large branch serveral times starts
 		to chew up memory */
	
	if (dir->subdirs) {
		subdir = dir->subdirs;
		while (subdir) {
			next_subdir = subdir->next;
			dir_unlog(subdir);
			free(subdir->name);
			free(subdir);
			subdir = next_subdir;
		}
		dir->subdirs = NULL;
	}
	if (dir->files) {
		file = dir->files;
		while (file) {
			next_file = file->next;
			file_free(file);
			file = next_file;
		}
		dir->files = NULL;
	}

	dir->logged = FALSE;

}

/*******************************************************************************
dir_calculate_size~

Calculates the total size of all files in a directory, excluding those in
subdirectories.
*******************************************************************************/

off_t dir_calculate_size(dir_info_struct* dir, int (*check_func)(file_info_struct*)) {

	file_info_struct *file;
	off_t size = 0;
	
	if (!dir->files) return 0;
	else {
		file = dir->files;

/*		if (type == TAGGED_FILES)
			do if (file->tagged) size += file->size; while ((file = file->next));
		else
			do size += file->size; while ((file = file->next));*/
		do if (check_func(file)) size += file->size; while ((file = file->next));
		return size;
	}

}

/*******************************************************************************
dir_calculate_total_size~

Calculates the total size of all files in a directory, including those in
subdirectories.
*******************************************************************************/

off_t dir_calculate_total_size(dir_info_struct* dir, int (*check_func)(file_info_struct*)) {

	dir_info_struct *subdir;
	off_t size = dir_calculate_size(dir,check_func);
	
	if (dir->subdirs) {
		subdir = dir->subdirs;
		do {
			size += dir_calculate_total_size(subdir,check_func);
		} while ((subdir = subdir->next));
	}
	return size;

}

/*******************************************************************************
dir_count_files~

Counts the number of files in a directory, excluding subdirectories.
*******************************************************************************/

int dir_count_files(dir_info_struct* dir, int (*check_func)(file_info_struct*)) {

	file_info_struct *file;
	int count = 0;

	if (!dir->files) return 0;
	else {
		file = dir->files;
/*		if (type == TAGGED_FILES)
			do if (file->tagged) count++; while ((file = file->next));
		else 
			do count++; while ((file = file->next));*/
		do if (check_func(file)) count++; while ((file = file->next));
		return count;
	}
}

/*******************************************************************************
dir_count_total_files~

Counts the number of files in a directory, including subdirectories.
*******************************************************************************/

int dir_count_total_files(dir_info_struct* dir, int (*check_func)(file_info_struct*)) {

	dir_info_struct *subdir;
	off_t count = dir_count_files(dir,check_func);
	
	if (dir->subdirs) {
		subdir = dir->subdirs;
		do {
			count += dir_count_total_files(subdir,check_func);
		} while ((subdir = subdir->next));
	}
	return count;

}

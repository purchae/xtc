/*  dir_window.c - routines to manipulate directory window & directory operations
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

int dir_window_lines_top;
int dir_window_lines_count;
int dir_window_lines_allocated;
int dir_window_lines_cur_sel;
dir_line_struct* dir_window_lines;

void dir_window_draw_line(int linenum);
int dir_window_find_line_from_dir(dir_info_struct *dir);
void dir_window_update_lines(int linenum, int update_screen);
void dir_window_add_line(dir_info_struct *subdir, int linenum);
void dir_window_add_lines(dir_info_struct* dir, int linenum);
void dir_window_remove_line(int linenum);
void dir_window_remove_subdir_lines(int linenum);
void dir_window_select(int linenum);
void dir_window_expand_dir(int linenum, int update);
void dir_window_collapse_dir(int linenum);
void dir_window_delete();

/*                                dir operations                              */

/*******************************************************************************
dir_window_prune_error_message~

This could probably be used for other things as well (in which case it should
be renamed, and possibly moved to status_line.c). Note however that it turns
nodelay back on afterwards.
*******************************************************************************/

int dir_window_prune_error_message(char *error_message) {

	int response;

	nodelay(stdscr,FALSE);
	status_line_error_message(error_message);
	response = status_line_get_response("Options:",RESPONSE_RETRYFILE+RESPONSE_CANCEL,FALSE);
	nodelay(stdscr,TRUE);
	status_line_display_command_keys("{F10} cancel");

	return response;

}

/*******************************************************************************
dir_window_prune~
*******************************************************************************/

static void dir_window_prune(void) {

	int linenum = dir_window_lines_cur_sel;
	int height = get_window_height(dir_window);
	int all_logged = TRUE;
	int canceled = FALSE;
	int width = get_window_width(command_window);
	int success;

	char *path;
	DIR* this_dir = NULL;
	struct stat statbuf;
	struct dirent *dir_entry_ptr;
	char *abs_name;
	char *display_name;
	char *error_message;
	
	if (!dir_window_lines[dir_window_lines_cur_sel].dir->logged) {
		werase(command_window);
		wrefresh(command_window);
		status_line_error_message("Directory not logged");
		info_window_update();	
		return;
	}

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"PRUNE:  Delete the hilighted branch of the tree");
	wrefresh(command_window);

	if (!status_line_confirm_string("PRUNE")) {
		info_window_update();	
		return;
	}

	while (linenum+1 < dir_window_lines_count &&
				 dir_check_if_parent(dir_window_lines[dir_window_lines_cur_sel].dir,dir_window_lines[linenum+1].dir)) {
		linenum++;
		if (!dir_window_lines[linenum].dir->logged)
			all_logged = FALSE;
	}
	
	if (!all_logged) {
		status_line_error_message("The selected sub-directory branch is not fully logged.");
		info_window_update();	
		return;
	}

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"PRUNING: ");
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);
	
	nodelay(stdscr,TRUE); /* so the read_key() below will return immediately if
													 no key is pressed */
	status_line_display_command_keys("{F10} cancel");
	while (linenum >= dir_window_lines_cur_sel && !canceled) {
		/* erase all files in the directory, and report error if occurred */
		path = dir_get_path(dir_window_lines[linenum].dir);

		wmove(command_window,0,9);
		wclrtoeol(command_window);
		display_name = truncate_string(path,width-10);
		mvwprintw(command_window,0,9,"%s",display_name);
		free(display_name);
		wrefresh(command_window);

		/* make sure the directory exists, is actually a directory, and that we can read it */
		do {
			error_message = NULL;
			if (lstat(path,&statbuf)) error_message = strdup(strerror(errno));
			else if (!S_ISDIR(statbuf.st_mode)) error_message = strdup("Not a directory");
			else if ((this_dir = opendir(path)) == NULL) error_message = strdup("Could not open directory");
			if (error_message) {
				switch(dir_window_prune_error_message(error_message)) {
					case RESPONSE_CANCEL: canceled = TRUE; break;
				}
				free(error_message);
			}
		} while (error_message && !canceled);

		/* delete all files in the directory */
		if (!canceled) {
			while((dir_entry_ptr = readdir(this_dir)) != NULL && !canceled) {
				success = FALSE;
				while (!success && !canceled) {
					success = TRUE;
					abs_name = malloc(strlen(path)+1+strlen(dir_entry_ptr->d_name)+1);
					if (!strcmp(path,"/"))
						sprintf(abs_name,"/%s",dir_entry_ptr->d_name);    
					else sprintf(abs_name,"%s/%s",path,dir_entry_ptr->d_name);
					if (!lstat(abs_name,&statbuf) &&
							!S_ISDIR(statbuf.st_mode)) {
						/* attempt to delete the file */
						if (unlink(abs_name)) /* error deleting file */
							switch(dir_window_prune_error_message(strerror(errno))) {
								case RESPONSE_RETRYFILE: success = FALSE; break;
								default: canceled = TRUE;
							}
					}
				}
			}
		}
		/* attempt to delete the directory */
		if (linenum > dir_window_lines_cur_sel) { /* don't delete if it's the selected directory; that's done later */
			success = FALSE;
			while (!success && !canceled) {
				if (!rmdir(path)) { /* delete successful */
					success = TRUE;
					dir_unlog(dir_window_lines[linenum].dir);
					dir_remove_subdir(dir_window_lines[linenum].dir->parent,dir_window_lines[linenum].dir);
					dir_window_remove_line(linenum);
					if (linenum-dir_window_lines_top < height) {
						wmove(dir_window,linenum-dir_window_lines_top,0);
						wdeleteln(dir_window);
						if (dir_window_lines_top+height-1 < dir_window_lines_count)
							dir_window_draw_line(dir_window_lines_top+height-1);
						wrefresh(dir_window);
					}
				}
				else /* error deleting directory */
					switch(dir_window_prune_error_message(strerror(errno))) {
						case RESPONSE_CANCEL: canceled = TRUE; break;
						default:
					}
			}
		}
		else {
			/* selected dir - relog so that the files that have been deleted from it are
				 gone from memory */
			dir_unlog(dir_window_lines[dir_window_lines_cur_sel].dir);
			dir_log(dir_window_lines[dir_window_lines_cur_sel].dir);
			file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
			file_window_draw();
		}
		linenum--;
		if (read_key() == F_KEY(10)) canceled = TRUE;
	}
	nodelay(stdscr,FALSE);
	
	/* ask if the user wants to delete the selected directory as well */
	if (!canceled)
		dir_window_delete();

	status_line_clear();
	info_window_update();	

}


/*******************************************************************************
dir_window_graft_op~
*******************************************************************************/

char *dir_window_graft_op(char *old_filename, char *new_filename,
 												char *dest_path, char* new_name) {

	char *error_message = NULL;
	dir_info_struct *dir = dir_window_get_selected_dir();
	dir_info_struct *dest_dir;
	int dest_linenum;

	if (rename(old_filename,new_filename)) error_message = (char*) strerror(errno);
	else {
		/* move/rename successful */
		dest_dir = dir_find_from_path(dest_path,dir_window_rootdir);
		if (dest_dir && dest_dir->logged) {
			dir_window_remove_subdir_lines(dir_window_lines_cur_sel);
			dir_window_remove_line(dir_window_lines_cur_sel);
			wrefresh(dir_window);

			dir_remove_subdir(dir->parent,dir);
			dir_insert_subdir(dest_dir,dir);
			
			if (dir->next) { dest_linenum = dir_window_find_line_from_dir(dir->next); }
			else { dest_linenum = dir_window_find_line_from_dir(dest_dir)+1; }
			if (dest_linenum >= 0) {  /* this should always be the case...
 																	if it isn't, we may have problems
 																	with the dir still being logged */
				dir_window_add_line(dir,dest_linenum);
				dir_window_add_lines(dir,dest_linenum+1);
				dir_window_select(dest_linenum);
			}
			dir_window_draw();
		}
		else {
			dir_unlog(dir);
			dir_window_remove_subdir_lines(dir_window_lines_cur_sel);
			dir_window_remove_line(dir_window_lines_cur_sel);
			dir_window_draw();
			wrefresh(dir_window);
		}
	}
	
	return error_message;

}

/*******************************************************************************
dir_window_graft~
*******************************************************************************/

void dir_window_graft(void) {

	dir_info_struct *dir = dir_window_get_selected_dir();

	if (!dir->parent) status_line_error_message("Can't graft root dir");
	else
		reloc_file_op("GRAFT dir:",
								dir->name,
								dir_get_path(dir),
								get_parent_path(dir_get_path(dir)),
								&dir_window_graft_op);

}


/*******************************************************************************
dir_window_symlink_op~
*******************************************************************************/

char *dir_window_symlink_op(char *old_filename, char *new_filename,
														char *dest_path, char* new_name) {

	char *error_message = NULL;
	struct stat statbuf;
	int response;
	char *link_dest;
	
	response = status_line_get_response("Symbolic link type:",RESPONSE_ABSOLUTE+RESPONSE_RELATIVE+RESPONSE_CANCEL,FALSE);
	if (response != RESPONSE_CANCEL) {
		if (response == RESPONSE_ABSOLUTE) link_dest = strdup(old_filename);
		else link_dest = get_relative_filename(dest_path,old_filename);  /* note that new_filename needs to be absolute here */
		if (!lstat(new_filename,&statbuf) && unlink(new_filename))  /* delete file if it already exists */
 			error_message = (char*) strerror(errno);
		else if (symlink(link_dest,new_filename))  /* make link */
			error_message = (char*) strerror(errno);
		else {
			/* symlink successful; update file info in memory and file list */
			if (!file_window_update_file_and_update_list(new_filename))
				file_window_add_file_and_update_list(new_filename);
		}
		free(link_dest);
	}
	
	return error_message;

}


/*******************************************************************************
dir_window_symlink~
*******************************************************************************/

void dir_window_symlink(void) {

	dir_info_struct *dir = dir_window_get_selected_dir();

	reloc_file_op("SYMLINK dir:",
								dir->name,
								dir_get_path(dir),
								get_parent_path(dir_get_path(dir)),
								&dir_window_symlink_op);

}


/*******************************************************************************
dir_window_chmod~
*******************************************************************************/

void dir_window_chmod(void) {

	int max_display_length;
	char *display_name;
	dir_info_struct *dir = dir_window_get_selected_dir();
	char *error_message = NULL;
	char *input_string;

	unsigned short new_mode;
	struct mode_change *changes;
	
	max_display_length = get_window_width(command_window) - 21; /* 21 = strlen("ATTRIBUTES for dir: ")+1 */
		
	display_name = truncate_string(dir->name,max_display_length);

	wset_predefined_color(command_window,PANEL_COLOR);
  mvwaddch(command_window,1,18,':');
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);
	
	status_line_set_message("Enter new mode");
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	input_string = get_line(command_window,1,20,max_display_length,-1,"",NULL);
	status_line_clear();

	if (input_string) {
		changes = mode_compile(input_string,MODE_MASK_EQUALS | MODE_MASK_PLUS | MODE_MASK_MINUS);
		if (changes == MODE_INVALID) error_message = strdup("Invalid mode");
		else if (changes == MODE_MEMORY_EXHAUSTED) error_message = strdup("Virtual memory exhausted");
		else {
			new_mode = mode_adjust(dir->mode,changes);
			if (chmod(dir_get_path(dir),new_mode)) error_message = strerror(errno);
			mode_free(changes);
		}
	}
	

	if (error_message) status_line_error_message(error_message);
	free(display_name);
	
	dir_update(dir);
	info_window_update();
	
}

/*******************************************************************************
dir_window_chowngrp~

Common function used by both dir_window_chown and dir_window_chgrp.
*******************************************************************************/

void dir_window_chowngrp(int chgrp) {

	int max_display_length;
	char *display_name;
	dir_info_struct *dir = dir_window_get_selected_dir();
	char *error_message = NULL;
	char *input_string, *chown_string;
	uid_t uid = -1;
	gid_t gid = -1;
	
	max_display_length = get_window_width(command_window) - 21; /* 21 = strlen("ATTRIBUTES for dir: ")+1 */
		
	display_name = truncate_string(dir->name,max_display_length);

	wset_predefined_color(command_window,PANEL_COLOR);
  mvwaddch(command_window,1,18,':');
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);
	
	if (chgrp) status_line_set_message("Enter new group");
	else status_line_set_message("Enter new owner or owner:group");
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	input_string = get_line(command_window,1,20,max_display_length,-1,"",NULL);
	status_line_clear();
	

	if (input_string && strcmp(input_string,"")) {
		if (chgrp) {
			chown_string = malloc(strlen(input_string)+2);
			sprintf(chown_string,":%s",input_string);
		}
		else chown_string = strdup(input_string);
		error_message = parse_chown_string(chown_string,&uid,&gid);
		if (!error_message && ((uid != -1) || (gid != -1)) &&
#ifdef HAVE_LCHOWN /* in linux 2.2, chown follows symlinks so use lchown instead */
			lchown(dir_get_path(dir),uid,gid)
#else
			chown(dir_get_path(dir),uid,gid)
#endif
			) error_message = strerror(errno);
		free(input_string);
		free(chown_string);
	}

	if (error_message) status_line_error_message(error_message);
	free(display_name);
	
	dir_update(dir);
	info_window_update();

}

/*******************************************************************************
dir_window_chown~
*******************************************************************************/

void dir_window_chown(void) {

	dir_window_chowngrp(FALSE);

}

/*******************************************************************************
dir_window_chgrp~
*******************************************************************************/

void dir_window_chgrp(void) {

	dir_window_chowngrp(TRUE);

}

/*******************************************************************************
dir_window_set_tagged~
*******************************************************************************/

void dir_window_set_tagged(int tagged) {

	dir_info_struct *dir;
	file_info_struct *file;
	
	if (dir_window_lines_cur_sel < 0) return;
	if (dir_window_lines_cur_sel >= dir_window_lines_count) return;
	
	dir = dir_window_lines[dir_window_lines_cur_sel].dir;
	if (dir->logged) {
		file = dir->files;
		while (file) {
			file->tagged = tagged;
			file = file->next;
		}
		info_window_update_stats(TAGGED_FILES);
		wrefresh(info_window);
	}
	if (dir_window_lines_cur_sel < dir_window_lines_count-1)
		dir_window_select(dir_window_lines_cur_sel+1);
	else
		file_window_draw();

}

/*******************************************************************************
dir_window_delete~
*******************************************************************************/

void dir_window_delete() {

	int height, width;
	int max_display_length;
	char *display_name;
	dir_info_struct *dir = dir_window_lines[dir_window_lines_cur_sel].dir;
	char *error_message = NULL;

	
	getmaxyx(command_window,height,width);
	max_display_length = width - 23; /* 23 = strlen("DELETE sub-directory: ")+1 */
		
	if (strlen(dir->name) > max_display_length) {
		display_name = malloc(max_display_length+1);
		strncpy(display_name,dir->name,max_display_length);
	}
	else display_name = (char*) strdup(dir->name);

	/* prompt the user for the new directory name */
	werase(command_window);
	if (!dir->logged) { error_message = (char*) strdup("Directory not logged"); }
	else {
		wset_predefined_color(command_window,TEXT_COLOR);
		mvwprintw(command_window,0,22,"%s",display_name);
		wset_predefined_color(command_window,PANEL_COLOR);
																	
		mvwprintw(command_window,0,0,"DELETE sub-directory: ");
		wrefresh(command_window);
		if (!dir->parent) error_message = (char*) strdup("Cannot delete the root directory");
		else if (dir->subdirs || dir->files) error_message = (char*) strdup("Directory not empty");
		else {
			if (status_line_get_response("Delete this directory?",RESPONSE_YES+RESPONSE_NO+RESPONSE_CANCEL,TRUE)
					== RESPONSE_YES) {
		
				if (rmdir(dir_get_path(dir))) {
					error_message = error_message = (char*) strerror(errno);
				}
	
				if (!error_message) {
					/* update directory data & directory window */
					dir_window_remove_subdir_lines(dir_window_lines_cur_sel);
					dir_window_remove_line(dir_window_lines_cur_sel);
					if (dir_window_lines_cur_sel > 0) dir_window_lines_cur_sel--;
					dir_unlog(dir);
					dir_remove_subdir(dir->parent,dir);
					dir_free(dir);
					dir_window_draw();

					/* update file window & info window */
					file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
					file_window_draw();
					info_window_update();
				}
			}
		}
	}

	wrefresh(command_window);
	if (error_message) status_line_error_message(error_message);
 
		
	free(display_name);
	info_window_update();

		

}

/*******************************************************************************
dir_window_make~
*******************************************************************************/

void dir_window_make() {

	int height, width;
	int max_display_length;
	char *display_name, *parent_path, *path, *new_name;
	dir_info_struct *parent = dir_window_lines[dir_window_lines_cur_sel].dir;
	dir_info_struct *dir;
	char *error_message = NULL;
	struct stat statbuf;

	
	getmaxyx(command_window,height,width);
	max_display_length = width - 27; /* 27 = strlen("MAKE sub-directory under:")+1 */
		
	parent_path = dir_get_path(parent);
	if (strlen(parent_path) > max_display_length) {
		display_name = malloc(max_display_length+1);
		strncpy(display_name,parent_path,max_display_length);
	}
	else display_name = (char*) strdup(parent_path);

	/* prompt the user for the new directory name */
	wset_predefined_background_color(command_window,PANEL_COLOR);
	werase(command_window);
	if (!parent->logged) { error_message = (char*) strdup("Directory not logged"); }
	else {
		wset_predefined_color(command_window,TEXT_COLOR);
		mvwprintw(command_window,0,26,"%s",display_name);
		wset_predefined_color(command_window,PANEL_COLOR);
		mvwprintw(command_window,0,0,"MAKE sub-directory under: ");
		mvwprintw(command_window,1,0,"                      as: ");
		status_line_set_message("Enter new directory name");
		status_line_display_command_keys("{%E} ok  {F10} cancel");
		wrefresh(command_window);
	
		wset_predefined_color(command_window,TEXT_COLOR);
		new_name = get_line(command_window,1,26,max_display_length,-1,"",NULL);
		status_line_clear();
		if (new_name) {
			/* create the directory, if we can */
			if (strcmp(new_name,"")) {
				if (strchr(new_name,'/')) error_message = (char*) strdup("Invalid directory name (contains /)"); /* directory name contains '/' */
				else {
					path = (char*) malloc(strlen(parent_path)+1+strlen(new_name)+1);
					if (!strcmp(parent_path,"/")) sprintf(path,"/%s",new_name);
					else sprintf(path,"%s/%s",parent_path,new_name);
					free(parent_path);
					if (!lstat(path,&statbuf)) error_message = (char*) strdup("Dir exists");
					else if (mkdir(path,0777)) error_message = (char*) strerror(errno);
					
					if (!error_message && parent->logged) {
						dir = dir_read(path);
						dir_insert_subdir(parent,dir);
						dir_log(dir);
						dir_window_update_lines(dir_window_lines_cur_sel,TRUE);
					}
				}
			}
		}
	}

	wrefresh(command_window);
	if (error_message) status_line_error_message(error_message);
 
		
	free(display_name);
	info_window_update();

		

}

/*******************************************************************************
dir_window_rename_ext~
*******************************************************************************/

char *dir_window_rename_ext(char *line,key_type key) {

	if (key == TAB_KEY) {
		return (char*) strdup(dir_window_get_selected_dir()->name);
	}
	else return NULL;
}

/*******************************************************************************
dir_window_rename~
*******************************************************************************/

void dir_window_rename() {

	int height, width;
	int max_display_length;
	char *display_name, *new_name;
	dir_info_struct *dir = dir_window_lines[dir_window_lines_cur_sel].dir;
	char *error_message = NULL;
	char *old_dirname, *new_dirname;
	struct stat statbuf;

	
	getmaxyx(command_window,height,width);
	max_display_length = width - 15;
		
	display_name = truncate_string(dir->name,max_display_length);

	/* prompt the user for the new directory name */
	wset_predefined_background_color(command_window,PANEL_COLOR);
	werase(command_window);
	wset_predefined_color(command_window,TEXT_COLOR);
	mvwprintw(command_window,0,12,"%s",display_name);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"RENAME dir: ");
	mvwprintw(command_window,1,0,"        to: ");
	if (!dir->parent) { error_message = (char*) strdup("Cannot rename the root directory"); }
	else {
		status_line_set_message("Enter new directory name");
		status_line_display_command_keys("{%E} ok  {F10} cancel");
		wrefresh(command_window);
	
		wset_predefined_color(command_window,TEXT_COLOR);
		new_name = get_line(command_window,1,12,max_display_length,-1,"",dir_window_rename_ext);
		status_line_clear();
		if (new_name && strcmp(new_name,"")) {
			/* rename the directory, if we can */
			if (strcmp(new_name,dir->name)) {
				if (strchr(new_name,'/')) error_message = (char*) strdup("Invalid directory name"); /* directory name contains '/' */
				else {
					old_dirname = dir_get_path(dir);
					new_dirname = (char*) malloc(strlen(dir_get_path(dir->parent))+1+strlen(new_name)+1);
					if (!strcmp(dir_get_path(dir->parent),"/")) sprintf(new_dirname,"/%s",new_name);
					else sprintf(new_dirname,"%s/%s",dir_get_path(dir->parent),new_name);
					if (!lstat(new_dirname,&statbuf)) error_message = (char*) strdup("Dir exists");
					else if (rename(old_dirname,new_dirname)) error_message = (char*) strerror(errno);
					if (!error_message) {
						dir->name = new_name;
						dir_window_select(dir_window_lines_cur_sel);
					}
				}
			}
		}
	}

	if (error_message) status_line_error_message(error_message);
 
		
	free(display_name);
	info_window_update();
	
}

/*******************************************************************************
dir_window_attributes~

Asks the user what attributes they want to change (owner, group, mode) and
calls the appropariate function to change this attribute.
*******************************************************************************/

void dir_window_attributes(void) {

  char *display_line;
	int got_response;
	
	werase(command_window);
  wset_predefined_color(command_window,PANEL_COLOR);
	mvwaddstr(command_window,0,0,"ATTRIBUTES for dir: ");
  display_line = truncate_string(dir_window_get_selected_dir()->name,get_window_width(command_window)-21);
	wset_predefined_color(command_window,TEXT_COLOR);
  mvwaddstr(command_window,0,20,display_line);
	free(display_line);
	wrefresh(command_window);
	
	status_line_set_message("Change which attribute?");
	status_line_display_command_keys("{O}wner  {G}roup  {M}ode  {F10} cancel");
	
  do {
	  got_response = TRUE;
    switch(uppercase_key(read_key())) {
		  case 'O': status_line_clear(); dir_window_chowngrp(FALSE); break;
			case 'G': status_line_clear(); dir_window_chowngrp(TRUE); break;
			case 'M': status_line_clear(); dir_window_chmod(); break;
			case F_KEY(10): status_line_clear(); break;
			default: got_response = FALSE;
		}
	} while (!got_response);
  info_window_update();


}







































/*******************************************************************************
dir_window_logadd_path~

Logs and adds a directory given by path to the tree, and it's parents if
necessary, but only if all parents of the path that ARE in the tree are
logged. 
*******************************************************************************/

dir_info_struct *dir_window_logadd_path(char *path) {

	char *parent_path;
	dir_info_struct *new_dir = NULL, *parent_dir;

	new_dir = dir_find_from_path(path,dir_window_rootdir);
	if (new_dir) return new_dir;

	/* dir is not in dir_window's tree */
	parent_path = get_parent_path(path);
	if (!dir_find_from_path(parent_path,dir_window_rootdir))
		dir_window_logadd_path(parent_path);

	new_dir = dir_find_from_path(path,dir_window_rootdir);
	if (new_dir) {
		/* dir has been added to tree in above call to this function */
		dir_window_expand_to_path(path,FALSE);
		return new_dir;
	}

	if ((parent_dir = dir_find_from_path(parent_path,dir_window_rootdir))) {
		if (parent_dir->logged && (new_dir = dir_read(path))) {
			/* parent path in tree and logged, so add subdir under that */
			dir_insert_subdir(parent_dir,new_dir);
			dir_log(new_dir);
			dir_window_expand_to_path(path,FALSE);
		}
	}
	free(parent_path);
	
	return new_dir;


}

/*******************************************************************************
dir_window_get_selected_dir~
*******************************************************************************/

dir_info_struct *dir_window_get_selected_dir(void) {

	if ((dir_window_lines_cur_sel >= 0) && (dir_window_lines_cur_sel < dir_window_lines_count))
		return dir_window_lines[dir_window_lines_cur_sel].dir;
	else
		return FALSE;

}



/*******************************************************************************
dir_window_draw_line~
*******************************************************************************/

void dir_window_draw_line(int linenum) {

	int y = linenum - dir_window_lines_top;
	int hilighted = dir_window_lines_cur_sel == linenum;
	int depth = 0;
	int hilight_col, normal_col;
	dir_info_struct* parent;
	char *display_name;
	int height, width;
	
	getmaxyx(dir_window,height,width);

	if (linenum < 0) return;
	if (linenum >= dir_window_lines_count) return;
	if (linenum < dir_window_lines_top || linenum >= dir_window_lines_top+height) return;
	if (!dir_window_lines[linenum].dir) return;
	
	if (dir_window_mode == DIR_WINDOW_SELECT_MODE) {
		hilight_col = DIRS_COLOR;
		normal_col = HILIGHT_COLOR;
	}
	else {
		hilight_col = HILIGHT_COLOR;
		normal_col = DIRS_COLOR;
	}

	parent = dir_window_lines[linenum].dir;
	while ((parent = parent->parent)) depth++;

	wset_predefined_color(dir_window,normal_col);
	wset_predefined_background_color(dir_window,normal_col);

	wmove(dir_window,y,0);
	wclrtoeol(dir_window);

	/* draw + */
	wset_predefined_color(dir_window,normal_col);
	if (dir_window_lines[linenum].dir->logged) waddch(dir_window,' ');
	else waddch(dir_window,'+');
	
	/* set hilight or normal color */
	if (hilighted) wset_predefined_color(dir_window,hilight_col);
	else wset_predefined_color(dir_window,normal_col);

	if (!dir_window_lines[linenum].dir->parent) waddstr(dir_window,"/ "); /* root dir */
	else {
		/* draw name */
		wmove(dir_window,y,depth*3);
		waddch(dir_window,special_char[HLINE_CHAR]);
		display_name = truncate_string(dir_window_lines[linenum].dir->name,width-depth*3-1);
		waddstr(dir_window,display_name);
		if (depth*3+1+strlen(display_name) < width) waddch(dir_window,' ');
		free(display_name);
	
		wset_predefined_color(dir_window,normal_col);
		wmove(dir_window,y,depth*3-2);
		if (dir_window_lines[linenum].dir->next) waddch(dir_window,special_char[LTEE_CHAR]);
		else waddch(dir_window,special_char[LLCORNER_CHAR]);
		waddch(dir_window,special_char[HLINE_CHAR]);

		parent = dir_window_lines[linenum].dir;
		while ((parent = parent->parent) && (depth > 1)) {
			depth--;
			wmove(dir_window,y,depth*3-2);
			if (parent->next) waddch(dir_window,special_char[VLINE_CHAR]);
			else waddch(dir_window,' ');
			waddstr(dir_window,"  ");
		}
		wmove(dir_window,y,0);              /* put the cursor at the start of the line,
 																					for terminals that can't hide the cursor */
	}

}

/*******************************************************************************
dir_window_draw~
*******************************************************************************/

void dir_window_draw() {

	int width, height;
	int cur_line = 0;
	
	getmaxyx(dir_window,height,width);

	wset_predefined_background_color(dir_window,DIRS_COLOR);
	werase(dir_window);

	if (dir_window_lines_cur_sel >= 0) {
		if (dir_window_lines_cur_sel > dir_window_lines_top+height-1)
			dir_window_lines_top = dir_window_lines_cur_sel-height+1;
		else if (dir_window_lines_cur_sel < dir_window_lines_top)
			dir_window_lines_top = dir_window_lines_cur_sel;
	}

	wset_predefined_color(dir_window,TEXT_COLOR);
	for (cur_line = dir_window_lines_top;
 			(cur_line < dir_window_lines_count) && (cur_line-dir_window_lines_top <= height-1);
 			cur_line++) {
		dir_window_draw_line(cur_line);
	}

	wrefresh(dir_window);

}


/*******************************************************************************
dir_window_find_line_from_dir~

Returns the number of the line that corresponds to the specified directory.
If it can't be found, -1 is returned.
*******************************************************************************/

int dir_window_find_line_from_dir(dir_info_struct *dir) {

	int linenum = 0;
	int found = FALSE;
	
	while ((linenum < dir_window_lines_count) && !found) {
		if (dir_window_lines[linenum].dir == dir) found = TRUE;
		else linenum++;
	}

	if (found) return linenum;
	else return -1;

}

/*******************************************************************************
dir_window_update_lines~

Adds lines for subdirectories that exist under dir, but do not have lines.
This could be expanded to remove subdirectories that no longer exist, if need
be.
*******************************************************************************/

void dir_window_update_lines(int linenum, int update_screen) {

	dir_info_struct *dir, *subdir;

	if (linenum < 0) return;
	if (linenum >= dir_window_lines_count) return;
	if (!dir_window_lines[linenum].dir->logged) return;
	
	dir = dir_window_lines[linenum].dir;
	subdir = dir->subdirs;
	if ((linenum+1 < dir_window_lines_count) && !dir_check_if_parent(dir,dir_window_lines[linenum+1].dir)
			&& subdir) {
		/* no subdirectories are shown under the current directory, but they exist, so add all of them */
		dir_window_add_lines(dir,linenum+1);
	}
	linenum++;
	while ((linenum < dir_window_lines_count) && dir_check_if_parent(dir,dir_window_lines[linenum].dir) && subdir) {
		if (dir_window_lines[linenum].dir->parent == dir) {
			if (strcmp(dir_window_lines[linenum].dir->name,subdir->name)) {
				dir_window_add_line(subdir,linenum);
			}
			subdir = subdir->next;
		}
		linenum++;
	}
	while (subdir) {
		dir_window_add_line(subdir,linenum);
		linenum++;
		subdir = subdir->next;
	}
	if (update_screen) {
		dir_window_draw();
		wrefresh(dir_window);
	}

}

/*******************************************************************************
dir_window_add_line~

Adds/inserts a single line for dir.
*******************************************************************************/

void dir_window_add_line(dir_info_struct *subdir, int linenum) {

	dir_window_lines_allocated++;
	dir_window_lines = realloc(dir_window_lines,dir_window_lines_allocated*sizeof(dir_line_struct));
	
	if ((linenum < 0) || (linenum >= dir_window_lines_count)) {
		linenum = dir_window_lines_count;  /* append */
	}
	else {
		memmove(&dir_window_lines[linenum+1],
						&dir_window_lines[linenum],
						(dir_window_lines_count-linenum)*sizeof(dir_line_struct));
		if (dir_window_lines_cur_sel >= linenum) dir_window_lines_cur_sel++;
	}
	
	dir_window_lines[linenum].dir = subdir;
	dir_window_lines_count++;

}

/*******************************************************************************
dir_window_add_lines~

Adds/inserts lines for all subdirs of dir.
*******************************************************************************/

void dir_window_add_lines(dir_info_struct *dir, int linenum) {

	dir_info_struct* subdir;
	int subdir_count = 0;

	if (!dir->subdirs) return;
		
	subdir = dir->subdirs;
	while (subdir) {
		subdir = subdir->next;
		subdir_count++;
	}

	dir_window_lines_allocated += subdir_count;
	dir_window_lines = realloc(dir_window_lines,dir_window_lines_allocated*sizeof(dir_line_struct));

	if ((linenum < 0) || (linenum >= dir_window_lines_count)) {
		linenum = dir_window_lines_count;  /* append */
	}
	else {
		memmove(&dir_window_lines[linenum+subdir_count],
						&dir_window_lines[linenum],
						(dir_window_lines_count-linenum)*sizeof(dir_line_struct));
	}
	if (dir_window_lines_cur_sel >= linenum) dir_window_lines_cur_sel += subdir_count;

	dir_window_lines_count += subdir_count;

	subdir = dir->subdirs;
	while (subdir) {
		dir_window_lines[linenum].dir = subdir;
		linenum++;
		subdir = subdir->next;
	}

	subdir = dir->subdirs;
	while (subdir) {
		dir_window_add_lines(subdir,dir_window_find_line_from_dir(subdir)+1);
		subdir = subdir->next;
	}

}

/*******************************************************************************
dir_window_remove_line~

Note: does not change the current selection, which may leave it outside the
range of existing lines. Doe not update screen.
*******************************************************************************/

void dir_window_remove_line(int linenum) {

	if (linenum < 0) return;
	if (linenum >= dir_window_lines_count) return;

	if (linenum < dir_window_lines_count-1)
		memmove(&dir_window_lines[linenum],
						&dir_window_lines[linenum+1],
						(dir_window_lines_count-linenum-1)*sizeof(dir_line_struct));
	dir_window_lines_allocated--;
	dir_window_lines = realloc(dir_window_lines,dir_window_lines_allocated*sizeof(dir_line_struct));
	dir_window_lines_count--;

}


/*******************************************************************************
dir_window_remove_subdir_lines~
*******************************************************************************/

void dir_window_remove_subdir_lines(int linenum) {

	dir_info_struct *dir = dir_window_lines[linenum].dir;
	int cur_line;
	int dirs_to_remove = 0;
	
	cur_line = linenum+1;
	while ((cur_line < dir_window_lines_count) &&
 				dir_check_if_parent(dir,dir_window_lines[cur_line].dir))
/*         (dir_tree_lines[cur_line].tree->parent == tree))*/
		cur_line++;
	dirs_to_remove = cur_line-linenum-1;
	
	if (dirs_to_remove > 0) {
		memmove(&dir_window_lines[linenum+1],
						&dir_window_lines[linenum+1+dirs_to_remove],
						(dir_window_lines_count-linenum-dirs_to_remove-1)*sizeof(dir_line_struct));
		dir_window_lines_count -= dirs_to_remove;
		dir_window_lines_allocated -= dirs_to_remove;
		dir_window_lines = realloc(dir_window_lines,dir_window_lines_allocated*sizeof(dir_line_struct));
	}
	dir_window_draw();
 	
}

/*******************************************************************************
dir_window_select~
*******************************************************************************/

void dir_window_select(int linenum) {

	int width, height;
	int old_sel = dir_window_lines_cur_sel;

	getmaxyx(dir_window,height,width);

	dir_window_lines_cur_sel = linenum;

	if (!((old_sel > dir_window_lines_top+height-1) || (old_sel < dir_window_lines_top))) {
		dir_window_draw_line(old_sel);
	}

	if ((dir_window_lines_cur_sel > dir_window_lines_top+height-1) || (dir_window_lines_cur_sel < dir_window_lines_top))
		dir_window_draw();
	dir_window_draw_line(dir_window_lines_cur_sel);
	wrefresh(dir_window);

	if (dir_window_mode == DIR_WINDOW_NORMAL_MODE) {
		file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
		file_window_draw();
	}
	
	info_window_update_local_stats();
	info_window_update_path(dir_get_path(dir_window_get_selected_dir()));
	if (dir_window_mode == DIR_WINDOW_SELECT_MODE)
		info_window_update_input_path(dir_get_path(dir_window_get_selected_dir()));
	else info_window_update_disk_stats();
	wrefresh(info_window);
	
}

/*******************************************************************************
dir_window_init~
*******************************************************************************/

void dir_window_init(void) {

	int screen_height, screen_width;

	getmaxyx(stdscr,screen_height,screen_width);
	dir_window = newwin_force(screen_height-bottom_height-minifl_height-4,screen_width-info_width-3,2,1);
	
	dir_window_mode = DIR_WINDOW_NORMAL_MODE;

	dir_window_lines_count = 0;
	dir_window_lines_allocated = 0;
	dir_window_lines = NULL;
	dir_window_lines_top = 0;
	dir_window_lines_cur_sel = -1;

	dir_window_rootdir = dir_read("/");
	

	dir_window_lines_allocated = 1;
	dir_window_lines_count = 1;
	dir_window_lines = malloc(sizeof(dir_line_struct));
	dir_window_lines[0].dir = dir_window_rootdir;
	
	dir_log(dir_window_rootdir);

	dir_window_add_lines(dir_window_rootdir,1);

}

/*******************************************************************************
dir_window_expand_dir~
*******************************************************************************/

void dir_window_expand_dir(int linenum, int update) {

	int log_error;

	if (dir_window_lines[linenum].dir->logged) return;
	
	log_error = dir_log(dir_window_lines[linenum].dir);
	if (log_error) status_line_error_message(strerror(log_error));
	else {
		dir_window_add_lines(dir_window_lines[linenum].dir,linenum+1);
		if ((current_mode == DIR_TREE_MODE) && update) {
			dir_window_draw();
			wrefresh(dir_window);
			if (dir_window_mode == DIR_WINDOW_NORMAL_MODE) {
				file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
				file_window_draw();
			}
			info_window_update();
		}
	}

}

/*******************************************************************************
dir_window_expand_branch~
*******************************************************************************/

void dir_window_expand_branch(int linenum) {

	dir_info_struct *dir = dir_window_lines[linenum].dir;
	int canceled = FALSE;
	int height, width;
	
	getmaxyx(command_window,height,width);

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,2,0,"Logging directories...");
	mvwprintw(command_window,2,width-11,"F10  cancel");
	wset_predefined_color(command_window,TEXT_COLOR);
	mvwprintw(command_window,2,width-11,"F10");
	wrefresh(command_window);
	
	nodelay(stdscr,TRUE);
	while ((linenum < dir_window_lines_count) &&
 				((dir == dir_window_lines[linenum].dir) || dir_check_if_parent(dir,dir_window_lines[linenum].dir)) &&
 				!canceled) {
		if (read_key() == F_KEY(10)) canceled = TRUE;
		else {
			dir_window_expand_dir(linenum,FALSE);
			linenum++;
			info_window_update_stats(ALL_FILES);
			info_window_update_stats(MATCHING_FILES);
			info_window_update_stats(TAGGED_FILES);
			wrefresh(info_window);
		}
	}
	nodelay(stdscr,FALSE);
	if (current_mode == DIR_TREE_MODE) {
		dir_window_draw();
		wrefresh(dir_window);
		if (dir_window_mode == DIR_WINDOW_NORMAL_MODE) {
			file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
			file_window_draw();
		}
		info_window_update();
	}
 				

}

/*******************************************************************************
dir_window_collapse_dir~
*******************************************************************************/

void dir_window_collapse_dir(int linenum) {

	if (!dir_window_lines[linenum].dir->logged)
		return;

	dir_unlog(dir_window_lines[linenum].dir);
	dir_window_remove_subdir_lines(linenum);
	wrefresh(dir_window);

	if (dir_window_mode == DIR_WINDOW_NORMAL_MODE) {
		file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
		file_window_draw();
	}

	info_window_update();

}

/*******************************************************************************
dir_window_sel_parent~

Selects the parent directory of the current one.
*******************************************************************************/

void dir_window_sel_parent(void) {

	int linenum = dir_window_lines_cur_sel;
	
	if (linenum <= 0) return;
	
	while (linenum > 0 &&
				 !dir_check_if_parent(dir_window_lines[linenum].dir,dir_window_get_selected_dir()))
		linenum--;
		
	dir_window_select(linenum);

}

/*******************************************************************************
dir_window_sel_up~

Move the selection up one line, if it is not already at the top.
*******************************************************************************/

static void dir_window_sel_up(void) {

	if (dir_window_lines_cur_sel > 0) {
		if (dir_window_lines_cur_sel == dir_window_lines_top) {
			wmove(dir_window,0,0);
			winsdelln(dir_window,1);
			dir_window_lines_top--;
		}
		dir_window_select(dir_window_lines_cur_sel-1);
	}
}

/*******************************************************************************
dir_window_sel_down~

Move the selection down one line, if it is not already at the bottom.
*******************************************************************************/

static void dir_window_sel_down(void) {

	int width, height;

	if (dir_window_lines_cur_sel < dir_window_lines_count-1) {
		getmaxyx(dir_window,height,width);
		if (dir_window_lines_cur_sel == dir_window_lines_top+height-1) {
			wmove(dir_window,0,0);
			wdeleteln(dir_window);
			dir_window_lines_top++;
		}
		dir_window_select(dir_window_lines_cur_sel+1);
	}

}

/*******************************************************************************
dir_window_sel_pgup~

Move the selection up one page, if it is not already at the top.
*******************************************************************************/

static void dir_window_sel_pgup(void) {

	int width, height;

	if (dir_window_lines_cur_sel > 0) {
		getmaxyx(dir_window,height,width);
		if (dir_window_lines_cur_sel < height)
			dir_window_select(0);
		else dir_window_select(dir_window_lines_cur_sel-height+1);
	}

}

/*******************************************************************************
dir_window_sel_pgdn~

Move the selection down one page, if it is not already at the bottom.
*******************************************************************************/

static void dir_window_sel_pgdn(void) {

	int width, height;

	if (dir_window_lines_cur_sel < dir_window_lines_count-1) {
		getmaxyx(dir_window,height,width);
		if (dir_window_lines_cur_sel+height-1 > dir_window_lines_count-1)
			dir_window_select(dir_window_lines_count-1);
		else dir_window_select(dir_window_lines_cur_sel+height-1);
	}

}

/*******************************************************************************
dir_window_sel_top~

Move the selection to the top, if it is not already there.
*******************************************************************************/

static void dir_window_sel_top(void) {

	if (dir_window_lines_cur_sel > 0) {
		dir_window_select(0);
	}

}

/*******************************************************************************
dir_window_sel_bottom~

Move the selection to the top, if it is not already there.
*******************************************************************************/

static void dir_window_sel_bottom(void) {

	if (dir_window_lines_cur_sel < dir_window_lines_count-1) {
		dir_window_select(dir_window_lines_count-1);
	}

}

/*******************************************************************************
dir_window_enter~
*******************************************************************************/

static void dir_window_enter(void) {

	if (!dir_window_lines[dir_window_lines_cur_sel].dir->logged) {
		dir_window_expand_dir(dir_window_lines_cur_sel,TRUE);
	}
	else {
		file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
		current_mode = FILE_LIST_MODE;
		if (!config.file_window.small_file_window)
			file_window_size = FILE_WINDOW_NORMAL_SIZE;
		file_window_activate();
	}

}

/*******************************************************************************
dir_window_branch~
*******************************************************************************/

static void dir_window_branch(void) {

	if (dir_window_lines[dir_window_lines_cur_sel].dir->logged &&
			file_window_set_items_from_branch(dir_window_lines[dir_window_lines_cur_sel].dir)) {
		current_mode = FILE_LIST_MODE;
		file_window_size = FILE_WINDOW_NORMAL_SIZE;
		file_window_activate();
	}

}

/*******************************************************************************
dir_window_process_key~

Returns 0 if key was not processed.
*******************************************************************************/

int dir_window_process_key(key_type key) {

	int finished = FALSE;
	int processed = TRUE;

	switch (dir_window_mode) {
		case DIR_WINDOW_NORMAL_MODE:
			/* normal mode (as opposed to selection mode */
			switch (key) {
				case     LEFT_KEY: dir_window_sel_parent(); break;
				case       UP_KEY: dir_window_sel_up(); break;
				case    RIGHT_KEY: /* same as DOWN_KEY */
				case          ' ': /* same as DOWN_KEY */
				case     DOWN_KEY: dir_window_sel_down(); break;
				case     PGUP_KEY: dir_window_sel_pgup(); break;
				case     PGDN_KEY: dir_window_sel_pgdn(); break;
				case     HOME_KEY: dir_window_sel_top(); break;
				case      END_KEY: dir_window_sel_bottom(); break;
				case          '+': dir_window_expand_dir(dir_window_lines_cur_sel,TRUE);  break; /* expand dir */
				case          '-': dir_window_collapse_dir(dir_window_lines_cur_sel);  break; /* collapse dir */
				case          '*': dir_window_expand_branch(dir_window_lines_cur_sel); break; /* expand branch */
				case    ENTER_KEY: dir_window_enter();                                 break;
				case          'A': dir_window_attributes();                            break; /* directory attributes */
				case          'B': dir_window_branch();                                break;
				case          'D': dir_window_delete();                                break; /* delete directory */
				case          'L': dir_window_symlink();                               break; /* symLink directory */
				case          'M': dir_window_make();                                  break; /* make directory */
				case          'R': dir_window_rename();                                break; /* rename directory */
				case          'T': dir_window_set_tagged(TRUE);                        break; /* tag/untag all files in directory */
				case          'U': dir_window_set_tagged(FALSE);                       break; /* tag/untag all files in directory */
				case ALT_KEY('G'): dir_window_graft();                                 break; /* graft dir */
				case ALT_KEY('P'): dir_window_prune();                                 break; /* prune directory */
				default: processed = FALSE;
			}
			break;
		default:
			/* selection mode - only allow limited functions */
			switch (key) {
				case    UP_KEY: dir_window_sel_up(); break;
				case  DOWN_KEY: dir_window_sel_down(); break;
				case  PGUP_KEY: dir_window_sel_pgup(); break;
				case  PGDN_KEY: dir_window_sel_pgdn(); break;
				case  HOME_KEY: dir_window_sel_top(); break;
				case   END_KEY: dir_window_sel_bottom(); break;
				case       '+': dir_window_expand_dir(dir_window_lines_cur_sel,TRUE);  break; /* expand dir */

				case ENTER_KEY: finished = TRUE; break; /* ENTER */
				case F_KEY(10):
					finished = TRUE;
					dir_window_lines_cur_sel = -1;
					break;
				default: processed = FALSE;
			}
	}
	
	if (finished) file_window_activate(); /* should remove finished altogether */
	
	return processed;
	
}

/*******************************************************************************
dir_window_activate~
*******************************************************************************/

void dir_window_activate(void) {

	current_mode = DIR_TREE_MODE;

	if (dir_window_lines_cur_sel < 0) dir_window_select(0);
	dir_window_draw();

	wrefresh(dir_window);

	if (dir_window_mode == DIR_WINDOW_NORMAL_MODE) {
		file_window_set_items_from_dir(dir_window_lines[dir_window_lines_cur_sel].dir);
		file_window_draw();
	}
	info_window_update();
					
}

/*******************************************************************************
dir_window_select_loop~

This funciton is the loop for directory selection. It is called by
dir_window_do_select() (below) and performs a similar function to main_loop()
(in main.c) and dir_window_process_key().
*******************************************************************************/

static void dir_window_select_loop(void) {

	key_type key;
	int finished = FALSE;

	while (!finished) {
		key = uppercase_key(read_key());
		switch (key) {
			case    UP_KEY: dir_window_sel_up(); break;
			case  DOWN_KEY: dir_window_sel_down(); break;
			case  PGUP_KEY: dir_window_sel_pgup(); break;
			case  PGDN_KEY: dir_window_sel_pgdn(); break;
			case  HOME_KEY: dir_window_sel_top(); break;
			case   END_KEY: dir_window_sel_bottom(); break;
			case       '+': dir_window_expand_dir(dir_window_lines_cur_sel,TRUE);  break; /* expand dir */
			case ENTER_KEY: finished = TRUE; break; /* ENTER */
			case F_KEY(10):
				finished = TRUE;
				dir_window_lines_cur_sel = -1;
				break;
		}
	}
}

/*******************************************************************************
dir_window_do_select~

Goes into "selection mode", letting the user select a directory. This mode
has limited functions - only things like expand, etc. and no actual directory
operations. Once the user presses enter, the dir selected is returned to the
calling function.
initial_dir is the directory that is to be initally selected.
This mode is generally used when the user pressed F2 to select a destination
directory.
*******************************************************************************/

dir_info_struct *dir_window_do_select(dir_info_struct *initial_dir) {

	dir_info_struct *old_top_dir, *old_selected_dir;
	int linenum;
	int screen_height, screen_width;
	int old_mode = current_mode;
	int selected_linenum = -1;

	linenum = dir_window_find_line_from_dir(initial_dir);
	if (linenum < 0) linenum = dir_window_lines_cur_sel;
	
	getmaxyx(stdscr,screen_height,screen_width);
	current_mode = DIR_TREE_MODE;

	/* resize dir_window */
	if (file_window_size == FILE_WINDOW_MINI_SIZE) file_window_hide_separator();
	resize_window(&dir_window,screen_height-bottom_height-3,screen_width-info_width-3);
	wset_predefined_background_color(dir_window,HILIGHT_COLOR);
	werase(dir_window);
	wset_predefined_color(dir_window,HILIGHT_COLOR);
	window_border(dir_window);
	wrefresh(dir_window);
	resize_window(&dir_window,screen_height-bottom_height-5,screen_width-info_width-5);
	mvwin(dir_window,3,2);

	/* save current dir_window status */
	old_top_dir = dir_window_lines[dir_window_lines_top].dir;
	old_selected_dir = dir_window_lines[dir_window_lines_cur_sel].dir;
	
	/* do the selection */
	dir_window_lines_cur_sel = linenum;
	dir_window_mode = DIR_WINDOW_SELECT_MODE;
	dir_window_activate();
	
	dir_window_select_loop();

	dir_window_mode = DIR_WINDOW_NORMAL_MODE;
	
	/* here, selected_linenum will be set to -1 if the user canceled the selection. */
	selected_linenum = dir_window_lines_cur_sel;
	
	/* change dir_window back to normal size */
	if (file_window_size == FILE_WINDOW_MINI_SIZE) file_window_show_separator();
	getmaxyx(stdscr,screen_height,screen_width);  /* just in case user has resized window while selecting dir */
	resize_window(&dir_window,screen_height-bottom_height-minifl_height-4,screen_width-info_width-3);
	mvwin(dir_window,2,1);
	
	/* restore dir_window status */
	/* - this may not work properly if the user has collapsed dirs, but we don't let them do this */
	dir_window_lines_top = dir_window_find_line_from_dir(old_top_dir);
	dir_window_lines_cur_sel = dir_window_find_line_from_dir(old_selected_dir);
	current_mode = old_mode;

	if ((selected_linenum >= 0) && (selected_linenum < dir_window_lines_count))
		return dir_window_lines[selected_linenum].dir;
	else return NULL;

}


/*******************************************************************************
dir_window_expand_to_path~
*******************************************************************************/

int dir_window_expand_to_path(char *path, int update) {

	int cur_char;
	int failed = FALSE;
	char *dir_name;
	char *path_so_far;
	int dir_start = 1;
	int dir_line = 0;
	
	dir_info_struct *dir;

	if ((dir = dir_find_from_path(path,dir_window_rootdir)) &&
			(dir_window_find_line_from_dir(dir) >= 0) &&
			dir->logged)
		/* this dir is already expanded to */
		return TRUE;

	if (!(path = get_full_path(path,NULL)) || (dir_window_lines_count <= 0))
		/* can't get full path or there are no lines in dir window */
		return FALSE;

	dir_name = (char*) malloc(strlen(path)+1);
	path_so_far = (char*) malloc(strlen(path)+1);
	path_so_far[0] = path[0];
	dir_window_update_lines(0,update);  /* root */
	for (cur_char = 1; (cur_char <= strlen(path)) && !failed; cur_char++) {
		if ((path[cur_char] == '/') || (cur_char == strlen(path))) {
			if (cur_char > dir_start) {
				path_so_far[cur_char] = 0;
				dir_name[cur_char-dir_start] = 0;

				if ((dir = dir_find_from_path(path_so_far,dir_window_rootdir))) {
					dir_line = dir_window_find_line_from_dir(dir);
					if (dir_line >= 0) {
						dir_window_expand_dir(dir_line,update);
						dir_window_update_lines(dir_line,update);
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
	free(dir_name);
	free(path_so_far);
	
	return !failed;
	

}

/*******************************************************************************
dir_window_set_path~
*******************************************************************************/

int dir_window_set_path(char *path) {

	static char current_path[PATH_MAX];
	dir_info_struct *dir;
	int dir_line;

	getcwd(current_path,PATH_MAX);

	if ((path = get_full_path(path,current_path)) &&
			dir_window_expand_to_path(path,FALSE) &&
			((dir = dir_find_from_path(path,dir_window_rootdir)) >= 0) &&
			((dir_line = dir_window_find_line_from_dir(dir)) >= 0)) {
		dir_window_select(dir_line);
		return TRUE;
	}
	else return FALSE;

}

/*******************************************************************************
dir_window_set_initial_path~

Sets the intial path for the directory window (either root, home or current
directory depending on confiruation).
*******************************************************************************/

void dir_window_set_initial_path(void) {

	char *home;
	char cwd[PATH_MAX];

	switch (config.dir_window.initial_dir) {
		case CONFIG_DIR_WINDOW_INITAL_DIR_CURRENT:
			getcwd(cwd,PATH_MAX);
			dir_window_set_path(cwd);			
			break;
		case CONFIG_DIR_WINDOW_INITAL_DIR_HOME:
			if ((home = getenv("HOME"))) dir_window_set_path(home);
			/* otherwise, don't set path - will default to root */
			break;
		default:
	}

}




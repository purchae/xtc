/*  file_window.c - routines to manipulate the file window and file operations
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


/* file_window_move_with_paths~ */
#define file_window_move_with_paths()\
	reloc_tagged_files_op("Duplicate paths and MOVE all tagged files as",\
												dir_get_path(dir_window_get_selected_dir()),\
												&file_window_move_tagged_op,TRUE);

/* file_window_symlink_with_paths~ */
#define file_window_symlink_with_paths()\
	reloc_tagged_files_op("Duplicate paths and SYMLINK all tagged files as",\
												dir_get_path(dir_window_get_selected_dir()),\
												&file_window_symlink_tagged_op,TRUE);



/* file_window_copy_with_paths~ */
#define file_window_copy_with_paths() \
	reloc_tagged_files_op("Duplicate paths and COPY all tagged files as",\
												dir_get_path(dir_window_get_selected_dir()),\
												&file_window_copy_tagged_op,TRUE);


/* file_window_move_tagged~ */
#define file_window_move_tagged() \
	reloc_tagged_files_op("MOVE all tagged files",\
												dir_get_path(dir_window_get_selected_dir()),\
												&file_window_move_tagged_op,FALSE);

/* file_window_symlink_tagged~ */
#define file_window_symlink_tagged() \
	reloc_tagged_files_op("SYMLINK all tagged files",\
												dir_get_path(dir_window_get_selected_dir()),\
												&file_window_symlink_tagged_op,FALSE);

/* file_window_copy_tagged~ */
#define file_window_copy_tagged() \
	reloc_tagged_files_op("COPY all tagged files",\
												dir_get_path(dir_window_get_selected_dir()),\
												&file_window_copy_tagged_op,FALSE);



enum {
  NAME_CRITERIA,
	EXT_CRITERIA,
	DATE_CRITERIA,
	SIZE_CRITERIA
};

struct _file_window_item_struct {
	file_info_struct* file;
};
typedef struct _file_window_item_struct file_window_item_struct;


int file_window_sort_criteria;
int file_window_sort_desc;
int file_window_sort_path;
int file_window_sort_case;

WINDOW* file_window;
file_window_item_struct* file_window_items;
int file_window_items_count;
int file_window_items_cur_sel;
int file_window_items_top;
int file_window_size;
int file_window_list_type;
dir_info_struct *file_window_branch_topdir;
int file_window_update_item(int itemno);
int file_window_cols = 1;

WINDOW* minifl_separator_window;


int file_window_dir_in_list(dir_info_struct *dir);
static void file_window_draw_item(int itemno);
void file_window_add_item(file_info_struct *file);
int file_window_find_itemno_from_file(file_info_struct *file);
void file_window_remove_item(int itemno);
int file_window_items_compare(const void* param1, const void* param2);







/*                misc. file/file window related functions                    */

/*******************************************************************************
file_window_change_display_format~
*******************************************************************************/

void file_window_change_display_format(void) {

	switch(file_display_format) {
		case NORMAL_FORMAT: file_display_format = DATE_FORMAT; break;
		case DATE_FORMAT: file_display_format = ATTRIBUTES_FORMAT; break;
		case ATTRIBUTES_FORMAT: file_display_format = NORMAL_FORMAT; break;
	}
	file_window_set_cols();
	file_window_draw();

}

/*******************************************************************************
file_window_count_tagged_files~
*******************************************************************************/

int file_window_count_tagged_files(int show_error) {

  int itemno;
	int num_files = 0;

	for (itemno = 0; itemno < file_window_items_count; itemno++)
		if (file_window_items[itemno].file->tagged) num_files++;
  if ((num_files == 0) && show_error) {
	  werase(command_window);
		wrefresh(command_window);
	  status_line_error_message("There are no tagged files to operate with");
		info_window_update();
  }

  return num_files;

}

/*******************************************************************************
file_window_set_filespec~
*******************************************************************************/

void file_window_set_filespec(void) {

	char *filespec_string;
	int max_display_length;

	max_display_length = get_window_width(command_window) - 11;

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"Filespec: ");
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);	
  status_line_set_message("Enter new filespec");
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	
	if ((filespec_string = get_line(command_window,0,10,max_display_length,-1,"",NULL))) {
		filespec_list_set(filespec_list,filespec_string);
		if (file_window_list_type == FILE_WINDOW_BRANCH_LIST)
			file_window_set_items_from_branch(file_window_branch_topdir);
		else
			file_window_set_items_from_dir(dir_window_get_selected_dir());
		if (current_mode == FILE_LIST_MODE) {
			if (file_window_items_count == 0) {
				current_mode = DIR_TREE_MODE;
				file_window_resize_mini();
				dir_window_draw();
			}
			else file_window_select(0);
		}
		file_window_draw();
	  free(filespec_string);
	}
	
	status_line_clear();	
	info_window_update();

}













/*                           tagged file operations                           */

/*******************************************************************************
file_window_chmod_tagged~
*******************************************************************************/

void file_window_chmod_tagged(void) {

	int linenum;
	file_info_struct *file;
	int canceled = FALSE;
	int num_files = 0, num_files_done = 0;
	char *display_name;
	char *error_message, *message;
	int max_display_length;
	char *input_string;

	unsigned short new_mode;
	struct mode_change *changes;

  num_files = file_window_count_tagged_files(FALSE);

	max_display_length = get_window_width(command_window) - 22;
		
	wset_predefined_color(command_window,PANEL_COLOR);
  mvwaddch(command_window,1,19,':');
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);

	/* get new mode */
	status_line_set_message("Enter new mode");
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	input_string = get_line(command_window,1,21,max_display_length,-1,"",NULL);
	status_line_clear();

	if (input_string && strcmp(input_string,"")) {  /* mode entered */
		changes = mode_compile(input_string,MODE_MASK_EQUALS | MODE_MASK_PLUS | MODE_MASK_MINUS);
		if (changes == MODE_INVALID) error_message = strdup("Invalid mode");
		else if (changes == MODE_MEMORY_EXHAUSTED) error_message = strdup("Virtual memory exhausted");
		else {
			status_line_display_command_keys("{F10} cancel");
			nodelay(stdscr,TRUE);

      wmove(command_window,0,0);
			wclrtoeol(command_window);
			wset_predefined_color(command_window,PANEL_COLOR);
			wprintw(command_window,"ATTRIBUTES for file: ");
			wset_predefined_color(command_window,TEXT_COLOR);

			linenum = 0;
			while ((linenum < file_window_items_count) && !canceled) {
				file = file_window_items[linenum].file;
				if (file->tagged) {
					file_window_select(linenum);

					wmove(command_window,0,21);
					wclrtoeol(command_window);
					display_name = truncate_string(file->name,get_window_width(command_window)-22);
					wprintw(command_window,"%s",display_name);
					free(display_name);
					wrefresh(command_window);

					if (read_key() == F_KEY(10)) canceled = TRUE;
					new_mode = mode_adjust(file->mode,changes);

					if (!S_ISLNK(file->mode)) {
						if (chmod(file_get_full_name(file),new_mode)) {
							/* error chmoding file */
							error_message = strerror(errno);
							nodelay(stdscr,FALSE);
							message = malloc(strlen(error_message)+8);
							sprintf(message,"Error: %s",error_message);
							if (config.misc.beep) beep();
							switch(status_line_get_response(message,RESPONSE_OK+RESPONSE_CANCEL,FALSE)) {
								case RESPONSE_CANCEL: canceled = TRUE;
								default:
							}
							free(message);
							free(error_message);
							linenum++;
							nodelay(stdscr,TRUE);
						}
						else {
							/* chmod successful */      
							file_window_update_item(file_window_items_cur_sel);
						}
					}
					num_files_done++;
					status_line_progress_bar_set(num_files_done*100/num_files);
				}
				linenum++;
			}
			nodelay(stdscr,FALSE);

			mode_free(changes);
		}
		free(input_string);
	}
	file_window_draw();
	status_line_clear();
	info_window_update();

}


/*******************************************************************************
file_window_chowngrp_tagged~

Common function used by both file_window_chown_tagged() and
file_window_chgrp_tagged()
*******************************************************************************/

void file_window_chowngrp_tagged(int chgrp) {

	int linenum;
	file_info_struct *file;
	int canceled = FALSE;
	int num_files = 0, num_files_done = 0;
	char *display_name;
	char *error_message, *message;
	int max_display_length;
	char *input_string, *chown_string;
	int uid = -1, gid = -1;

  num_files = file_window_count_tagged_files(FALSE);

	max_display_length = get_window_width(command_window) - 22;
		
	wset_predefined_color(command_window,PANEL_COLOR);
  mvwaddch(command_window,1,19,':');
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);


	if (chgrp) status_line_set_message("Enter new group");
	else status_line_set_message("Enter new owner or owner:group");
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	input_string = get_line(command_window,1,21,max_display_length,-1,"",NULL);
	status_line_clear();

	if (input_string && (error_message = parse_chown_string(input_string,&uid,&gid))) {
		status_line_error_message(error_message);
		free(error_message);
	}
	else if (uid != -1 || gid != -1) {

		if (chgrp) {
			chown_string = malloc(strlen(input_string)+2);
			sprintf(chown_string,":%s",input_string);
		}
		else chown_string = strdup(input_string);

		status_line_display_command_keys("{F10} cancel");
		nodelay(stdscr,TRUE);

     wmove(command_window,0,0);
		wclrtoeol(command_window);
		wset_predefined_color(command_window,PANEL_COLOR);
		wprintw(command_window,"ATTRIBUTES for file: ");
		wset_predefined_color(command_window,TEXT_COLOR);

		linenum = 0;
		while ((linenum < file_window_items_count) && !canceled) {
			file = file_window_items[linenum].file;
			if (file->tagged) {
				file_window_select(linenum);

				wmove(command_window,0,21);
				wclrtoeol(command_window);
				display_name = truncate_string(file->name,get_window_width(command_window)-22);
				wprintw(command_window,"%s",display_name);
				free(display_name);
				wrefresh(command_window);

				if (read_key() == F_KEY(10)) canceled = TRUE;
				else if ((error_message = do_chown(file_get_full_name(file),chown_string))) {
					/* error chowning file */
					nodelay(stdscr,FALSE);
					message = malloc(strlen(error_message)+8);
					sprintf(message,"Error: %s",error_message);
					if (config.misc.beep) beep();
					switch(status_line_get_response(message,RESPONSE_OK+RESPONSE_CANCEL,FALSE)) {
						case RESPONSE_CANCEL: canceled = TRUE;
						default:
					}
					free(message);
					free(error_message);
					nodelay(stdscr,TRUE);
				}
				else 
					/* chown successful */      
					file_window_update_item(file_window_items_cur_sel);
				num_files_done++;
				status_line_progress_bar_set(num_files_done*100/num_files);
			}
			linenum++;
		}
		nodelay(stdscr,FALSE);


		free(input_string);
		free(chown_string);
	}
	file_window_draw();
	status_line_clear();
	info_window_update();

}

/*******************************************************************************
file_window_tagged_op_setup_dest_path~
Works out the destination filename, depending on several factors including paths
which is either NO_PATHS, RELATIVE_PATHS or ABSOLUTE_PATHS.
This function is only intended for use by file_window_copy_tagged_op,
file_window_move_tagged_op and file_window_symlink_tagged_op.
*******************************************************************************/

void file_window_tagged_op_setup_dest_path(int paths, char *dest_path,
 																					char **dest_filename, char **error_message,
 																					file_info_struct *file) {

	char *actual_dest_path, *relative_name;
	char *base_path = dir_get_path(dir_window_get_selected_dir());
	char *src_filename = file_get_full_name(file);
	dir_info_struct *new_dir;

	if (paths == NO_PATHS)
		*dest_filename = join_path_and_name(dest_path,file->name);
	else {
		relative_name = get_relative_filename((paths == RELATIVE_PATHS ? base_path : "/"),src_filename);
		*dest_filename = join_path_and_name(dest_path,relative_name);
		actual_dest_path = get_parent_path(*dest_filename);
		*error_message = make_path(actual_dest_path);
		if (!*error_message) {
			/* add dir to dir_window if it isn't already there and parents are logged */
			new_dir = dir_find_from_path(actual_dest_path,dir_window_rootdir);
			if (!new_dir)
				new_dir = dir_window_logadd_path(actual_dest_path);
		}
		free(actual_dest_path);
		free(relative_name);
	}

}

/*******************************************************************************
file_window_symlink_tagged_op~
*******************************************************************************/

void file_window_symlink_tagged_op(char *dest_path, int auto_replace, int paths) {

	int linenum;
	file_info_struct *file;
	int canceled = FALSE;
	int allow_symlink;
	size_type total_files = 0, total_files_done = 0;
	char *src_filename, *dest_filename;
	char *error_message = NULL;
	struct stat statbuf;
	int relative = FALSE;
	char *link_dest;
	char *actual_dest_path;

  total_files = file_window_count_tagged_files(FALSE);

	switch(status_line_get_response("Symbolic link type:",RESPONSE_ABSOLUTE+RESPONSE_RELATIVE+RESPONSE_CANCEL,FALSE)) {
		case RESPONSE_ABSOLUTE: relative = FALSE; break;
		case RESPONSE_RELATIVE: relative = TRUE; break;
		default: return;
	}

	linenum = 0;
	while ((linenum < file_window_items_count) && !canceled) {
		file = file_window_items[linenum].file;


		if (file->tagged) {
			allow_symlink = TRUE;
			file_window_select(linenum);
			wrefresh(file_window);

			src_filename = file_get_full_name(file);

			file_window_tagged_op_setup_dest_path(paths,dest_path,&dest_filename,
																						&error_message,file);

			command_window_show_from_to("SYMLINK",src_filename,dest_filename);

			if (!auto_replace && !lstat(dest_filename,&statbuf)) {
				switch(ask_replace(12,src_filename,dest_filename)) {
					case RESPONSE_CANCEL: allow_symlink = FALSE; canceled = TRUE; break;
					case RESPONSE_YES: break;
					default: allow_symlink = FALSE;
				}
			}
			if (allow_symlink) {

				if (relative) {
					actual_dest_path = get_parent_path(dest_filename);
					link_dest = get_relative_filename(actual_dest_path,src_filename);  /* note that dest_filename needs to be absolute here */
					free(actual_dest_path);
				}
				else link_dest = strdup(src_filename);

				if (lstat(src_filename,&statbuf)) error_message = strerror(errno);
				else if (!lstat(dest_filename,&statbuf) && unlink(dest_filename)) error_message = strerror(errno);
				else if (symlink(link_dest,dest_filename)) error_message = strerror(errno);

				if (error_message) {
					status_line_error_message(error_message);
					error_message = NULL;
					switch(status_line_get_response("Options:",RESPONSE_RETRYFILE+RESPONSE_SKIPFILE+RESPONSE_CANCEL,FALSE)) {
						case RESPONSE_RETRYFILE: linenum--; break;
						case RESPONSE_CANCEL: canceled = TRUE; break;
						default:
					}
				}
				else {
					/* symlink successful; update file info in memory and file list */
					if (!file_window_update_file_and_update_list(dest_filename))
						file_window_add_file_and_update_list(dest_filename);
				}
				free(link_dest);
			}
			total_files_done++;
			if (total_files > 0) status_line_progress_bar_set(total_files_done*100/total_files);
			linenum++;
			free(src_filename);
			free(dest_filename);
		}
		else linenum++;
	}
	file_window_draw();
	

	if (error_message) status_line_error_message(error_message);
	
	status_line_clear();
	info_window_update();
		

}


/*******************************************************************************
file_window_move_tagged_op~
*******************************************************************************/

void file_window_move_tagged_op(char *dest_path, int auto_replace, int paths) {

	int linenum;
	file_info_struct *file;
	int canceled = FALSE;
	int allow_move;
	size_type total_bytes = 0, bytes_done = 0;
	float multiplier;
	char *src_filename, *dest_filename;
	char *error_message = NULL;

	struct stat statbuf;
	
	dir_info_struct *dest_dir;
	file_info_struct *dest_file;
	int dest_file_linenum;

	for (linenum = 0; linenum < file_window_items_count; linenum++)
		if (file_window_items[linenum].file->tagged) total_bytes += file_window_items[linenum].file->size;

	multiplier = 100.0/total_bytes;
	
	linenum = 0;
	while ((linenum < file_window_items_count) && !canceled) {
		file = file_window_items[linenum].file;


		if (file->tagged) {
			allow_move = TRUE;
			file_window_select(linenum);
			wrefresh(file_window);

			src_filename = file_get_full_name(file);

			file_window_tagged_op_setup_dest_path(paths,dest_path,&dest_filename,
																						&error_message,file);

			command_window_show_from_to("MOVING",src_filename,dest_filename);

			if (!auto_replace && !lstat(dest_filename,&statbuf)) {
				switch(ask_replace(9,src_filename,dest_filename)) {
					case RESPONSE_CANCEL: allow_move = FALSE; canceled = TRUE; break;
					case RESPONSE_YES: break;
					default: allow_move = FALSE;
				}
			}
			if (allow_move) {
				error_message = do_move(src_filename,dest_filename,&bytes_done,total_bytes);
				if (error_message && !strcmp(error_message,"canceled")) {
					error_message = NULL;
					canceled = TRUE;
				}
				if (!error_message) {
					/* move successful; update file info in memory and file list */
					dest_dir = dir_find_from_path(dest_path,dir_window_rootdir);
 				
					/* remove the existing file from memory */
					dir_remove_file(file->dir,file);
					file_free(file);

					if ((dest_file = file_find_from_full_name(dest_filename,dir_window_rootdir))) {

						/* dest file already exists in memory, so remove it */
						if ((dest_file_linenum = file_window_find_itemno_from_file(dest_file)) >= 0) {

							/* file is in file list, so remove it */
							file_window_remove_item(dest_file_linenum);
							if (linenum > dest_file_linenum) linenum--;
						}
						dir_remove_file(dest_file->dir,dest_file);
						file_free(dest_file);
					}
					
					/* add the dest file to memory */
					
					if ((dest_file = file_read_and_add(dest_filename)) &&
										file_window_dir_in_list(dest_file->dir)) {
						file_window_items[linenum].file = dest_file;
						linenum++;
					}
					else
						file_window_remove_item(linenum);
				}
				if (error_message) {
					status_line_error_message(error_message);
					error_message = NULL;
					switch(status_line_get_response("Options:",RESPONSE_RETRYFILE+RESPONSE_SKIPFILE+RESPONSE_CANCEL,FALSE)) {
						case RESPONSE_SKIPFILE: linenum++; break;
						case RESPONSE_CANCEL: canceled = TRUE; break;
						default:
					}
				}
			}
			else {
				bytes_done += file->size;
				if (total_bytes > 0) status_line_progress_bar_set((int)(bytes_done*multiplier));
				linenum++;
			}
			free(src_filename);
			free(dest_filename);
		}
		else linenum++;
	}
	file_window_draw();

	if (error_message) status_line_error_message(error_message);
	
	status_line_clear();
	info_window_update();

}

/*******************************************************************************
file_window_copy_tagged_op~
*******************************************************************************/

void file_window_copy_tagged_op(char *dest_path, int auto_replace, int paths) {

	int itemno;
	file_info_struct *file = NULL;
	int canceled = FALSE;
	int allow_copy;
	size_type total_bytes = 0, bytes_done = 0;
	char *src_filename, *dest_filename;
	char *error_message = NULL;
	float multiplier;

	struct stat statbuf;

	for (itemno = 0; itemno < file_window_items_count; itemno++)
		if (file_window_items[itemno].file->tagged) total_bytes += file_window_items[itemno].file->size;

	multiplier = 100.0/total_bytes;
	
	itemno = 0;
	while ((itemno < file_window_items_count) && !canceled) {
		file = file_window_items[itemno].file;
		if (file->tagged) {
			allow_copy = TRUE;
			file_window_select(itemno);
			wrefresh(file_window);

			src_filename = file_get_full_name(file);

			file_window_tagged_op_setup_dest_path(paths,dest_path,&dest_filename,
																						&error_message,file);

			command_window_show_from_to("COPYING",src_filename,dest_filename);
			
			if (!error_message) {
				if (!auto_replace && !lstat(dest_filename,&statbuf)) {
					switch(ask_replace(9,src_filename,dest_filename)) {
						case RESPONSE_CANCEL: allow_copy = FALSE; canceled = TRUE; break;
						case RESPONSE_YES: break;
						default: allow_copy = FALSE;
					}
				}
				if (allow_copy)
					error_message = do_copy(src_filename,dest_filename,&bytes_done,total_bytes);
				else {
					bytes_done += file->size;
					if (total_bytes > 0) status_line_progress_bar_set((int)(bytes_done*multiplier));
				}
			}

			if (error_message) {
				if (!strcmp(error_message,"canceled")) {
					canceled = TRUE;
					error_message = NULL;
				}
				else {
					status_line_error_message(error_message);
					error_message = NULL;
					switch(status_line_get_response("Options:",RESPONSE_RETRYFILE+RESPONSE_SKIPFILE+RESPONSE_CANCEL,FALSE)) {
						case RESPONSE_RETRYFILE: itemno--; break;
						case RESPONSE_CANCEL: canceled = TRUE; break;
						default:
					}
				}
			}
			else {
				/* copy successful; update file info in memory and file list */
				if (!file_window_update_file_and_update_list(dest_filename))
					file_window_add_file_and_update_list(dest_filename);
			}
			
			itemno++;
			/* if the dest filename has been added to the list, we may
				 need to move down another line */
			if (file == file_window_items[itemno].file) itemno++;
			free(src_filename);
			free(dest_filename);
		}
		else itemno++;
	}
	file_window_draw();

	if (error_message) status_line_error_message(error_message);
	
	status_line_clear();
	info_window_update();
		

}

/*******************************************************************************
file_window_delete_tagged~
*******************************************************************************/

void file_window_delete_tagged() {

	int linenum;
	int confirm_for_each = TRUE;
	file_info_struct *file;
	int canceled = FALSE;
	int allow_delete;
	int num_files = 0, num_files_done = 0;
	char *display_name;
	char *message;

  num_files = file_window_count_tagged_files(TRUE);
	if (num_files <= 0) return;
	
	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"DELETE all tagged files");
	wrefresh(command_window);

	switch(status_line_get_response("Confirm delete for each file?",RESPONSE_YES+RESPONSE_NO+RESPONSE_CANCEL,TRUE)) {
		case RESPONSE_NO: confirm_for_each = FALSE; break;
		case RESPONSE_CANCEL: canceled = TRUE;
		default: confirm_for_each = TRUE; break;
	}
	
	if (!confirm_for_each) {
		status_line_display_command_keys("{F10} cancel");
		nodelay(stdscr,TRUE);
	}


	linenum = 0;
	while ((linenum < file_window_items_count) && !canceled) {
		file = file_window_items[linenum].file;
		if (file->tagged) {
			allow_delete = TRUE;
			file_window_select(linenum);

			werase(command_window);
			wset_predefined_color(command_window,PANEL_COLOR);
			wprintw(command_window,"DELETING: ");
			wset_predefined_color(command_window,TEXT_COLOR);
			display_name = truncate_string(file->name,get_window_width(command_window)-11);
			wprintw(command_window,"%s",display_name);
			free(display_name);
			wrefresh(command_window);

			if (confirm_for_each) {
				switch(status_line_get_response("Delete this file?",RESPONSE_YES+RESPONSE_NO+RESPONSE_CANCEL,TRUE)) {
					case RESPONSE_YES: break;
					case RESPONSE_CANCEL: allow_delete = FALSE; canceled = TRUE; break;
					default: allow_delete = FALSE;
				}
			}
			if (allow_delete) {
				if (!confirm_for_each && (read_key() == F_KEY(10))) canceled = TRUE;
				else if (!unlink(file_get_full_name(file))) { /* delete successful */
					dir_remove_file(file->dir,file);
					file_free(file);
					file_window_remove_item(linenum);
				}
				else { /* error deleting file */
					if (!confirm_for_each) nodelay(stdscr,FALSE);
					message = malloc(strlen(strerror(errno))+8);
					sprintf(message,"Error: %s",strerror(errno));
					if (config.misc.beep) beep();
					switch(status_line_get_response(message,RESPONSE_OK+RESPONSE_CANCEL,FALSE)) {
						case RESPONSE_CANCEL: canceled = TRUE;
						default:
					}
					free(message);
					linenum++;
					if (!confirm_for_each) nodelay(stdscr,TRUE);
				}
				info_window_update_stats(ALL_FILES);
				info_window_update_stats(MATCHING_FILES);
				info_window_update_stats(TAGGED_FILES);
				info_window_update_disk_stats();
			}
			else linenum++;
			num_files_done++;
			if (!confirm_for_each) status_line_progress_bar_set(num_files_done*100/num_files);
		}
		else linenum++;
	}
	if (!confirm_for_each) nodelay(stdscr,FALSE);
	
	file_window_draw();
	status_line_clear();
	info_window_update();
		

}

/*******************************************************************************
file_window_set_tagged_all~
*******************************************************************************/

void file_window_set_tagged_all(int tagged) {

	int linenum;
	
	for (linenum = 0; linenum < file_window_items_count; linenum++) {
		file_window_items[linenum].file->tagged = tagged;
	}
	file_window_draw();
	info_window_update_stats(TAGGED_FILES);
	wrefresh(info_window);

}








































/*                                file operations                             */

/*******************************************************************************
file_window_run_external~
*******************************************************************************/

void file_window_run_external(char *command) {

	char *filename = file_get_full_name(file_window_get_selected_file());
	char *filename_param; /* filename to be passed as parameter */
	char *command_line;

	/* prepare terminal for running another program */	
	noraw();
	tputs(clear_screen,1,putchar);
	refresh();
	show_cursor();  /* must do this before the fork otherwise the hide_cursor won't work later on */

	if (strchr(filename,'\'')) {
		/* the filename contains one or more ' characters, so we have to make all of
		   these '\'' in filename_param so that the paramater is passed properly */
		int fpos = 0, fppos = 0;
		filename_param = malloc(strlen(filename)*4+1); /* max length needed - if filename is all '''''s */
		while (filename[fpos] != 0) {
			if (filename[fpos] == '\'') { /* ' encountered; replace with '\'' */
				filename_param[fppos++] = '\'';
				filename_param[fppos++] = '\\';
				filename_param[fppos++] = '\'';
				filename_param[fppos++] = '\'';
				fpos++;
			}
			else filename_param[fppos++] = filename[fpos++]; /* some other character */
		}
		filename_param[fppos] = 0;
	}
	else filename_param = strdup(filename);
	free(filename);
	
	
	command_line = malloc(strlen(command)+2+strlen(filename_param)+2);
	sprintf(command_line,"%s '%s'",command,filename_param);
	free(filename_param);
	system(command_line); /* run the external program */
	free(command_line);
	
	/* restore terminal */
	hide_cursor();
	wrefresh(curscr);
	raw();        /* immediate char return */
	noecho();     /* no immediate echo */
	keypad(stdscr,TRUE);

	file_window_update_item(file_window_items_cur_sel);
	info_window_update();

}


/*******************************************************************************
file_window_view~
*******************************************************************************/

void file_window_view(void) {

	if (config.external_programs.viewer && strlen(config.external_programs.viewer) > 0)
		file_window_run_external(config.external_programs.viewer);
	else file_window_run_external(VIEWER);
	
}

/*******************************************************************************
file_window_edit~
*******************************************************************************/

void file_window_edit(void) {

	if (config.external_programs.editor && strlen(config.external_programs.editor) > 0)
		file_window_run_external(config.external_programs.editor);
	else file_window_run_external(EDITOR);


}


/*******************************************************************************
file_window_copy_op~
*******************************************************************************/

char *file_window_copy_op(char *old_filename, char *new_filename,
 															char *dest_path, char* new_name) {

	char *error_message = NULL;
	struct stat statbuf;
	int canceled = FALSE;
	
	size_type total_bytes = 0, bytes_done = 0;

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"COPYING: ");
	mvwprintw(command_window,1,0,"     to: ");
	status_line_display_command_keys("{F10} cancel");
	wset_predefined_color(command_window,TEXT_COLOR);
	mvwprintw(command_window,0,9,truncate_string(old_filename,get_window_width(command_window)-10));
	mvwprintw(command_window,1,9,truncate_string(new_filename,get_window_width(command_window)-10));
	wrefresh(command_window);
	
	if (lstat(old_filename,&statbuf)) error_message = strdup(strerror(errno));
	else {
		total_bytes = statbuf.st_size;
		error_message = do_copy(old_filename,new_filename,&bytes_done,total_bytes);
	}

	if (error_message && !strcmp(error_message,"canceled")) {
		canceled = TRUE;
		error_message = NULL;
	}

	if (!error_message && !canceled) {
		/* copy successful; update file info in memory and file list */
		if (!file_window_update_file_and_update_list(new_filename)) {
			file_window_add_file_and_update_list(new_filename);
		}
	}

	status_line_clear();
	info_window_update();

	return error_message;

}

/*******************************************************************************
file_window_copy~
*******************************************************************************/

void file_window_copy() {

	file_info_struct *file = file_window_get_selected_file();

	reloc_file_op("COPY file:",
								file->name,
								file_get_full_name(file),
								dir_get_path(file->dir),
								&file_window_copy_op);
								
}

/*******************************************************************************
file_window_symlink_op~
*******************************************************************************/

char *file_window_symlink_op(char *old_filename, char *new_filename,
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
file_window_symlink~
*******************************************************************************/

void file_window_symlink() {

	file_info_struct *file = file_window_get_selected_file();

	reloc_file_op("SYMLINK file:",
								file->name,
								file_get_full_name(file),
								dir_get_path(file->dir),
								&file_window_symlink_op);

}

/*******************************************************************************
file_window_rename_ext~
*******************************************************************************/

char *file_window_rename_ext(char *line,key_type key) {

	if (key == TAB_KEY) {
		return (char*) strdup(file_window_get_selected_file()->name);
	}
	else return NULL;
}

/*******************************************************************************
file_window_rename~
*******************************************************************************/

void file_window_rename() {

	int max_display_length;
	char *display_name, *new_name;
	file_info_struct *file = file_window_get_selected_file();
	char *error_message = NULL;
	char *old_filename, *new_filename;
	struct stat statbuf;
	
	max_display_length = get_window_width(command_window) - 14; /* 14 = strlen("RENAME file: ")+1 */
		
	display_name = truncate_string(file->name,max_display_length);

	/* prompt the user for the new filename */
	wset_predefined_background_color(command_window,PANEL_COLOR);
	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"RENAME file: ");
	mvwprintw(command_window,1,0,"         to: ");
	wset_predefined_color(command_window,TEXT_COLOR);
	mvwprintw(command_window,0,13,"%s",display_name);
	wrefresh(command_window);
	status_line_set_message("Enter file specification");
	status_line_display_command_keys("{TAB} copy name  {%E} ok  {F10} cancel");

	new_name = get_line(command_window,1,13,max_display_length,-1,"",file_window_rename_ext);
	status_line_clear();
	if (new_name && strcmp(new_name,"")) {
		/* rename the file, if we can */
		if (strcmp(new_name,file->name)) {
			if (strchr(new_name,'/')) error_message = (char*) strdup("Invalid filename (contains /)"); /* filename contains '/' */
			else {
				old_filename = file_get_full_name(file);
				new_filename = (char*) malloc(strlen(dir_get_path(file->dir))+1+strlen(new_name)+1);
				if (!strcmp(dir_get_path(file->dir),"/")) sprintf(new_filename,"/%s",new_name);
				else sprintf(new_filename,"%s/%s",dir_get_path(file->dir),new_name);
				if (!lstat(new_filename,&statbuf)) error_message = (char*) strdup("File exists");
				else if (rename(old_filename,new_filename)) error_message = strerror(errno);
			}
		}
	
		if (!error_message) {
			file->name = new_name;
			file_window_select(file_window_items_cur_sel);
		}
	}

	if (error_message) status_line_error_message(error_message);
		
	free(display_name);
	info_window_update();
	
}



/*******************************************************************************
file_window_move_op~
*******************************************************************************/

char *file_window_move_op(char *old_filename, char *new_filename,
													char *dest_path, char* new_name) {

	char *error_message = NULL;
	file_info_struct *file = file_window_get_selected_file();
	dir_info_struct *dest_dir;
	int canceled = FALSE;
	size_type total_bytes = 0, bytes_done = 0;
	struct stat statbuf;

	if (!stat(old_filename,&statbuf)) total_bytes = statbuf.st_size;

	error_message = do_move(old_filename,new_filename,&bytes_done,total_bytes);
	if (error_message && !strcmp(error_message,"canceled")) {  /* copy canceled */
		error_message = NULL;
		canceled = TRUE;
	}
	if (!error_message && !canceled) {
		/* move/rename successful */
		if (strcmp(file->name,new_name)) {
			free(file->name);
			file->name = (char*) strdup(new_name);
		}
		dest_dir = dir_find_from_path(dest_path,dir_window_rootdir);
		if (!(dest_dir && dest_dir->logged && (file_window_list_type == FILE_WINDOW_BRANCH_LIST) &&
				(dir_check_if_parent(file_window_branch_topdir,dest_dir) || (file_window_branch_topdir == dest_dir)))
				&& (file->dir != dest_dir)) {
			file_window_remove_item(file_window_items_cur_sel);
			/* Otherwise, we are in branch mode and the destination dir is known and logged and is
 				a child of the top directory in the branch,
 				OR the file has been "moved" to the same directory (user specified . as destination
 				but differnet filename),
 				so we leave the file in the list */
		}
		if (file->dir != dest_dir) {
			dir_remove_file(file->dir,file);
			if (dest_dir && dest_dir->logged) dir_add_file(dest_dir,file);
		}
		info_window_update();
	}
	status_line_clear();
	
	return error_message;

}

/*******************************************************************************
file_window_move~
*******************************************************************************/

void file_window_move() {

	file_info_struct *file = file_window_get_selected_file();

	reloc_file_op("MOVE file:",
								file->name,
								file_get_full_name(file),
								dir_get_path(file->dir),
								&file_window_move_op);
								
}


/*******************************************************************************
file_window_delete~
*******************************************************************************/

void file_window_delete() {

	int max_display_length;
	char *display_name;
	file_info_struct *file = file_window_get_selected_file();
	char *error_message = NULL;
	
	max_display_length = get_window_width(command_window) - 14; /* 14 = strlen("DELETE file: ")+1 */
		
	display_name = truncate_string(file->name,max_display_length);

	/* confirm deletion */
	wset_predefined_background_color(command_window,PANEL_COLOR);
	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"DELETE file: ");
	wset_predefined_color(command_window,TEXT_COLOR);
	mvwprintw(command_window,0,13,"%s",display_name);
	wrefresh(command_window);
	
	if (status_line_get_response("Delete this file?",RESPONSE_YES+RESPONSE_NO+RESPONSE_CANCEL,TRUE) == RESPONSE_YES) {
		if (unlink(file_get_full_name(file))) error_message = strerror(errno);
		else {
			dir_remove_file(file->dir,file);
			free(file);
			file_window_remove_item(file_window_items_cur_sel);
		}
	}
	info_window_update();

	if (error_message) status_line_error_message(error_message);
		
	free(display_name);


}


/*******************************************************************************
file_window_chowngrp~

Common function used by both file_window_chown and file_window_chgrp
*******************************************************************************/

void file_window_chowngrp(int chgrp) {

	int max_display_length;
	char *display_name;
	file_info_struct *file = file_window_get_selected_file();
	char *error_message = NULL;
	char *input_string, *chown_string;
	uid_t uid = -1;
	gid_t gid = -1;
	
	max_display_length = get_window_width(command_window) - 22; /* 22 = strlen("ATTRIBUTES for file: ")+1 */
		
	display_name = truncate_string(file->name,max_display_length);

	wset_predefined_color(command_window,PANEL_COLOR);
  mvwaddch(command_window,1,19,':');
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);
	
	if (chgrp) status_line_set_message("Enter new group");
	else status_line_set_message("Enter new owner or owner:group");
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	input_string = get_line(command_window,1,21,max_display_length,-1,"",NULL);
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
			lchown(file_get_full_name(file),uid,gid)
#else
			chown(file_get_full_name(file),uid,gid)
#endif
			) error_message = strerror(errno);
		free(input_string);
		free(chown_string);
	}

	if (error_message) status_line_error_message(error_message);
	free(display_name);
	
	file_window_update_item(file_window_items_cur_sel);
	info_window_update();  

}

/*******************************************************************************
file_window_chmod~
*******************************************************************************/

void file_window_chmod(void) {

	int max_display_length;
	char *display_name;
	file_info_struct *file = file_window_get_selected_file();
	char *error_message = NULL;
	char *input_string;

	unsigned short new_mode;
	struct mode_change *changes;
	
	max_display_length = get_window_width(command_window) - 22; /* 22 = strlen("ATTRIBUTES for file: ")+1 */
		
	display_name = truncate_string(file->name,max_display_length);

	wset_predefined_color(command_window,PANEL_COLOR);
  mvwaddch(command_window,1,19,':');
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);
	
	if (S_ISLNK(file->mode)) error_message = strdup("Can't chmod symbolic link");
	else {
		status_line_set_message("Enter new mode");
		status_line_display_command_keys("{%E} ok  {F10} cancel");
		input_string = get_line(command_window,1,21,max_display_length,-1,"",NULL);
		status_line_clear();

		if (input_string) {
			changes = mode_compile(input_string,MODE_MASK_EQUALS | MODE_MASK_PLUS | MODE_MASK_MINUS);
			if (changes == MODE_INVALID) error_message = strdup("Invalid mode");
			else if (changes == MODE_MEMORY_EXHAUSTED) error_message = strdup("Virtual memory exhausted");
			else {
				new_mode = mode_adjust(file->mode,changes);
				if (chmod(file_get_full_name(file),new_mode)) error_message = strerror(errno);
				mode_free(changes);
			}
		}
	}
	

	if (error_message) status_line_error_message(error_message);
	free(display_name);
	
	file_window_update_item(file_window_items_cur_sel);
	info_window_update();  

}



/*******************************************************************************
file_window_set_tagged~

Tags or untags the currently selected file.
*******************************************************************************/

void file_window_set_tagged(int tagged) {


	if (file_window_items_cur_sel >= 0) {
		file_window_get_selected_file()->tagged = tagged;
		if (file_window_items_cur_sel < file_window_items_count-1)
			file_window_select(file_window_items_cur_sel+1);
		else file_window_select(file_window_items_cur_sel);
		info_window_update_stats(TAGGED_FILES);
		wrefresh(info_window);
	}


}

/*******************************************************************************
file_window_attributes~

Asks the user what attributes they want to change (owner, group, mode) and
calls the appropariate function to change this attribute.
*******************************************************************************/

void file_window_attributes(int tagged) {

  char *display_line;
	int got_response;
	
	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	if (tagged) {
		if (file_window_count_tagged_files(TRUE) <= 0) return;
		mvwaddstr(command_window,0,0,"ATTRIBUTES for all tagged files");
	}
	else {
		mvwaddstr(command_window,0,0,"ATTRIBUTES for file: ");
		display_line = file_get_display_line(file_window_get_selected_file(),
																				 get_window_width(command_window)-22,
																				 ATTRIBUTES_FORMAT);
		wset_predefined_color(command_window,TEXT_COLOR);
		mvwaddstr(command_window,0,21,display_line);
		free(display_line);
	}
	wrefresh(command_window);
	
	status_line_set_message("Change which attribute?");
	status_line_display_command_keys("{O}wner  {G}roup  {M}ode  {F10} cancel");
	
	do {
		got_response = TRUE;
		switch(uppercase_key(read_key())) {
			case 'O': status_line_clear();
								if (tagged) file_window_chowngrp_tagged(FALSE);
								else file_window_chowngrp(FALSE);
								break;
		  case 'G': status_line_clear();
			          if (tagged) file_window_chowngrp_tagged(TRUE);
								else file_window_chowngrp(TRUE);
								break;
		  case 'M': status_line_clear();
			          if (tagged) file_window_chmod_tagged();
								else file_window_chmod();
								break;
			case F_KEY(10): status_line_clear(); break;
			default: got_response = FALSE;
		}
	} while (!got_response);
  info_window_update();


}

/*******************************************************************************
file_window_change_sort_order~
*******************************************************************************/

void file_window_change_sort_order(void) {

  int finished = FALSE;
	int canceled = FALSE;
	int new_criteria = file_window_sort_criteria;
	int new_desc = file_window_sort_desc;
  int new_path = file_window_sort_path;
  int new_case = file_window_sort_case;

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwaddstr(command_window,0,0,"SORT FILE DISPLAY BY: Name  Ext  Date & Time  Size");
	mvwaddstr(command_window,1,0,"                      Order               Path        Case sensitive");
	wset_predefined_color(command_window,TEXT_COLOR);
  mvwaddch(command_window,0,22,'N');
  mvwaddch(command_window,0,28,'E');
  mvwaddch(command_window,0,33,'D');
  mvwaddch(command_window,0,46,'S');
  mvwaddch(command_window,1,22,'O');
  mvwaddch(command_window,1,42,'P');
  mvwaddch(command_window,1,54,'C');
	status_line_set_message("Enter sort option");
	status_line_display_command_keys("{%E} ok  {F10} cancel");

	wset_predefined_color(command_window,PANEL_COLOR);
	if (new_desc) mvwaddstr(command_window,1,28,"(descending)");
	else mvwaddstr(command_window,1,28,"(ascending) ");
	if (new_path) mvwaddstr(command_window,1,47,"(on) ");
	else mvwaddstr(command_window,1,47,"(off)");
	if (new_case) mvwaddstr(command_window,1,69,"(yes)");
	else mvwaddstr(command_window,1,69,"(no) ");
	wmove(command_window,0,0);
	wrefresh(command_window);

	while (!finished) {
		switch(uppercase_key(read_key())) {
		  case 'N': new_criteria = NAME_CRITERIA; finished = TRUE; break;
			case 'E': new_criteria = EXT_CRITERIA; finished = TRUE; break;
			case 'D': new_criteria = DATE_CRITERIA; finished = TRUE; break;
			case 'S': new_criteria = SIZE_CRITERIA; finished = TRUE; break;
			case 'O': new_desc = !new_desc;
								if (new_desc) mvwaddstr(command_window,1,28,"(descending)");
								else mvwaddstr(command_window,1,28,"(ascending) ");
								wmove(command_window,0,0);
								wrefresh(command_window);
								break;
			case 'P': new_path = !new_path;
								if (new_path) mvwaddstr(command_window,1,47,"(on) ");
								else mvwaddstr(command_window,1,47,"(off)");
								wmove(command_window,0,0);
								wrefresh(command_window);
								break;
			case 'C': new_case = !new_case;
								if (new_case) mvwaddstr(command_window,1,69,"(yes)");
								else mvwaddstr(command_window,1,69,"(no) ");
								wmove(command_window,0,0);
								wrefresh(command_window);
								break;
		  case F_KEY(10): canceled = TRUE; finished = TRUE; break;
			case ENTER_KEY: finished = TRUE; break;
		  default:
		}
	}
	status_line_clear();
	
	if (!canceled) {
	  if (file_window_items_cur_sel >= 0) file_window_select(0);
		file_window_sort_criteria = new_criteria;
		file_window_sort_desc = new_desc;
		file_window_sort_path = new_path;
		file_window_sort_case = new_case;
		qsort(file_window_items,file_window_items_count,sizeof(file_window_item_struct),file_window_items_compare);
		file_window_draw();
	}
	info_window_update();

}



































/*                     functions dealing with the file window                 */

/*******************************************************************************
file_window_set_cols~

Works out the number of columns based on the longest filename in the list
*******************************************************************************/

void file_window_set_cols(void) {

	int itemno;
	int longest = 0, length;
	int width = get_window_width(file_window);

	if (file_window_items_count <= 0 ||
			file_display_format != NORMAL_FORMAT) {
		file_window_cols = 1;
		return;
	}
	
	if (config.file_window.cols > 0) {
		file_window_cols = config.file_window.cols;
		return;
	}
	
	for (itemno = 0; itemno < file_window_items_count; itemno++) {
		length = strlen(file_window_items[itemno].file->name);
		if (length > longest) longest = length;
	}

	file_window_cols = width/(longest+5);
	if (file_window_cols < 1) file_window_cols = 1;

}

/*******************************************************************************
file_window_get_selected_file~
*******************************************************************************/

file_info_struct *file_window_get_selected_file(void) {

	if (file_window_items_cur_sel < 0) return NULL;
	if (file_window_items_cur_sel >= file_window_items_count) return NULL;

	return file_window_items[file_window_items_cur_sel].file;

}


/******************************************************************************
file_window_add_file_and_update_list~

Reads a file's info from disk, adds it to memory, and updates the file list
if appropriate.
*******************************************************************************/

void file_window_add_file_and_update_list(char *filename) {

	file_info_struct *file;

	if ((file = file_read_and_add(filename)) &&
			file_window_dir_in_list(file->dir)) {
		file_window_add_item(file);
		wrefresh(file_window);
	}

}


/*******************************************************************************
file_window_remove_file_and_update_list~

Removes any information associated with the specified filename from memory, if
present, and also removes the file from the file_list if it is there
*******************************************************************************/

void file_window_remove_file_and_update_list(char *filename) {

	file_info_struct *file;
	int itemno;

	if ((file = file_find_from_full_name(filename,dir_window_rootdir))) {
		if ((itemno = file_window_find_itemno_from_file(file)) >= 0) {
			file_window_remove_item(itemno);
		}
		dir_remove_file(file->dir,file);
		file_free(file);
	}

}

/*******************************************************************************
file_window_update_file_and_update_list~

Update's the file's info in memory and in the file list
*******************************************************************************/

int file_window_update_file_and_update_list(char *filename) {

	file_info_struct *file;
	int linenum;

	if ((file = file_find_from_full_name(filename,dir_window_rootdir))) {
		if ((linenum = file_window_find_itemno_from_file(file)) >= 0)
			file_window_update_item(linenum);
		else file_update(file);
		return TRUE;
	}
	else return FALSE;

}


/*******************************************************************************
file_window_update_item~

Re-reads a file's info from disk, and either updates the file-list with the
new info, or removes it from the list if the file has disappeared.
*******************************************************************************/

int file_window_update_item(int itemno) {

	if ((itemno < 0) || (itemno >= file_window_items_count)) return FALSE;
	else if (!file_update(file_window_items[itemno].file)) {
		file_window_remove_item(itemno);
		return FALSE;
	}
	else {
		if ((itemno >= file_window_items_top) &&
				(itemno < file_window_items_top+get_window_height(file_window))) {
			file_window_draw_item(itemno);
			wrefresh(file_window);
		}
		return TRUE;
	}

	
}

/*******************************************************************************
file_window_dir_in_list~

Determines whether the file list displays files for the given directory.
*******************************************************************************/

int file_window_dir_in_list(dir_info_struct *dir) {

	if ((file_window_list_type == FILE_WINDOW_BRANCH_LIST) &&
			dir_check_if_parent(dir_window_get_selected_dir(),dir))
		return TRUE;
	else if (dir_window_get_selected_dir() == dir)
		return TRUE;
	else
		return FALSE;
}

/*******************************************************************************
file_window_items_compare_by_name~
*******************************************************************************/

int file_window_items_compare_by_name(const void* param1, const void* param2) {

	const file_window_item_struct *line1 = param1;
	const file_window_item_struct *line2 = param2;
	int namecmp, dircmp;
  int (*xstrcmp)() = file_window_sort_case ? strcmp : strcasecmp;

	namecmp = xstrcmp(line1->file->name,line2->file->name);
	if (!xstrcmp(line1->file->name,line2->file->name)) {  /* names are same */
		dircmp = xstrcmp(dir_get_path(line1->file->dir),dir_get_path(line2->file->dir));
		return dircmp;
	}
	else return namecmp;

}

/*******************************************************************************
file_window_items_compare~
*******************************************************************************/

int file_window_items_compare(const void* param1, const void* param2) {

	const file_window_item_struct *line1 = param1;
	const file_window_item_struct *line2 = param2;
	char *ext1, *ext2;
  int (*xstrcmp)() = file_window_sort_case ? strcmp : strcasecmp;
	int cmp = 0;

  if (file_window_sort_path && (line1->file->dir != line2->file->dir)) {
		cmp = xstrcmp(dir_get_path(line1->file->dir),dir_get_path(line2->file->dir));
	}
	else {
	  switch (file_window_sort_criteria) {
		  case EXT_CRITERIA:
			  ext1 = strrchr(line1->file->name,'.');
			  ext2 = strrchr(line2->file->name,'.');
				if (ext1 && ext2) {
				  cmp = xstrcmp(ext1,ext2);  /* same extension, comare by name */
					if (cmp == 0) cmp = file_window_items_compare_by_name(param1,param2);
				}
				else if (ext2) cmp = -1;
				else if (ext1) cmp = 1;
				else cmp = file_window_items_compare_by_name(param1,param2);  /* neither have extensions, compare by name */
			  break;
		  case SIZE_CRITERIA:
				if (line1->file->size < line2->file->size) cmp = -1;
				else if (line1->file->size > line2->file->size) cmp = 1;
				else cmp = file_window_items_compare_by_name(param1,param2); /* size same, compare by name */
				break;
		  case DATE_CRITERIA:
				if (line1->file->mtime < line2->file->mtime) cmp = -1;
				else if (line1->file->mtime > line2->file->mtime) cmp = 1;
				else cmp = 0;
				break;
		  default: /* name then path */
				cmp = file_window_items_compare_by_name(param1,param2);
		}
	}
	
  if (file_window_sort_desc) return cmp;
	else return -cmp;

}

/*******************************************************************************
file_window_hide_separator~
*******************************************************************************/

void file_window_hide_separator() {

	int height, width;
	int x;

	getmaxyx(minifl_separator_window,height,width);

	wset_predefined_background_color(minifl_separator_window,PANEL_COLOR);
	wset_predefined_color(minifl_separator_window,PANEL_COLOR);

	wmove(minifl_separator_window,0,0);
	waddch(minifl_separator_window,special_char[VLINE_CHAR]);
	for (x = 1; x < width-1; x++) waddch(minifl_separator_window,' ');
	waddch(minifl_separator_window,special_char[VLINE_CHAR]);
	wrefresh(minifl_separator_window);

}

/*******************************************************************************
file_window_show_separator~
*******************************************************************************/

void file_window_show_separator() {

	int height, width;
	int x;

	getmaxyx(minifl_separator_window,height,width);

	wset_predefined_background_color(minifl_separator_window,PANEL_COLOR);
	wset_predefined_color(minifl_separator_window,PANEL_COLOR);

	wmove(minifl_separator_window,0,0);
	waddch(minifl_separator_window,special_char[LTEE_CHAR]);
	for (x = 1; x < width-1; x++) waddch(minifl_separator_window,special_char[HLINE_CHAR]);
	waddch(minifl_separator_window,special_char[RTEE_CHAR]);
	wrefresh(minifl_separator_window);
	
}

/*******************************************************************************
file_window_init~
*******************************************************************************/

void file_window_init() {

	int screen_height, screen_width;

	switch (config.file_window.sort_criteria) {
		case CONFIG_FILE_WINDOW_SORT_CRITERIA_EXT: file_window_sort_criteria = EXT_CRITERIA; break;
		case CONFIG_FILE_WINDOW_SORT_CRITERIA_DATE: file_window_sort_criteria = DATE_CRITERIA; break;
		case CONFIG_FILE_WINDOW_SORT_CRITERIA_SIZE: file_window_sort_criteria = SIZE_CRITERIA; break;
		default: file_window_sort_criteria = NAME_CRITERIA;
	}		
	file_window_sort_desc = config.file_window.sort_desc;
	file_window_sort_path = config.file_window.sort_path;
	file_window_sort_case = config.file_window.sort_case;
	switch (config.file_window.display_format) {
		case CONFIG_FILE_WINDOW_DISPLAY_FORMAT_DATE: file_display_format = DATE_FORMAT; break;
		case CONFIG_FILE_WINDOW_DISPLAY_FORMAT_ATTRIBUTES: file_display_format = ATTRIBUTES_FORMAT; break;
		default: file_display_format = NORMAL_FORMAT;
	}

	getmaxyx(stdscr,screen_height,screen_width);
	minifl_separator_window = newwin_force(1,screen_width-info_width-1,screen_height-bottom_height-minifl_height-2,0);
	file_window = newwin_force(minifl_height,screen_width-info_width-3,screen_height-bottom_height-minifl_height-1,1);

	file_window_items_count = 0;
	file_window_items = NULL;
	file_window_items_cur_sel = -1;
	file_window_size = FILE_WINDOW_MINI_SIZE;
	file_window_list_type = FILE_WINDOW_NORMAL_LIST;
	notimeout(file_window,TRUE);
	
	filespec_list = filespec_list_create();	
	
}

/*******************************************************************************
file_window_draw_item~
*******************************************************************************/

static void file_window_draw_item(int itemno) {

	int color_to_use;
	int y, x, col = 1;
	int height, width;
	char *display_line;
	int hilighted = file_window_items_cur_sel == itemno;
	int display_length;
	
	getmaxyx(file_window,height,width); /* should be optimised so this doesn't have to be done every time */
	
	display_length = width/file_window_cols-3;
	if (display_length <= 0) display_length = 1;
	
	/* if the line isn't within the range of the file window or the list, don't draw it */
	if (itemno < file_window_items_top ||
			itemno >= file_window_items_top+file_window_cols*height ||
			itemno < 0 ||
			itemno >= file_window_items_count)
		return;
	
	y = itemno-file_window_items_top;
	

	if ((itemno >= 0) && (itemno < file_window_items_count)) {  
		/* decide which color to use */
		if (hilighted) color_to_use = HILIGHT_COLOR;
		else if (S_ISLNK(file_window_items[itemno].file->mode))
			color_to_use = SLINK_COLOR;
		else
			color_to_use = FILES_COLOR;

		/* determine line's column */ 
		while (itemno >= file_window_items_top+col*height) {
			col++;
		}
		x = (col-1)*(width/file_window_cols);
		
		/* draw the line */
		wmove(file_window,y-(col-1)*height,x);
		wset_predefined_color(file_window,hilighted ? HILIGHT_COLOR : FILES_COLOR);
		if (file_window_items[itemno].file->tagged) waddch(file_window,special_char[DIAMOND_CHAR]);
		else waddch(file_window,' ');
		wset_predefined_color(file_window,color_to_use);
		display_line = file_get_display_line(file_window_items[itemno].file,display_length,file_display_format);
		waddstr(file_window,display_line);
/*		waddstr(file_window,"  ");*/
		free(display_line);
	}

}

/*******************************************************************************
file_window_draw~
*******************************************************************************/

void file_window_draw() {

	int height, width;
	int cur_line;

	getmaxyx(file_window,height,width);
	
	wset_predefined_background_color(file_window,FILES_COLOR);
	werase(file_window);

	if (file_window_items_count == 0) {
		wset_predefined_color(file_window,FILES_COLOR);
		if ((file_window_list_type == FILE_WINDOW_NORMAL_LIST) &&
				(dir_window_get_selected_dir()->files))
			mvwprintw(file_window,0,1,"No Files Match");
		else
			mvwprintw(file_window,0,1,"Dir Empty");
	}
	else if (file_window_items_count < 0) {
		wset_predefined_color(file_window,FILES_COLOR);
		mvwprintw(file_window,0,1,"Dir Not Logged");
	}
	else {
		if (file_window_items_cur_sel >= 0) {
			if (file_window_items_cur_sel > file_window_items_top+file_window_cols*height-1)
				file_window_items_top = file_window_items_cur_sel-file_window_cols*height+1;
			else if (file_window_items_cur_sel < file_window_items_top)
				file_window_items_top = file_window_items_cur_sel;
		}
		for (cur_line = file_window_items_top;
 				(cur_line < file_window_items_count) && (cur_line < height*file_window_cols + file_window_items_top);
	 cur_line++) {
 			file_window_draw_item(cur_line);
		}
	}
	wrefresh(file_window);

}

/*******************************************************************************
file_window_select~
*******************************************************************************/

void file_window_select(int entrynum) {

	int width, height;
	int old_sel = file_window_items_cur_sel;
	getmaxyx(file_window,height,width);

	file_window_items_cur_sel = entrynum;

	if (!((old_sel > file_window_items_top+file_window_cols*height-1) ||
			(old_sel < file_window_items_top))) {
		wset_predefined_color(file_window,FILES_COLOR);
		file_window_draw_item(old_sel);
	}

	if (file_window_items_cur_sel >= 0) {
		if ((file_window_items_cur_sel > file_window_items_top+file_window_cols*height-1) ||
				(file_window_items_cur_sel < file_window_items_top))
			file_window_draw();
		wset_predefined_color(file_window,HILIGHT_COLOR);
		file_window_draw_item(file_window_items_cur_sel);

		wrefresh(file_window);
	}
	info_window_update_local_stats();
	wrefresh(info_window);

}

/*******************************************************************************
file_window_add_item~
*******************************************************************************/

void file_window_add_item(file_info_struct *file) {

	int itemno = 0;
	file_window_item_struct new_item;
	int height = get_window_height(file_window);

	if (file) {
		new_item.file = file;
		while ((itemno < file_window_items_count) &&
 					(file_window_items_compare(&file_window_items[itemno],&new_item) <= 0))
			itemno++;
		file_window_items_count++;
		file_window_items = (file_window_item_struct*) realloc(file_window_items,file_window_items_count*sizeof(file_window_item_struct));
		if (itemno < file_window_items_count-1)
			memmove(&file_window_items[itemno+1],
							&file_window_items[itemno],
							(file_window_items_count-1-itemno)*sizeof(file_window_item_struct));
		file_window_items[itemno].file = file;
		if (itemno <= file_window_items_cur_sel) file_window_items_cur_sel++;
		if (file_window_items_cur_sel == file_window_items_top+file_window_cols*height) {
			/* selection below bottom of file_window, scroll window down one line */
			file_window_items_top++;
		}
		if ((itemno >= file_window_items_top) && (itemno < file_window_items_top+file_window_cols*height)) {
			/* added line is within view, redraw the file window */
			file_window_draw();
		}
		wrefresh(file_window);
	}

}

/*******************************************************************************
file_window_find_itemno_from_file~
*******************************************************************************/

int file_window_find_itemno_from_file(file_info_struct *file) {

	int itemno = 0;
	int found_line = FALSE;
	
	while ((itemno < file_window_items_count) && !found_line) {
		if (file_window_items[itemno].file == file) found_line = TRUE;
		else itemno++;
	}
	if (found_line) return itemno;
	else return -1;

}

/*******************************************************************************
file_window_remove_item~
*******************************************************************************/

void file_window_remove_item(int itemno) {

	int height, width;
	
	getmaxyx(file_window,height,width);

	wset_predefined_background_color(file_window,FILES_COLOR);
	if ((itemno >= 0) && (itemno < file_window_items_count)) {
		if (file_window_items_count == 1) {
			/* last file in list - clear list, deselect */
			file_window_items_count = 0;
			free(file_window_items);
			file_window_items = NULL;
			file_window_resize_mini();
			file_window_draw();
			dir_window_activate();
		}
		else {
			if (itemno < file_window_items_count - 1)
				memmove(&file_window_items[itemno],
								&file_window_items[itemno+1],
								(file_window_items_count-itemno)*sizeof(file_window_item_struct));
			file_window_items_count--;
			file_window_items = (file_window_item_struct*) realloc(file_window_items,file_window_items_count*sizeof(file_window_item_struct));
			
			if (file_window_items_cur_sel >= file_window_items_count) file_window_select(file_window_items_count-1);
			if ((itemno >= file_window_items_top) && (itemno < file_window_items_top+file_window_cols*height)) {
				file_window_draw();
			}
			wrefresh(file_window);
		}
	}

}

/*******************************************************************************
file_window_add_items_from_dir~

Adds items for all matching files in the directory, but not files in
subdirectories.

Does not sort the list.
*******************************************************************************/

static void file_window_add_items_from_dir(dir_info_struct *dir) {

	file_info_struct *file;
	int all_files_count;
	int cur_file;
	int itemno = 0;

	if (dir->files) {
		file = dir->files;
		
		/* count the total number of files in the directory */
		all_files_count = 0;
		do all_files_count++; while ((file = file->next));

		/* make the list big enough to fit all the files */
		file_window_items = (file_window_item_struct*) realloc(file_window_items,(file_window_items_count+all_files_count)*sizeof(file_window_item_struct));

		/* only add matching files to the list */
		file = dir->files;
		for (cur_file = 0; cur_file < all_files_count; cur_file++) {
			if (filespec_list_check_match(filespec_list,file->name))
				file_window_items[file_window_items_count+itemno++].file = file;
			file = file->next;
		}

		/* resize the list to only fit matching files */
		file_window_items_count += itemno;
		file_window_items = (file_window_item_struct*) realloc(file_window_items,file_window_items_count*sizeof(file_window_item_struct));

	}

}

/*******************************************************************************
file_window_add_items_from_branch~

Adds items for all matching files in the directory, and all files in
subdirectories.

Does not sort the list.
*******************************************************************************/

static void file_window_add_items_from_branch(dir_info_struct* dir) {

	dir_info_struct *subdir;
	
	if (dir->logged) {
		file_window_add_items_from_dir(dir);

		if (dir->subdirs) {
			subdir = dir->subdirs;
			do {
				file_window_add_items_from_branch(subdir);
			} while ((subdir = subdir->next));
		}
	}

}

/*******************************************************************************
file_window_set_items_from_branch~

Sets the contents of the file window to be all the files in the given branch
that match the current filespec.
*******************************************************************************/

int file_window_set_items_from_branch(dir_info_struct* dir) {

	file_window_list_type = FILE_WINDOW_BRANCH_LIST;
	file_window_items_cur_sel = -1;
	file_window_items_top = 0;

	if (!dir->logged) return FALSE;
	else {
		file_window_items_count = 0;
		file_window_add_items_from_branch(dir);
		file_window_branch_topdir = dir;
		qsort(file_window_items,file_window_items_count,sizeof(file_window_item_struct),file_window_items_compare);
		file_window_set_cols();
		return (file_window_items_count > 0);
	}


}

/*******************************************************************************
file_window_set_items_from_dir~

Sets the contents of the file window to be all the files in the given directory
that match the current filespec.
*******************************************************************************/

void file_window_set_items_from_dir(dir_info_struct* dir) {

	file_window_list_type = FILE_WINDOW_NORMAL_LIST;
	file_window_items_cur_sel = -1;
	file_window_items_top = 0;

	file_window_items_count = 0;
	if (!dir->logged) file_window_items_count = -1;
	else {
		file_window_add_items_from_dir(dir);
		qsort(file_window_items,file_window_items_count,sizeof(file_window_item_struct),file_window_items_compare);
		file_window_set_cols();
	}

}

/*******************************************************************************
file_window_resize_normal~
*******************************************************************************/

void file_window_resize_normal() {

	int screen_height, screen_width;
	int file_window_height, file_window_width;

	getmaxyx(stdscr,screen_height,screen_width);

	mvwin(file_window,2,1);
	resize_window(&file_window,screen_height-bottom_height-3,screen_width-info_width-3);

	file_window_hide_separator();

	getmaxyx(file_window,file_window_height,file_window_width);
		
	if (file_window_items_top + file_window_height > file_window_items_count) {    
		if (file_window_items_count <= file_window_height) file_window_items_top = 0;
		else file_window_items_top = file_window_items_count - file_window_height;    
	}
		
	file_window_size = FILE_WINDOW_NORMAL_SIZE;

	file_window_draw();

}

/*******************************************************************************
file_window_resize_mini~
*******************************************************************************/

void file_window_resize_mini() {

	int screen_height, screen_width;

	getmaxyx(stdscr,screen_height,screen_width);

	resize_window(&file_window,minifl_height,screen_width-info_width-3);
	mvwin(file_window,screen_height-bottom_height-minifl_height-1,1);

	file_window_show_separator();

	file_window_size = FILE_WINDOW_MINI_SIZE;

	file_window_draw();

}

/*******************************************************************************
file_window_sel_up~

Move the selection up one line, if it is not already at the top.
*******************************************************************************/

static void file_window_sel_up(void) {

	int width, height;
	int col;

	if (file_window_items_cur_sel > 0) {
		getmaxyx(file_window,height,width);
		if (file_window_items_cur_sel == file_window_items_top) {
			wmove(file_window,0,0);
			winsdelln(file_window,1);
			file_window_items_top--;
			/* we have moved all lines in the window down one, so draw the top line */
			for (col = 1; col <= file_window_cols; col++) {
				if (file_window_items_top+(col-1)*height < file_window_items_count)
					file_window_draw_item(file_window_items_top+(col-1)*height);
			}
		}
		file_window_select(file_window_items_cur_sel-1);
	}

}

/*******************************************************************************
file_window_sel_down~

Move the selection down one line, if it is not already at the bottom.
*******************************************************************************/

static void file_window_sel_down(void) {

	int width, height;
	int col;

	if (file_window_items_cur_sel < file_window_items_count-1) {
		getmaxyx(file_window,height,width);
		if (file_window_items_cur_sel == file_window_items_top+file_window_cols*height-1) {
			wmove(file_window,0,0);
			wdeleteln(file_window);
			file_window_items_top++;
			/* we have moved all lines in the window up one, so draw the bottom line */
			for (col = 1; col <= file_window_cols; col++) {
				if (file_window_items_top+col*height-1 < file_window_items_count)
					file_window_draw_item(file_window_items_top+col*height-1);
			}
		}
		file_window_select(file_window_items_cur_sel+1);
	}


}

/*******************************************************************************
file_window_sel_left~

Move the selection left one column
*******************************************************************************/

static void file_window_sel_left(void) {

	int height, width;
	int col = 1;
	int old_top;
	
	getmaxyx(file_window,height,width);
	
	while (file_window_items_cur_sel >= file_window_items_top+col*height)
		col++;
	
	if (col > 1) {
		/* not in first column */
		file_window_select(file_window_items_cur_sel-height);
	}
	else if (file_window_items_top - height < 0) {
		/* top of first column is less than one column's worth of items from the top of the list */
		old_top = file_window_items_top;
		file_window_items_top = 0;
		file_window_select(file_window_items_cur_sel-old_top);
		file_window_draw();
	}
	else {
		file_window_items_top -= height;
		file_window_select(file_window_items_cur_sel-height);
		file_window_draw();
	}


}

/*******************************************************************************
file_window_sel_right~

Move the selection right one column
*******************************************************************************/

static void file_window_sel_right(void) {

	int height, width;
	int col = 1;
	int new_sel;
	
	getmaxyx(file_window,height,width);
	
	while (file_window_items_cur_sel >= file_window_items_top+col*height)
		col++;
	
	new_sel = file_window_items_cur_sel+height;
	if (new_sel >= file_window_items_count) new_sel = file_window_items_count-1;
	
	if (col < file_window_cols) {
		/* not in last column */
		if (file_window_items_top+col*height < file_window_items_count)
			file_window_select(new_sel);
	}
	else if (file_window_items_top+file_window_cols*height < file_window_items_count) {
		/* next column to the right is less than a whole column's worth of items before the end of the list */
		file_window_items_top += height;
		file_window_select(new_sel);
		file_window_draw();
	}
	

}

/*******************************************************************************
file_window_sel_pgup~

Move the selection up one page, if it is not already at the top.
*******************************************************************************/

static void file_window_sel_pgup(void) {

	int width, height;

	if (file_window_items_cur_sel > 0) {
		getmaxyx(file_window,height,width);
		if (file_window_items_cur_sel < height)
			file_window_select(0);
		else file_window_select(file_window_items_cur_sel-file_window_cols*height+1);
	}

}

/*******************************************************************************
file_window_sel_pgdn~

Move the selection down one page, if it is not already at the bottom.
*******************************************************************************/

static void file_window_sel_pgdn(void) {

	int width, height;

	if (file_window_items_cur_sel < file_window_items_count-1) {
		getmaxyx(file_window,height,width);
		if (file_window_items_cur_sel+file_window_cols*height-1 > file_window_items_count-1)
			/* selection is less than a page before the end, so select the end */
			file_window_select(file_window_items_count-1);
		else
			/* select one page down */
			file_window_select(file_window_items_cur_sel+file_window_cols*height-1);
	}
	
}

/*******************************************************************************
file_window_sel_top~

Move the selection to the top, if it is not already there.
*******************************************************************************/

static void file_window_sel_top(void) {

	if (file_window_items_cur_sel > 0) {
		file_window_select(0);
	}

}

/*******************************************************************************
file_window_sel_bottom~

Move the selection to the top, if it is not already there.
*******************************************************************************/

static void file_window_sel_bottom(void) {

	if (file_window_items_cur_sel < file_window_items_count-1) {
		file_window_select(file_window_items_count-1);
	}

}

/*******************************************************************************
file_window_enter~
*******************************************************************************/

static void file_window_enter(void) {

	if (file_window_size == FILE_WINDOW_MINI_SIZE) {
		file_window_resize_normal();
		info_window_update();
	}
	else {
		file_window_select(-1);
		file_window_resize_mini();
		dir_window_activate();
	}

}

/*******************************************************************************
file_window_switch_to_dir_window~
*******************************************************************************/

static void file_window_switch_to_dir_window() {

	file_window_select(-1);
	file_window_resize_mini();
	dir_window_activate();
	
}

/*******************************************************************************
file_window_process_key~

Returns 0 if key was not processed.
*******************************************************************************/

int file_window_process_key(key_type key) {

	int processed = TRUE;

	switch(key) {
		case     ENTER_KEY: file_window_enter(); break;
		case     F_KEY(10): file_window_switch_to_dir_window(); break;
		case        UP_KEY: file_window_sel_up(); break;
		case      DOWN_KEY:
		case           ' ': file_window_sel_down(); break;
		case      LEFT_KEY: file_window_sel_left(); break;
		case     RIGHT_KEY: file_window_sel_right(); break;
		case      PGUP_KEY: file_window_sel_pgup(); break;
		case      PGDN_KEY: file_window_sel_pgdn(); break;
		case      HOME_KEY: file_window_sel_top(); break;
		case       END_KEY: file_window_sel_bottom(); break;

		case           'A': file_window_attributes(FALSE);        break;  /* file attributes */
		case           'C': file_window_copy();                   break;  /* copy file */
		case           'D': file_window_delete();                 break;  /* delete file */
		case           'E': file_window_edit();                   break;  /* edit file */
		case           'L': file_window_symlink();                break;  /* symLink file */
		case           'M': file_window_move();                   break;  /* move file */
		case           'R': file_window_rename();                 break;  /* rename file */
		case           'T': file_window_set_tagged(TRUE);         break;  /* tag file */
		case           'U': file_window_set_tagged(FALSE);        break;  /* untag file */
		case           'V': file_window_view();                   break;  /* view file */
		/* CTRL keys */
		case CTRL_KEY('A'): file_window_attributes(TRUE);         break;  /* tagged file attributes */
		case CTRL_KEY('C'): file_window_copy_tagged();            break;  /* copy tagged files */
		case CTRL_KEY('D'): file_window_delete_tagged();          break;  /* delete tagged files */
		/* ctrl+k is move for now as ctrl+m is enter :( */
		case CTRL_KEY('K'): file_window_move_tagged();            break;  /* move tagged files */
		case CTRL_KEY('L'): file_window_symlink_tagged();         break;  /* symlink tagged files */
		case CTRL_KEY('T'): file_window_set_tagged_all(TRUE);     break;  /* TAG all files */
		case CTRL_KEY('U'): file_window_set_tagged_all(FALSE);    break;  /* UNTAG all files */
		/* ALT keys */
		case  ALT_KEY('C'): file_window_copy_with_paths();        break;  /* copy tagged files */
		case  ALT_KEY('K'): file_window_move_with_paths();        break;  /* move tagged files */
		case  ALT_KEY('L'): file_window_symlink_with_paths();     break;  /* symlink tagged files */
			
		default: processed = FALSE;
	}
	
	return processed;
	
}

/*******************************************************************************
file_window_activate~
*******************************************************************************/

void file_window_activate(void) {

	current_mode = FILE_LIST_MODE;

	if (file_window_items_count <= 0) {
		dir_window_activate();
		return;
	}

	if (file_window_size == FILE_WINDOW_NORMAL_SIZE) file_window_resize_normal();
	else file_window_resize_mini();
 
	file_window_draw();
	file_window_select(0);
	info_window_update();

}

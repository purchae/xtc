/*  info_window.c - routines to manipulate info window other misc. parts of screen
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

WINDOW *path_window;
WINDOW *info_window;
static int info_window_drawn;
int info_window_input_path_pos = 0;
WINDOW *command_window;
static int command_window_height = 2;


/*******************************************************************************
info_window_draw~
*******************************************************************************/

void info_window_draw(void) {

	info_window_drawn = FALSE;
	info_window_update();

}

/*******************************************************************************
info_window_check_if_all~

Always returns true.
*******************************************************************************/

int info_window_check_if_all(file_info_struct *file) {

	return TRUE;

}

/*******************************************************************************
info_window_check_if_tagged~

Checks if a file is tagged.
*******************************************************************************/

int info_window_check_if_tagged(file_info_struct *file) {

	return file->tagged;

}

/*******************************************************************************
info_window_check_if_matching~

Checks if a file matches the current filespec.
*******************************************************************************/

int info_window_check_if_matching(file_info_struct *file) {

	return filespec_list_check_match(filespec_list,file->name);

}

/*******************************************************************************
info_window_init~
*******************************************************************************/

void info_window_init(void) {

	int screen_height, screen_width;
	
	getmaxyx(stdscr,screen_height,screen_width);

	/* create path window */
	path_window = newwin_force(1,screen_width,0,0);
	wset_predefined_background_color(path_window,PANEL_COLOR);
	wset_predefined_color(path_window,PANEL_COLOR);
	werase(path_window);
	wrefresh(path_window);

  /* create command window */
	command_window = newwin_force(command_window_height,screen_width,screen_height-command_window_height-1,0);
	wset_predefined_background_color(command_window,PANEL_COLOR);
	wset_predefined_color(command_window,PANEL_COLOR);
	werase(command_window);
	wrefresh(command_window);


	info_window = newwin_force(screen_height-bottom_height-3,info_width,2,screen_width-info_width-1);
	info_window_draw();
	

}

/*******************************************************************************
info_window_reinit~
*******************************************************************************/

void info_window_reinit(void) {

	delwin(info_window);
	delwin(path_window);
	delwin(command_window);
	info_window_init();

}

/*******************************************************************************
info_window_update_filespec~
*******************************************************************************/

void info_window_update_filespec(void) {

	char *filespec_string;
	char *display_string;
	
	filespec_string = filespec_list_to_string(filespec_list," ");
	display_string = truncate_string(filespec_string,13);
	if (strlen(filespec_string) > strlen(display_string))
		display_string[12] = '>';
	wmove(info_window,0,6);
	wclrtoeol(info_window);
	mvwaddstr(info_window,0,6,display_string);
	free(display_string);
	free(filespec_string);

}

/*******************************************************************************
info_window_update_path~
*******************************************************************************/

void info_window_update_path(char *path) {

	char *display_path = truncate_string(path,get_window_width(path_window)-9);

	wmove(path_window,0,0);
	mvwprintw(path_window,0,2,"Path: %s",display_path);
	free(display_path);
	wclrtoeol(path_window);
	wrefresh(path_window);

}

/*******************************************************************************
info_window_update_input_path~
*******************************************************************************/

void info_window_update_input_path(char *path) {

	wmove(command_window,0,0);
	mvwprintw(command_window,1,info_window_input_path_pos,"%s",path);
	wclrtoeol(command_window);
	wrefresh(command_window);

}

/*******************************************************************************
info_window_update_disk_stats~
*******************************************************************************/

void info_window_update_disk_stats() {

	long freek;

	wset_predefined_color(info_window,TEXT_COLOR);  
	wmove(info_window,4,8);  wclrtoeol(info_window);
	if (((current_mode == FILE_LIST_MODE) && file_window_get_selected_file() &&
			!get_fs_freek(file_get_full_name(file_window_get_selected_file()),&freek))
			||
			((current_mode == DIR_TREE_MODE) && (dir_window_get_selected_dir != NULL) &&
 			!get_fs_freek(dir_get_path(dir_window_get_selected_dir()),&freek))) {
		mvwprintw(info_window,4,8,"%11s",comma_number(freek));
	}

}

/*******************************************************************************
info_window_update_stats~
*******************************************************************************/

/*static void info_window_update_stats1(int (*check_func)(file_info_struct*)) {*/
void info_window_update_stats(int type) {

	int y = 0;
	int (*check_func)(file_info_struct*);
	
	switch (type) {
		case MATCHING_FILES:
			y = 11;
			check_func = &info_window_check_if_matching;
			break;
		case TAGGED_FILES:
			y = 14;
			check_func = &info_window_check_if_tagged;
			break;
		default:
			y = 8;
			check_func = &info_window_check_if_all;
			break;
	}

	wset_predefined_color(info_window,TEXT_COLOR);
	if ((current_mode == DIR_TREE_MODE) &&
			(dir_window_get_selected_dir != NULL)) {
		mvwprintw(info_window,y,8,"%11s",comma_number(dir_count_total_files(dir_window_rootdir,check_func)));
		mvwprintw(info_window,y+1,8,"%11s",comma_number(dir_calculate_total_size(dir_window_rootdir,check_func)));
	}
	else if ((current_mode == FILE_LIST_MODE) &&
 					file_window_get_selected_file()) {
		wset_predefined_color(info_window,TEXT_COLOR);
		/* BRANCH view */
		if (file_window_list_type == FILE_WINDOW_BRANCH_LIST) {
			mvwprintw(info_window,y,8,"%11s",comma_number(dir_count_total_files(dir_window_get_selected_dir(),check_func)));
			mvwprintw(info_window,y+1,8,"%11s",comma_number(dir_calculate_total_size(dir_window_get_selected_dir(),check_func)));
		}
		/* NORMAL view */
		else {
			mvwprintw(info_window,y,8,"%11s",comma_number(dir_count_files(dir_window_get_selected_dir(),check_func)));
			mvwprintw(info_window,y+1,8,"%11s",comma_number(dir_calculate_size(dir_window_get_selected_dir(),check_func)));
		}
	}
 
}

/*******************************************************************************
info_window_update_stats2~
*******************************************************************************/

/*void info_window_update_stats2(int type) {

	switch(type) {
		case MATCHING_FILES:
			info_window_update_stats1(&info_window_check_if_all);
			break;
		case TAGGED_FILES:
			info_window_update_stats1(&info_window_check_if_tagged);
			break;
		case DEFAULT:
			info_window_update_stats1(&info_window_check_if_all);
	}

}*/

/*******************************************************************************
info_window_update_local_stats~

Updates the statistics for the currently selected file or directory.
*******************************************************************************/

void info_window_update_local_stats() {

	char tempstr[18];
	int width, height;
	file_info_struct *file;
	dir_info_struct *dir;

	getmaxyx(info_window,height,width);

	if ((current_mode == DIR_TREE_MODE) &&
			(dir_window_mode != DIR_WINDOW_SELECT_MODE) &&
			(dir_window_get_selected_dir != NULL)) {

		dir = dir_window_get_selected_dir();
		wset_predefined_color(info_window,TEXT_COLOR);
		strncpy(tempstr,dir->name,17);
		if (strlen(dir->name) >= 17) tempstr[17] = 0;
		mvwprintw(info_window,17,2,tempstr);  wclrtoeol(info_window);
		mvwprintw(info_window,18,8,"%11s",comma_number(dir_calculate_size(dir,&info_window_check_if_all)));

		if (height > 19) mvwprintw(info_window,19,11,"%8s",dir->username);
		if (height > 20) mvwprintw(info_window,20,11,"%8s",dir->groupname);
		if (height > 21) mvwprintw(info_window,21,9,"%s",mode_string(dir->mode));
		if (height > 22) mvwprintw(info_window,22,7,"%12s","Directory");
		if (height > 23) mvwprintw(info_window,23,9,"%s",date_string(dir->mtime,config.misc.date_format));
		if (height > 24) mvwprintw(info_window,24,8,"%11s",time_string(dir->mtime,config.misc.time_format));
	}
	else if ((current_mode == FILE_LIST_MODE) &&
 					file_window_get_selected_file()) {
		file = file_window_get_selected_file();
		strncpy(tempstr,file->name,17);
		if (strlen(file->name) >= 17) tempstr[17] = 0;
		mvwprintw(info_window,17,2,tempstr);  wclrtoeol(info_window);
		mvwprintw(info_window,18,8,"%11s",comma_number(file->size));
		info_window_update_path(dir_get_path(file->dir));
		if (height > 19) mvwprintw(info_window,19,11,"%8s",file->username);
		if (height > 20) mvwprintw(info_window,20,11,"%8s",file->groupname);
		if (height > 21) mvwprintw(info_window,21,9,"%s",mode_string(file->mode));
		if (height > 22) mvwprintw(info_window,22,7,"%12s",file_type_string(file->mode,TRUE));
		if (height > 23) mvwprintw(info_window,23,9,"%s",date_string(file->mtime,config.misc.date_format));
		if (height > 24) mvwprintw(info_window,24,8,"%11s",time_string(file->mtime,config.misc.time_format));
	}

}

/*******************************************************************************
info_window_update~
*******************************************************************************/

void info_window_update() {

	int x;
	int width, height;

	getmaxyx(info_window,height,width);
	
	if (!info_window_drawn) {
		wset_predefined_background_color(info_window,PANEL_COLOR);
		werase(info_window);
	
		wset_predefined_color(info_window,PANEL_COLOR);
		for (x = 0; x < width; x++) {
			mvwaddch(info_window,1,x,special_char[HLINE_CHAR]);
			mvwaddch(info_window,5,x,special_char[HLINE_CHAR]);
		}

		mvwprintw(info_window, 0,0,"FILE");
		mvwprintw(info_window, 2,0,"DISK");
		mvwprintw(info_window, 3,0," Available");
		mvwprintw(info_window, 4,0," KBytes");
		mvwprintw(info_window, 7,0," Total");
		mvwprintw(info_window, 8,0,"  Files");
		mvwprintw(info_window, 9,0,"  Bytes");
		mvwprintw(info_window,10,0," Matching");
		mvwprintw(info_window,11,0,"  Files");
		mvwprintw(info_window,12,0,"  Bytes");
		mvwprintw(info_window,13,0," Tagged");
		mvwprintw(info_window,14,0,"  Files");
		mvwprintw(info_window,15,0,"  Bytes");
		mvwprintw(info_window,18,0,"  Bytes");
		if (height > 19) mvwprintw(info_window,19,0,"  Owner");
		if (height > 20) mvwprintw(info_window,20,0,"  Group");
		if (height > 21) mvwprintw(info_window,21,0,"  Mode");
		if (height > 22) mvwprintw(info_window,22,0,"  Type");
		if (height > 23) mvwprintw(info_window,23,0,"  Date");
		if (height > 24) mvwprintw(info_window,24,0,"  Time");
		wset_predefined_color(info_window,TEXT_COLOR);
		mvwprintw(info_window,0,6,"*");
		info_window_drawn = TRUE;
		
	}
/*  wmove(info_window,8,7); wclrtoeol(info_window);
	wmove(info_window,9,7); wclrtoeol(info_window);
	wmove(info_window,11,7); wclrtoeol(info_window);
	wmove(info_window,12,7); wclrtoeol(info_window);
	wmove(info_window,14,7); wclrtoeol(info_window);
	wmove(info_window,15,7); wclrtoeol(info_window);*/
	
/*  wmove(info_window,6,0); wclrtoeol(info_window);
	wmove(info_window,16,0); wclrtoeol(info_window);
	wmove(info_window,17,0); wclrtoeol(info_window);
	wmove(info_window,18,7); wclrtoeol(info_window);*/

	if (current_mode == DIR_TREE_MODE) {
		if (dir_window_mode == DIR_WINDOW_SELECT_MODE) info_window_update_input_path(dir_get_path(dir_window_get_selected_dir()));
		else {
			wset_predefined_color(info_window,TEXT_COLOR);
			mvwprintw(info_window,6,0,"DISK Statistics     ");
			wset_predefined_color(info_window,PANEL_COLOR);
			mvwprintw(info_window,16,0," Current Directory  ");

			if (dir_window_get_selected_dir != NULL) {
				info_window_update_local_stats();
				info_window_update_stats(ALL_FILES);
				info_window_update_stats(MATCHING_FILES);
				info_window_update_stats(TAGGED_FILES);
				info_window_update_path(dir_get_path(dir_window_get_selected_dir()));
			}
		
			wset_predefined_color(command_window,PANEL_COLOR);
			werase(command_window);
			mvwprintw(command_window,0,0,"DIR       Attributes  Branch  Delete  Filespec  symLink  Make  Prune  Rename");
			mvwprintw(command_window,1,0,"COMMANDS  Tag  Untag  Quit");
			status_line_set_message("<-| file  ");
			wset_predefined_color(command_window,TEXT_COLOR);
			mvwprintw(command_window,2,0,"<%c%c",special_char[HLINE_CHAR],special_char[LRCORNER_CHAR]);
			mvwaddch(command_window,0,10,'A');
			mvwaddch(command_window,0,22,'B');
			mvwaddch(command_window,0,30,'D');
			mvwaddch(command_window,0,38,'F');
			mvwaddch(command_window,0,51,'L');
			mvwaddch(command_window,0,57,'M');
			mvwaddch(command_window,0,63,'P');
			mvwaddch(command_window,0,70,'R');
			mvwaddch(command_window,1,10,'T');
			mvwaddch(command_window,1,15,'U');
			mvwaddch(command_window,1,22,'Q');
			wmove(command_window,0,0);
		}

	}
	else if (current_mode == FILE_LIST_MODE) {
		wset_predefined_color(info_window,TEXT_COLOR);
		if (file_window_list_type == FILE_WINDOW_BRANCH_LIST)
 				mvwprintw(info_window,6,0,"BRANCH Statistics   ");
		else mvwprintw(info_window,6,0,"DIRECTORY Stats     ");
		wset_predefined_color(info_window,PANEL_COLOR);
		mvwprintw(info_window,16,0," Current File       ");
		if (file_window_get_selected_file()) {
			
			info_window_update_stats(ALL_FILES);
			info_window_update_stats(MATCHING_FILES);
			info_window_update_stats(TAGGED_FILES);

			wset_predefined_color(info_window,TEXT_COLOR);
			/* Current File */
			info_window_update_local_stats();
		}
		wset_predefined_color(command_window,PANEL_COLOR);
		werase(command_window);
		mvwprintw(command_window,0,0,"FILE      Attributes  Copy  Delete  Filespec  symLink  Move  Rename  Tag  Untag");
		mvwprintw(command_window,1,0,"COMMANDS  Quit");
		if (file_window_size == FILE_WINDOW_MINI_SIZE) status_line_set_message("<-| more  ");
		else status_line_set_message("<-| tree  ");
		wset_predefined_color(command_window,TEXT_COLOR);
		mvwprintw(command_window,2,0,"<%c%c",special_char[HLINE_CHAR],special_char[LRCORNER_CHAR]);
    mvwaddch(command_window,0,10,'A');
    mvwaddch(command_window,0,22,'C');
    mvwaddch(command_window,0,28,'D');
    mvwaddch(command_window,0,36,'F');
    mvwaddch(command_window,0,49,'L');
    mvwaddch(command_window,0,55,'M');
    mvwaddch(command_window,0,61,'R');
    mvwaddch(command_window,0,69,'T');
    mvwaddch(command_window,0,74,'U');
    mvwaddch(command_window,1,10,'Q');
		wmove(command_window,0,0);
	}
	info_window_update_disk_stats();
	info_window_update_filespec();
	
	wrefresh(info_window);
	wrefresh(command_window);

}

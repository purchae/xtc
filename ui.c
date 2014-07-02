/*  ui.c - various user interface functions
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

char* reloc_file_op_initial_name;
char* reloc_file_op_base_path;
int reloc_file_op_prompt_len;


/*******************************************************************************
hide_cursor~
*******************************************************************************/

void hide_cursor(void) {

	curs_set(0);

}

/*******************************************************************************
show_cursor~
*******************************************************************************/

void show_cursor(void) {

	curs_set(1);

}

/*******************************************************************************
redraw_screen~
*******************************************************************************/

void redraw_screen(void) {

	file_window_set_cols(); file_window_draw();
	info_window_update();
	touchwin(stdscr); wnoutrefresh(stdscr);
	touchwin(dir_window); wnoutrefresh(dir_window);
	touchwin(file_window); wnoutrefresh(file_window);
	touchwin(path_window); wnoutrefresh(path_window);
	touchwin(info_window); wnoutrefresh(info_window);
	touchwin(command_window); wnoutrefresh(command_window);
	doupdate();

}

#ifdef HAVE_RESIZETERM
/*******************************************************************************
resize_screen~

From view.c in the test directory for ncurses-4.2:
This uses functions that are "unsafe", but it seems to work on SunOS and
Linux.  The 'wrefresh(curscr)' is needed to force the refresh to start from
the top of the screen -- some xterms mangle the bitmap while resizing.
*******************************************************************************/

void resize_screen(int sig) {
	struct winsize size;
  int height, width;

	if (ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
		resizeterm(size.ws_row, size.ws_col);

		wrefresh(curscr);	/* Linux needs this */

		if (current_mode == CONFIG_MODE) config_screen_resize();
		else {
			getmaxyx(stdscr,height,width);

			main_window_draw();

			/* dir window and file window */
			delwin(dir_window);
			dir_window = newwin_force(height-bottom_height-minifl_height-4,width-info_width-3,2,1);

			delwin(minifl_separator_window);
			minifl_separator_window = newwin_force(1,width-info_width-1,height-bottom_height-minifl_height-2,0);

		  delwin(file_window);
			if (file_window_size == FILE_WINDOW_MINI_SIZE) {
				file_window = newwin_force(minifl_height,width-info_width-3,height-bottom_height-minifl_height-1,1);
				file_window_show_separator();
				dir_window_draw();
			}
			else {
				file_window = newwin_force(height-bottom_height-3,width-info_width-3,2,1);
				file_window_hide_separator();
			}
			file_window_set_cols();
			file_window_draw();

			/* status line, command window and info window */
			status_line_reinit();
			info_window_reinit();

			wrefresh(stdscr);
		}
	}

	(void) signal(SIGWINCH, resize_screen);	/* some systems need this */

}
#endif

/*******************************************************************************
confirm_quit~

Asks the user whether or not they want to quit the program. Returns zero on no,
non-zero on yes.
*******************************************************************************/

int confirm_quit(void) {

	int response;

	if (config.misc.skip_quit_prompt) return TRUE; /* skip prompt */

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"QUIT");
	wrefresh(command_window);
	
	response = status_line_get_response("Quit XTC?",RESPONSE_YES+RESPONSE_NO+RESPONSE_CANCEL,TRUE);
	
	info_window_update(); /* to redraw command window */
	
	switch (response) {
		case RESPONSE_YES: return TRUE;
		default: return FALSE;
	}

}

/*******************************************************************************
main_window_draw~
*******************************************************************************/

void main_window_draw(void) {

	int x, y, width, height, line_x, line_y, bottom_y;


	getmaxyx(stdscr,height,width);
	wset_predefined_background_color(stdscr,TEXT_COLOR);
	werase(stdscr);
	wrefresh(stdscr);
	wset_predefined_color(stdscr,LINES_COLOR);
	bottom_y = height - bottom_height-1;
	line_x = width - info_width-2;
	line_y = height-bottom_height-minifl_height-2;
	mvwaddch(stdscr,1,0,special_char[ULCORNER_CHAR]);
	mvwaddch(stdscr,bottom_y,0,special_char[LLCORNER_CHAR]);
	mvwaddch(stdscr,1,width-1,special_char[URCORNER_CHAR]);
	mvwaddch(stdscr,bottom_y,width-1,special_char[LRCORNER_CHAR]);

	for (x = 1; x < width-1; x++) {
		mvwaddch(stdscr,1,x,special_char[HLINE_CHAR]);
		mvwaddch(stdscr,bottom_y,x,special_char[HLINE_CHAR]);
	}
	for (x = 1; x < line_x; x++)
		mvwaddch(stdscr,line_y,x,special_char[HLINE_CHAR]);
	for (y = 2; y < bottom_y; y++) {
		mvwaddch(stdscr,y,0,special_char[VLINE_CHAR]);
		mvwaddch(stdscr,y,line_x,special_char[VLINE_CHAR]);
		mvwaddch(stdscr,y,width-1,special_char[VLINE_CHAR]);
	}
	wmove(stdscr,1,0);
	mvwaddch(stdscr,1,line_x,special_char[TTEE_CHAR]);
	mvwaddch(stdscr,bottom_y,line_x,special_char[BTEE_CHAR]);
	mvwaddch(stdscr,line_y,0,special_char[LTEE_CHAR]);
	mvwaddch(stdscr,line_y,line_x,special_char[RTEE_CHAR]);
	mvwaddch(stdscr,3,width-info_width-2,special_char[LTEE_CHAR]);
	mvwaddch(stdscr,3,width-1,special_char[RTEE_CHAR]);
	mvwaddch(stdscr,7,width-info_width-2,special_char[LTEE_CHAR]);
	mvwaddch(stdscr,7,width-1,special_char[RTEE_CHAR]);
	
	wrefresh(stdscr);

}

/*******************************************************************************
newwin_force~

Same as newwin, but forces the new window to be created. The size and position
of the window may be different to that supplied to make the window fit on the
screen. This is useful when working with very small screen, and may mean that
the screen won't look right, but means the program can continue to operate as
normal.

This function exists mainly to ensure that the program doesn't crash if the user
resizes their xterm window to something ridiculously small.

Assumes that the height and width of the screen are at least 1.
*******************************************************************************/

WINDOW *newwin_force(int height, int width, int y, int x) {

  int screen_height, screen_width;
			
	getmaxyx(stdscr,screen_height,screen_width);

	if (height > screen_height) height = screen_height;
  if (width > screen_width) width = screen_width;
		
  if (y < 0) y = 0;	
  if (x < 0) x = 0;
  if (height <= 0) height = 1;
  if (width <= 0) width = 1;
	
	if (y+height > screen_height) y = screen_height - height;
	if (x+width > screen_width) x = screen_width - width;

	return newwin(height,width,y,x);

}


/*******************************************************************************
resize_window~

Replacement for ncurses wresize because not all versions of curses have wresize.
This function deletes the existing window and creates a new one, so all contents
and window settings are lost.
*******************************************************************************/

void resize_window(WINDOW **win, int height, int width) {

	int y, x;

	getbegyx(*win,y,x);
	delwin(*win);
	*win = newwin_force(height,width,y,x);
	

}

/*******************************************************************************
command_window_show_from_to~
*******************************************************************************/

void command_window_show_from_to(char *op, char *from, char *to) {

	int op_len = strlen(op);
	int display_width = get_window_width(command_window)-op_len-3;
	char *display_name;

	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,"%s: ",op);
	mvwprintw(command_window,1,op_len-2,"to: ");
	
	wset_predefined_color(command_window,TEXT_COLOR);
	display_name = truncate_string(from,display_width);
	mvwprintw(command_window,0,op_len+2,"%s",display_name);
	free(display_name);
	display_name = truncate_string(to,display_width);
	mvwprintw(command_window,1,op_len+2,"%s",display_name);
	free(display_name);
	wrefresh(command_window);  

}

/*******************************************************************************
ask_replace~
*******************************************************************************/

int ask_replace(int file_pos, char *src_filename, char *dest_filename) {

	int height, width;
	int response;
	file_info_struct *src_file, *dest_file;
	char *src_name, *dest_name;
	char *display_line;
	
	getmaxyx(command_window,height,width);

	src_name = get_name(src_filename);
	dest_name = get_name(dest_filename);
	src_file = file_read(src_filename);
	dest_file = file_read(dest_filename);
	
	if (src_file && dest_file) {
		/* display src file's stats */
		wmove(command_window,0,file_pos);  wclrtoeol(command_window);
		display_line = file_get_display_line(src_file,width-file_pos-1,file_display_format);
		waddstr(command_window,display_line);
		free(display_line);
		/* display dest file's stats */
		wmove(command_window,1,file_pos);  wclrtoeol(command_window);
		display_line = file_get_display_line(dest_file,width-file_pos-1,file_display_format);
		waddstr(command_window,display_line);
		free(display_line);
	}
	free(src_name);
	free(dest_name);
	wrefresh(command_window);
	if (src_file) file_free(src_file);
	if (dest_file) file_free(dest_file);

	response = status_line_get_response("File exists, replace?",RESPONSE_YES+RESPONSE_NO+RESPONSE_CANCEL,TRUE);


	return response;

}

/*******************************************************************************
reloc_file_op_copy_name_ext~
*******************************************************************************/

char *reloc_file_op_copy_name_ext(char *line,key_type key) {

	if (key == TAB_KEY) {
		return (char*) strdup(reloc_file_op_initial_name);
	}
	else return NULL;
	
}

/*******************************************************************************
reloc_file_op_to_ext~

This is an extension function for get_line. If the user presses F2, it calls
dir_window_do_select which lets the user select a destination directory.
It attempts to find a directory specified by the contents of line, and if it
exists, then the selection mode starts on this directory.
If the selection was successful, it returns the path of that directory to
get_line, which replaces the contents of the line with the new path.
*******************************************************************************/

char *reloc_file_op_to_ext(char *line,key_type key) {


	dir_info_struct *selected_dir = dir_window_get_selected_dir();
	dir_info_struct *dir;
	char *full_path;
	char *new_path = NULL;

	if (key == F_KEY(2)) {
		hide_cursor();
		info_window_input_path_pos = reloc_file_op_prompt_len+1;

		/* try and work out the dir corresponding to the given path */
		if ((full_path = get_full_path(line,reloc_file_op_base_path))) {
			dir_window_expand_to_path(get_parent_path(full_path),TRUE);
			if ((dir = dir_find_from_path(full_path,dir_window_rootdir)))
				selected_dir = dir;
			free(full_path);
		}
		
		/* do the selection */
		selected_dir = dir_window_do_select(selected_dir);
		
		info_window_update_path(reloc_file_op_base_path);
		if (current_mode == DIR_TREE_MODE) {
			file_window_resize_mini();
			dir_window_draw();
		}
		file_window_draw();
		
		/* work out the path to return based on the selection */
		if (selected_dir) new_path = dir_get_path(selected_dir);
		else new_path = (char*) strdup(line);
		if (file_window_size == FILE_WINDOW_MINI_SIZE) dir_window_draw();
		show_cursor();
	}
	return new_path;

}

/*******************************************************************************
reloc_file_op~

Handles all the interface stuff for file and directory operations that relocate
a file, or create a file in a new location. These are: move file, copy file,
link file, link dir, graft dir.
*******************************************************************************/

void reloc_file_op(
	char *prompt,                              /* e.g. "MOVE file:" */
	char *initial_name,                        /* file/dirname WITHOUT the path */
	char *initial_filename,                    /* file/dirname WITH the path */
	char *base_path,                           /* "current" path to use for initial selection and resolving relative dirs */
	char* (*op_func)(char*,char*,char*,char*)  /* function to do the actual operation */
) {

	int height, width;
	int max_display_length, max_new_name_display_length;
	char *display_name, *new_name, *input_dest_path, *dest_path;
	char *error_message = NULL;
	char *old_filename, *new_filename;
	struct stat statbuf;
	int prompt_len = strlen(prompt);
	struct stat src_statbuf;
	int op_ok = FALSE;
	
	reloc_file_op_initial_name = initial_name;
	reloc_file_op_base_path = base_path;
	reloc_file_op_prompt_len = prompt_len;
	getmaxyx(command_window,height,width);
	max_display_length = width - prompt_len - 1;
		
	display_name = truncate_string(initial_name,max_display_length);

	/* prompt the user for the new filename */
	wset_predefined_background_color(command_window,PANEL_COLOR);
	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,0,prompt);
	wset_predefined_color(command_window,TEXT_COLOR);
	mvwprintw(command_window,0,prompt_len+1,"%s",display_name);
	status_line_set_message("Enter file spec or strike enter");
	status_line_display_command_keys("{TAB} copy name  {%E} ok  {F10} cancel");
 
	if (strlen(display_name) > (width-prompt_len-5)/2)
		max_new_name_display_length = ((width-prompt_len-5)/2);
	else max_new_name_display_length = width-strlen(display_name)-prompt_len-6;
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwprintw(command_window,0,width-max_new_name_display_length-5," as ");
	wclrtoeol(command_window);
	wrefresh(command_window);
	wset_predefined_color(command_window,TEXT_COLOR);

	
	new_name = get_line(command_window,0,width-max_new_name_display_length-1,
											max_new_name_display_length,-1,"",&reloc_file_op_copy_name_ext);
	status_line_clear();
	if (new_name) {
		if (strchr(new_name,'/')) error_message = (char*) strdup("Invalid filename (contains /)"); /* filename contains '/' */
		else {
			if (!strcmp(new_name,"")) {
				/* user did not enter a new name, default to existing one */
				new_name = strdup(initial_name);
				wmove(command_window,0,width-max_new_name_display_length-1);
				waddstr(command_window,truncate_string(new_name,max_new_name_display_length));
			}
			

			wset_predefined_color(command_window,PANEL_COLOR);
			mvwprintw(command_window,1,prompt_len-3,"to: ");
			status_line_set_message("Enter destination path");
			status_line_display_command_keys("{F2} point  {%E} ok  {F10} cancel");
			wset_predefined_color(command_window,TEXT_COLOR);

			input_dest_path = get_line(command_window,1,prompt_len+1,max_display_length,-1,"",
 																&reloc_file_op_to_ext);
			status_line_clear();
			if (input_dest_path) {
				if (!strcmp(input_dest_path,"")) input_dest_path = (char*) strdup(".");
				dest_path = get_full_path(input_dest_path,base_path);
				free(input_dest_path);
				if (!dest_path) error_message = strdup("Directory does not exist or is not accessible");
				else {
					old_filename = initial_filename;
					new_filename = (char*) malloc(strlen(dest_path)+1+strlen(new_name)+1);
					if (!strcmp(dest_path,"/")) sprintf(new_filename,"/%s",new_name);
					else sprintf(new_filename,"%s/%s",dest_path,new_name);
					
					if (lstat(old_filename,&src_statbuf)) error_message = strerror(errno); /* couldn't stat src file */
					else if (files_are_same(old_filename,new_filename))
						error_message = strdup("Source and destination files are the same");
						/* thus preventing the user from "copying" a file to it's self, and losing all contents */

					else {
						if (lstat(new_filename,&statbuf)) op_ok = TRUE;  /* dest file doesn't exist */
						else if (S_ISDIR(statbuf.st_mode)) error_message = strdup("A directory with this name already exists");
						else if (ask_replace(prompt_len+1,old_filename,new_filename) == RESPONSE_YES) op_ok = TRUE; /* replace ok, dest file deleted */
					}
					if (op_ok)
						error_message = op_func(old_filename,new_filename,dest_path,new_name);
				}
	
				free(dest_path);    
			}
			free(new_name);
		}
	}
	if (error_message) status_line_error_message(error_message);
	info_window_update();
	free(display_name);
			
}



/*******************************************************************************
reloc_tagged_files_op~

Handles all the interface stuff for file and directory operations that relocate
all tagged files, or create files in a new location. These are: move tagged
files, copy tagged files, symlink tagged files.
*******************************************************************************/

void reloc_tagged_files_op(
	char *prompt,                              /* e.g. "MOVE file:" */
	char *base_path,                           /* "current" path to use for initial selection and resolving relative dirs */
	void (*op_func)(char*,int,int),            /* function to do the actual operation */
	int with_paths                             /* whether or not to prompt the user about paths */
) {

	int max_display_length;
	char *input_dest_path, *dest_path;
	char *error_message = NULL;
	int prompt_len = 8;
	int canceled = FALSE;
	int auto_replace = FALSE;
	char *as_string;
	int paths = (with_paths ? RELATIVE_PATHS : NO_PATHS);

  if (file_window_count_tagged_files(TRUE) <= 0) return;
	
	reloc_file_op_base_path = base_path;
	reloc_file_op_prompt_len = prompt_len;

	max_display_length = get_window_width(command_window) - prompt_len - 1;

			
	werase(command_window);
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwaddstr(command_window,0,0,prompt);
	wrefresh(command_window);
	wset_predefined_color(command_window,TEXT_COLOR);

	/* get file spec */
	wset_predefined_color(command_window,PANEL_COLOR);
	mvwaddstr(command_window,0,strlen(prompt)+1,"as ");
	wset_predefined_color(command_window,TEXT_COLOR);
	wrefresh(command_window);
	status_line_set_message("Just press enter here, only * works for now");
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	as_string = get_line(command_window,0,strlen(prompt)+4,max_display_length,-1,"*",NULL);
	status_line_clear();
	if (as_string && strcmp(as_string,"*")) {
		status_line_error_message("Invalid filespec");
		free(as_string);
		as_string = NULL;
	}

	/* get destination dir from user */
	if (as_string) {
		status_line_set_message("Enter destination path");
		status_line_display_command_keys("{F2} point  {%E} ok  {F10} cancel");
		wset_predefined_color(command_window,PANEL_COLOR);
		mvwprintw(command_window,1,prompt_len-3,"to: ");
		wset_predefined_color(command_window,TEXT_COLOR);
		input_dest_path = get_line(command_window,1,prompt_len+1,max_display_length,-1,"",
 															&reloc_file_op_to_ext);
		status_line_clear();
	}



	if (as_string && input_dest_path) {
		if (!strcmp(input_dest_path,"")) input_dest_path = (char*) strdup(".");
		dest_path = get_full_path(input_dest_path,base_path);
		free(input_dest_path);

		if (!dest_path) error_message = strdup("Directory does not exist or is not accessible");
		else {
			if (with_paths) {
				switch(status_line_get_response("Paths are",RESPONSE_ABSOLUTE+RESPONSE_RELATIVE+RESPONSE_CANCEL,FALSE)) {
					case RESPONSE_ABSOLUTE: paths = ABSOLUTE_PATHS; break;
					case RESPONSE_RELATIVE: paths = RELATIVE_PATHS; break;
					default: canceled = TRUE;
				}
			}
			
			if (!canceled)
				switch(status_line_get_response("Automatically replace existing files?",RESPONSE_YES+RESPONSE_NO+RESPONSE_CANCEL,TRUE)) {
					case RESPONSE_YES: auto_replace = TRUE; break;
					case RESPONSE_NO: auto_replace = FALSE; break;
					default: canceled = TRUE;
				}
		
			if (!canceled) {
				op_func(dest_path,auto_replace,paths);
			}
		}

		free(dest_path);    
	}
	if (as_string) free(as_string);
	if (error_message) status_line_error_message(error_message);
	info_window_update();
			
}



/*******************************************************************************
window_border~
*******************************************************************************/

void window_border(WINDOW *window) {

	int x, y, width, height;
	
	getmaxyx(window,height,width);
	mvwaddch(window,0,0,special_char[ULCORNER_CHAR]);
	mvwaddch(window,height-1,0,special_char[LLCORNER_CHAR]);
	mvwaddch(window,0,width-1,special_char[URCORNER_CHAR]);
	mvwaddch(window,height-1,width-1,special_char[LRCORNER_CHAR]);
	for (x = 1; x < width-1; x++) {
		mvwaddch(window,0,x,special_char[HLINE_CHAR]);
		mvwaddch(window,height-1,x,special_char[HLINE_CHAR]);
	}
	for (y = 1; y < height-1; y++) {
		mvwaddch(window,y,0,special_char[VLINE_CHAR]);
		mvwaddch(window,y,width-1,special_char[VLINE_CHAR]);
	}
	wmove(window,0,0);

}


/*******************************************************************************
get_line~

General purpose function to read a string from the user, allowing the use
of the backspace, delete, home and end keys.
This uses an area of window, input_length characters wide, located at y,x.
The maximum length and default (initial) string can be specified.
A value of -1 for max_length indicates that there is no limit placed on the
length of the string.
The user can cancel the entry by pressing F10, in which case NULL is returned.

Extension function:
 	ext_func is a pointer to a function which will recognise extra keys, and
 	act on them accordingly (e.g. F2 could select a directory).
 	Every time a key is pressed that is not recognised as a special key (like
 	backspace or delete), ext_func is called and passed the current value of the
 	line and the key that has been pressed. The value it returns is either a
 	modified value of line (in which case line is replaced), or NULL (in which case
 	line is left as is).
*******************************************************************************/

char* get_line(WINDOW* window, int y, int x, int input_length, int max_length,
 							char *default_string, char* (*ext_func)(char*,key_type)) {

	int line_pos = 0;
	int display_start = 0;
	int cur_char;
	char* line = (char*) malloc(100);
	char* return_line;
	int line_allocated = 100;
	key_type key;
	int finished = 0;
	int canceled = 0;
	char *ext_func_string;
	
	if (default_string && (strlen(default_string) > 0)) {
		strcpy(line,default_string);
	}
	else { line[0] = 0; }
	
	show_cursor();

	wmove(window,y,x);
	
	for (cur_char = display_start; (cur_char < input_length) && line[cur_char]; cur_char++)
		waddch(window,line[cur_char]);
	for (; cur_char < input_length; cur_char++) waddch(window,' ');
	wmove(window,y,x);
	wrefresh(window);
	while (!finished) {
		key = read_key();

		if (key == F_KEY(10)) {
			finished = canceled = TRUE;
		}
		else if (key == RIGHT_KEY) {
			if (line_pos < strlen(line)) {
				line_pos++;
				if (line_pos-input_length+1 > display_start) {
					display_start++;
					mvwdelch(window,y,x);
					if (line_pos < strlen(line)) mvwinsch(window,y,x+input_length-1,line[line_pos]);
					else mvwinsch(window,y,x+input_length-1,' ');
				}
				wmove(window,y,x+line_pos-display_start);
				wrefresh(window);
			}
		}
		else if (key == LEFT_KEY) {
			if (line_pos > 0) {
				line_pos--;
				if (line_pos < display_start) {
					display_start--;
					mvwdelch(window,y,x+input_length-1);
					mvwinsch(window,y,x,line[line_pos]);
				}
				wmove(window,y,x+line_pos-display_start);
				wrefresh(window);
			}
		}
		else if (key == HOME_KEY) {
			if (line_pos > 0) {
				line_pos = 0;
				if (display_start > 0) {
					display_start = 0;
					wmove(window,y,x);
					for (cur_char = display_start; (cur_char < input_length) && line[cur_char]; cur_char++)
						waddch(window,line[cur_char]);
					for (; cur_char < input_length; cur_char++) waddch(window,' ');
				}
				wmove(window,y,x);
				wrefresh(window);
			}
		}
		else if (key == END_KEY) {
			if (line_pos < strlen(line)) {
				line_pos = strlen(line);
				if (line_pos-input_length+1 > display_start) {
					display_start = line_pos-input_length+1;
					wmove(window,y,x);
					for (cur_char = display_start; (cur_char-display_start < input_length) && line[cur_char]; cur_char++)
						waddch(window,line[cur_char]);
					for (; cur_char-display_start < input_length; cur_char++) waddch(window,' ');
				}
				wmove(window,y,x+line_pos-display_start);
				wrefresh(window);
			}
		}
		else if (key == BACKSPACE_KEY) {
 		/* backspace */
			if (line_pos > 0) {
				memmove(&line[line_pos-1],&line[line_pos],strlen(line)-line_pos+1);
				line_pos--;
				if (line_pos < display_start) {
					display_start--;
				}
				else {
					mvwdelch(window,y,x+line_pos-display_start);
					if (display_start+input_length-1 < strlen(line)) mvwinsch(window,y,x+input_length-1,line[display_start+input_length-1]);
					else mvwinsch(window,y,x+input_length-1,' ');
					wmove(window,y,x+line_pos-display_start);
					wrefresh(window);
				}
				
			}
		}
		else if (key == DELETE_KEY) {
			if (line_pos < strlen(line)) {
				memmove(&line[line_pos],&line[line_pos+1],strlen(line)-line_pos);
				mvwdelch(window,y,x+line_pos-display_start);
				if (display_start+input_length-1 < strlen(line)) mvwinsch(window,y,x+input_length-1,line[display_start+input_length-1]);
				else mvwinsch(window,y,x+input_length-1,' ');
				wmove(window,y,x+line_pos-display_start);
				wrefresh(window);          
			}
		}
		else if (key == ENTER_KEY) {
			finished = 1;
		}
		else if (ext_func && (ext_func_string = ext_func(line,key))) {
			/* extension function called and returned a string */
			free(line);
			if ((max_length >= 0) && (strlen(ext_func_string) > max_length)) {
				line_allocated = max_length+1;
				line = malloc(line_allocated);
				strncpy(line,ext_func_string,max_length);
				free(ext_func_string);
			}
			else {
				line = ext_func_string;
				line_allocated = strlen(ext_func_string)+1;
			}
			line_pos = strlen(line);
			
			display_start = line_pos-input_length+1;
			if (display_start < 0) display_start = 0;
			wmove(window,y,x);
			wclrtoeol(window);

			for (cur_char = display_start; (cur_char-display_start < input_length) && line[cur_char]; cur_char++)
				waddch(window,line[cur_char]);
			wmove(window,y,x+line_pos-display_start);
			wrefresh(window);
		}
		else {
			/* another character: add to string (if possible) */
			if ((key < 256) && (key > 31) &&
					((max_length < 0) || (strlen(line) < max_length))) {
				if (strlen(line)+1 >= line_allocated) {
					line_allocated += 100;
					line = realloc(line,line_allocated);
				}
				memmove(&line[line_pos+1],&line[line_pos],strlen(line)-line_pos+1);
				line[line_pos] = key;
				mvwdelch(window,y,x+input_length-1);
				mvwinsch(window,y,x+line_pos-display_start,line[line_pos]);
				line_pos++;

				if (line_pos-input_length+1 > display_start) {
					display_start++;
					mvwdelch(window,y,x);
					if (line_pos < strlen(line)) mvwinsch(window,y,x+input_length-1,line[line_pos]);
					else mvwinsch(window,y,x+input_length-1,' ');
				}

				wmove(window,y,x+line_pos-display_start);
				wrefresh(window);
			}
		}
	}

	if (canceled) return_line = NULL;
	else {
		return_line = strdup(line);
	}
	free(line);

	hide_cursor();
	
	return return_line;  

}


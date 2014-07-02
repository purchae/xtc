/*  main.c - main source file for XTC
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

WINDOW* minifl_window;

int info_width = 20;
int bottom_height = 3;
int minifl_height = 8; /* actually set in main(), depending on screen size */

/*******************************************************************************
main_loop~
*******************************************************************************/

static void main_loop(void) {

	key_type key;
	int processed;

	while (current_mode != QUIT_MODE) {
		key = uppercase_key(read_key());
		switch(current_mode) {
			case DIR_TREE_MODE: processed = dir_window_process_key(key); break;
			case FILE_LIST_MODE: processed = file_window_process_key(key); break;
			default: processed = FALSE;
		}
		/* process global keys */
		if (!processed) switch (key) {
			case      'Q': if (confirm_quit()) current_mode = QUIT_MODE; break; /* quit */
			case F_KEY(2): config_screen_main();                         break; /* configuration */
			case ALT_KEY('S'): file_window_change_sort_order();          break; /* change sort order of file window */
			case ALT_KEY('F'): file_window_change_display_format();      break;
			case          'F': file_window_set_filespec();               break; /* set filespec */
		}
	}

}


/*******************************************************************************
main~

First command line option (argv[1]): Starting path
*******************************************************************************/

int main(int argc, char* argv[]) {

	int screen_height, screen_width;
#ifdef HAVE_DEFINE_KEY
	char key_string[10];
#endif
	char *termtype;

	config_load();
	initscr();
	if ((in_color = has_colors())) start_color();
	hide_cursor();
	raw();        /* immediate char return */
	noecho ();    /* no immediate echo */
	keypad(stdscr,TRUE);
	termtype = getenv("TERM");
	if (termtype && !strncmp(termtype,"xterm",5)) {
		/* A quick and dirty hack to get certain keys working properly in xterm */
		/* For kvt, you might need to set your terminal emulation to "Original Xterm II" */
#ifdef HAVE_DEFINE_KEY
		sprintf(key_string,"%cOH",033); define_key(key_string,KEY_HOME);
		sprintf(key_string,"%cOF",033); define_key(key_string,KEY_END);
		sprintf(key_string,"%cOj",033); define_key(key_string,'*');
		sprintf(key_string,"%cOm",033); define_key(key_string,'-');
		sprintf(key_string,"%cOM",033); define_key(key_string,KEY_ENTER);
		sprintf(key_string,"%cOk",033); define_key(key_string,'+');
#endif
	}
	
	set_special_chars(config.misc.use_ext_chars);

	getmaxyx(stdscr,screen_height,screen_width);
	if (screen_height >= 40) minifl_height = 8;
	else minifl_height = 4;
	
	
	main_window_draw();
	refresh();
	file_window_init();
	info_window_init();
	status_line_init();
	dir_window_init();

#ifdef HAVE_RESIZETERM
	(void) signal(SIGWINCH, resize_screen); /* arrange interrupts to resize */
#endif


	if (argc >= 2) dir_window_set_path(argv[1]);
	else dir_window_set_initial_path();

	
	dir_window_activate();	
	main_loop();
	
	show_cursor();
	remove_color();
	erase();
	refresh();
	noraw();
	endwin();
	return 0;
}


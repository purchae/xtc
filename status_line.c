/*  status_line.c - routines to manipulate the status line
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

int progress_bar_visible = FALSE;
int progress_bar_old_percent = 0;
static WINDOW *status_line_window;

/*******************************************************************************
status_line_init~
*******************************************************************************/

void status_line_init() {

	int screen_height, screen_width;
	
	getmaxyx(stdscr,screen_height,screen_width);
	status_line_window = newwin_force(1,screen_width,screen_height-1,0);
	wset_predefined_background_color(status_line_window,PANEL_COLOR);
	wset_predefined_color(status_line_window,PANEL_COLOR);
	werase(status_line_window);
	progress_bar_visible = FALSE;

}

/*******************************************************************************
status_line_reinit~
*******************************************************************************/

void status_line_reinit(void) {

	delwin(status_line_window);
	status_line_init();

}


/*******************************************************************************
status_line_clear~
*******************************************************************************/

void status_line_clear() {

	werase(status_line_window);
	wrefresh(status_line_window);
	progress_bar_visible = FALSE;

}

/*******************************************************************************
status_line_display_command_keys~

Displays a string at the bottom-right corner of the screen, hilighting
characters that are within {} brackets. In the case of {%E}, the enter symbol
is displayed using ACS characters.   

This probably needs to be re-written in a better way so that other % things
can be used, e.g. %C for cancel, %H for help.
*******************************************************************************/

void status_line_display_command_keys(char *keystring) {

	int height, width;
	char normal_string[strlen(keystring)+1];
	long hilight_string[strlen(keystring)+1];  /* long, so we can include ACS characters */
	int kspos, spos = 0, ppos;
	int in_hilight = FALSE;
	int start;
	
	getmaxyx(status_line_window,height,width);
	
	for (kspos = 0; kspos < strlen(keystring); kspos++) {
		if (keystring[kspos] == '{') {
			if ((kspos+3 < strlen(keystring)) && !strncmp(&keystring[kspos+1],"%E}",3)) {
				normal_string[spos] = normal_string[spos+1] = normal_string[spos+2] = ' ';
				hilight_string[spos] = special_char[LARROW_CHAR]; hilight_string[spos+1] = special_char[HLINE_CHAR]; hilight_string[spos+2] = special_char[LRCORNER_CHAR];
				spos += 3; kspos += 3;
			}
			else in_hilight = TRUE;
		}
		else if (keystring[kspos] == '}') in_hilight = FALSE;
		else {
 			if (in_hilight) hilight_string[spos] = keystring[kspos];
 			else hilight_string[spos] = 0;
 			normal_string[spos++] = keystring[kspos];
		}
	}
	
	normal_string[spos] = 0;
	
	start = width-strlen(normal_string)-1;
	wset_predefined_color(status_line_window,PANEL_COLOR);
	mvwaddstr(status_line_window,0,start,normal_string);
	wset_predefined_color(status_line_window,TEXT_COLOR);
	for (ppos = 0; ppos < strlen(normal_string); ppos++)
		if (hilight_string[ppos])
			mvwaddch(status_line_window,0,start+ppos,hilight_string[ppos]);

	wrefresh(status_line_window);

}

/*******************************************************************************
status_line_set_message~
*******************************************************************************/

void status_line_set_message(char *message) {

	wset_predefined_color(status_line_window,PANEL_COLOR);
	mvwaddstr(status_line_window,0,0,message);
	wrefresh(status_line_window);
	progress_bar_visible = FALSE;

}


/*******************************************************************************
status_line_get_response~
*******************************************************************************/

int status_line_get_response(char *prompt, int options, int cursor_shown) {

	char keystring[80] = "";
	int response = 0;
	key_type key;
	char *prompt_with_space = malloc(strlen(prompt)+2);
	
	status_line_clear();
	
	strcpy(keystring,"");

	if (!options) options = RESPONSE_OK;  
	if (options & RESPONSE_YES)       strcat(keystring,"  {Y}es");
	if (options & RESPONSE_NO)        strcat(keystring,"  {N}o");
	if (options & RESPONSE_ABSOLUTE)  strcat(keystring,"  {A}bsolute");
	if (options & RESPONSE_RELATIVE)  strcat(keystring,"  {R}elative");
	if (options & RESPONSE_RETRYFILE) strcat(keystring,"  {R}etry file");
	if (options & RESPONSE_SKIPFILE)  strcat(keystring,"  {S}kip file");
	if (options & RESPONSE_OK)        strcat(keystring,"  {%E} ok");
	if (options & RESPONSE_CANCEL)    strcat(keystring,"  {F10} cancel");
	if (options & RESPONSE_HELP)      strcat(keystring,"  {F1} help");
	
	status_line_display_command_keys(&keystring[2]); /* skip first two characters (spaces) */
	sprintf(prompt_with_space,"%s ",prompt);
	status_line_set_message(prompt_with_space);
	free(prompt_with_space);  
	if (cursor_shown) show_cursor();
	while (!response) {
		key = uppercase_key(read_key());
		if ((options & RESPONSE_YES) && (key == 'Y')) response = RESPONSE_YES;
		else if ((options & RESPONSE_NO) && (key == 'N')) response = RESPONSE_NO;
		else if ((options & RESPONSE_ABSOLUTE) && (key == 'A')) response = RESPONSE_ABSOLUTE;
		else if ((options & RESPONSE_RELATIVE) && (key == 'R')) response = RESPONSE_RELATIVE;
		else if ((options & RESPONSE_RETRYFILE) && (key == 'R')) response = RESPONSE_RETRYFILE;
		else if ((options & RESPONSE_SKIPFILE) && (key == 'S')) response = RESPONSE_SKIPFILE;
		else if ((options & RESPONSE_OK) && (key == ENTER_KEY)) response = RESPONSE_OK;
		else if ((options & RESPONSE_CANCEL) && (key == F_KEY(10))) response = RESPONSE_CANCEL;
		else if ((options & RESPONSE_HELP) && (key == F_KEY(1))) response = RESPONSE_HELP;
	}
	if (cursor_shown) hide_cursor();
	status_line_clear();

	return response;

}

/*******************************************************************************
status_line_error_message~
*******************************************************************************/

void status_line_error_message(char *error_message) {

	char *message = malloc(strlen(error_message)+8);
	
	sprintf(message,"Error: %s",error_message);
	if (config.misc.beep) beep();
	status_line_get_response(message,RESPONSE_OK,FALSE);
	free(message);

}

/*******************************************************************************
status_line_progress_bar_set~
*******************************************************************************/

void status_line_progress_bar_set(int percent) {

	int cur_char;
	
	if (percent > 100) percent = 100;
	else if (percent < 0) percent = 0;
	
	wset_predefined_color(status_line_window,PANEL_COLOR);

	if ((!progress_bar_visible) || (percent > progress_bar_old_percent)) {

		if (!progress_bar_visible) {
			mvwaddch(status_line_window,0,0,special_char[HLINE_CHAR]);
			for (cur_char = 1; cur_char <= 32; cur_char++) waddch(status_line_window,special_char[BULLET_CHAR]);
			mvwaddstr(status_line_window,0,34,"0%  ");
			progress_bar_old_percent = 0;
			progress_bar_visible = TRUE;
		}

		if (percent > progress_bar_old_percent) {
			wmove(status_line_window,0,progress_bar_old_percent/3+1);
			for (cur_char = progress_bar_old_percent/3+1; cur_char <= percent/3; cur_char++)
				waddch(status_line_window,special_char[HLINE_CHAR]);
			mvwprintw(status_line_window,0,34,"%d%%",percent);
			progress_bar_old_percent = percent;
		}

		wrefresh(status_line_window);
	}

}

/*******************************************************************************
status_line_confirm_string~

Askes the user to enter the specified string. If they make any mistakes, the
whole thing is cancelled. Used for PRUNE.
*******************************************************************************/

int status_line_confirm_string(char *string) {

	char *c = string;
	int mistake_made = FALSE;
	key_type key;

	status_line_clear();
	status_line_display_command_keys("{%E} ok  {F10} cancel");
	wset_predefined_color(status_line_window,PANEL_COLOR);
	mvwprintw(status_line_window,0,0,"Enter the word %s: ",string);

	wset_predefined_color(status_line_window,TEXT_COLOR);
	show_cursor();
	wrefresh(status_line_window);
	while (*c && !mistake_made) {
		key = uppercase_key(read_key());
		if (key != *c) mistake_made = TRUE;
		else {
			waddch(status_line_window,*c);
			wrefresh(status_line_window);
		}
		c++;
	}
	if (!mistake_made) {
		key = uppercase_key(read_key());
		if (key != ENTER_KEY) mistake_made = TRUE;
	}
	
	hide_cursor();
	status_line_clear();

	return !mistake_made;

}


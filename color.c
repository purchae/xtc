/*  color.c - color support
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

const predefined_color_struct predefined_colors[] = {{15,4,A_NORMAL},  /* FILES_COLOR */
																											{15,4,A_NORMAL}, /* DIRS_COLOR */
																											{0,7,A_REVERSE}, /* HILIGHT_COLOR */
																											{14,4,A_NORMAL}, /*  LINES_COLOR */
																											{15,4,A_NORMAL}, /* TEXT_COLOR */
																											{14,4,A_NORMAL}, /* PANEL_COLOR */
																											{0,7,A_NORMAL},  /* INPUT_COLOR */
																											{11,4,A_NORMAL}, /* SLINK_COLOR */
																											{7,0,A_NORMAL},  /* CONFIG_LO_COLOR */
																											{15,0,A_NORMAL},  /* CONFIG_HI_COLOR */
																											{0,7,A_NORMAL}  /* CONFIG_HILIGHT_COLOR */
 																										};

long special_char[LAST_CHAR];

int in_color;


/*******************************************************************************
wset_color~
*******************************************************************************/

void wset_color(WINDOW *window, short fg, short bg) {

	int bold = (fg >= 8);
	fg = (fg % 8);
	bg = (bg % 8);
	init_pair(fg*8+bg,fg,bg);
	if (bold) wattrset(window,COLOR_PAIR((fg*8+bg)) | A_BOLD);
	else wattrset(window,COLOR_PAIR((fg*8+bg)));
}

/*******************************************************************************
set_color~
*******************************************************************************/

void set_color(short fg, short bg) {

	wset_color(stdscr,fg,bg);

}

/*******************************************************************************
wset_background_color~
*******************************************************************************/

void wset_background_color(WINDOW *window, short bg) {

	init_pair(56+bg,7,bg);
	wbkgdset(window,COLOR_PAIR((56+bg)));

}

/*******************************************************************************
wset_predefined_color~
*******************************************************************************/

void wset_predefined_color(WINDOW *window, int colornum) {

	if (in_color) wset_color(window,predefined_colors[colornum].fg,predefined_colors[colornum].bg);
	else wattrset(window,predefined_colors[colornum].bw_attrs);
	
}

/*******************************************************************************
wset_predefined_background_color~
*******************************************************************************/

void wset_predefined_background_color(WINDOW *window, int colornum) {

	if (in_color) wset_background_color(window,predefined_colors[colornum].bg);
	else wbkgdset(window,predefined_colors[colornum].bw_attrs);
	
}

/*******************************************************************************
remove_color~
*******************************************************************************/

void remove_color() {

	attrset(A_NORMAL);
	init_pair(56,7,0);
	wbkgdset(stdscr,COLOR_PAIR(56));

}

/*******************************************************************************
init_special_chars~

This is a bit messy and probably needs to be improved.
*******************************************************************************/

void set_special_chars(int use_acs) {

	if (use_acs) {
		special_char[HLINE_CHAR] = ACS_HLINE;
		special_char[VLINE_CHAR] = ACS_VLINE;
		special_char[ULCORNER_CHAR] = ACS_ULCORNER;
		special_char[URCORNER_CHAR] = ACS_URCORNER;
		special_char[LLCORNER_CHAR] = ACS_LLCORNER;
		special_char[LRCORNER_CHAR] = ACS_LRCORNER;
		special_char[TTEE_CHAR] = ACS_TTEE;
		special_char[BTEE_CHAR] = ACS_BTEE;
		special_char[LTEE_CHAR] = ACS_LTEE;
		special_char[RTEE_CHAR] = ACS_RTEE;
		special_char[DIAMOND_CHAR] = ACS_DIAMOND;
		special_char[BULLET_CHAR] = ACS_BULLET;
		special_char[LARROW_CHAR] = ACS_LARROW;
		special_char[RARROW_CHAR] = ACS_RARROW;
	}
	else {
		special_char[HLINE_CHAR] = '-';
		special_char[VLINE_CHAR] = '|';
		special_char[ULCORNER_CHAR] = '-';
		special_char[URCORNER_CHAR] = '-';
		special_char[LLCORNER_CHAR] = '-';
		special_char[LRCORNER_CHAR] = '-';
		special_char[TTEE_CHAR] = '-';
		special_char[BTEE_CHAR] = '-';
		special_char[LTEE_CHAR] = '|';
		special_char[RTEE_CHAR] = '|';
		special_char[DIAMOND_CHAR] = '*';
		special_char[BULLET_CHAR] = '.';
		special_char[LARROW_CHAR] = '<';
		special_char[RARROW_CHAR] = '>';
	}

}

/*  key.c - key handling routines and definitions
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
read_key~
*******************************************************************************/

key_type read_key(void) {

	key_type key = 0;
	short ch;
	
	ch = getch();
	if (ch == ERR) key = NO_KEY;
	else if ((ch >= 32) && (ch < 127) /*&& (ch != '[')*/ && (ch != '`')) key = ch;
	else if ((ch >= 128) && (ch < 256)) key = ALT_KEY(ch-0x80);
	else if ((ch >= KEY_F0) && (ch <= KEY_F(12))) key = F_KEY(ch-KEY_F0);
	else if ((ch == '[') || (ch == 27)) {  /* start of escape sequence */
		ch = getch();
		if ((ch != '[') && (ch != 0x27)) {  /* ALT key */
			key = ALT_KEY(ch);
		}
	}
	else if (ch == '`') {  /* CTRL key */
		ch = getch();
		if ((ch < 256) && isalpha(ch)) {
			ch = toupper(ch);
			key = CTRL_KEY(ch);
		}
		else key = NO_KEY;
	}
	else
		switch(ch) {
			case '\n':          key = ENTER_KEY;     break;
			case KEY_UP:        key = UP_KEY;        break;
			case KEY_DOWN:      key = DOWN_KEY;      break;
			case KEY_LEFT:      key = LEFT_KEY;      break;
			case KEY_RIGHT:     key = RIGHT_KEY;     break;
			case KEY_PPAGE:     key = PGUP_KEY;      break;
			case KEY_NPAGE:     key = PGDN_KEY;      break;
			case KEY_HOME:      key = HOME_KEY;      break;
			case KEY_END:       key = END_KEY;       break;
			case KEY_DC:        key = DELETE_KEY;    break;
			case 127:           key = DELETE_KEY;    break;
			case KEY_BACKSPACE: key = BACKSPACE_KEY;    break;
			case 9:             key = TAB_KEY;    break;
			default:
				if ((ch > 0) && (ch <= 26)) key = ch; /* CTRL keys */
		}
	
	return key;

}

/*******************************************************************************
uppercase_key~

Returns the uppercase value of a key, if it is a letter.
*******************************************************************************/

key_type uppercase_key(key_type key) {

	if (((key >= 'a') && (key <= 'z')) ||
			((key >= ALT_KEY('a')) && (key <= ALT_KEY('z')))) {
		key += 'A'-'a';
	}
	return key;

}

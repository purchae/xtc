/*  filespec.c - routines to handle file specification manipulation and matching
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

typedef struct _filespec_struct filespec_struct;

struct _filespec_struct {
  char *pattern;
  filespec_struct *next;
};

struct _filespec_list_struct {
	filespec_struct *items;
	filespec_struct *last_item;
};

filespec_list_struct *filespec_list = NULL;

/*******************************************************************************
filespec_list_create~
*******************************************************************************/

filespec_list_struct *filespec_list_create(void) {

	filespec_list_struct *list = malloc(sizeof(filespec_list_struct));
	
	list->items = NULL;
	list->last_item = NULL;
	return list;

}

/*******************************************************************************
filespec_list_clear~
*******************************************************************************/

void filespec_list_clear(filespec_list_struct *list) {
  filespec_struct *filespec, *next_filespec;
	
  filespec = list->items;
	while (filespec) {
		next_filespec = filespec->next;
		free(filespec->pattern);
		free(filespec);
		filespec = next_filespec;
	}
	list->items = NULL;
	list->last_item = NULL;

}

/*******************************************************************************
filespec_list_add_filespec~
*******************************************************************************/

static void filespec_list_add_filespec(filespec_list_struct *list,
																			 filespec_struct *filespec) {

	if (list->last_item) list->last_item->next = filespec;
	else list->items = filespec;
	list->last_item = filespec;

}

/*******************************************************************************
filespec_create~
*******************************************************************************/

static filespec_struct *filespec_create(char *pattern) {

	filespec_struct *filespec;
	
	filespec = malloc(sizeof(filespec));
	filespec->pattern = strdup(pattern);
	filespec->next = NULL;
	
	return filespec;

}

/*******************************************************************************
filespec_list_set~

Sets the filespecs according to a comma-separated string of patterns.
*******************************************************************************/

void filespec_list_set(filespec_list_struct *list, char *filespec_string) {

	int pos = 0;
	int pattern_start;  /* starting position of the current pattern */
	char *pattern;
	
	filespec_list_clear(list);
	while ((pos < strlen(filespec_string)) && (filespec_string[pos] == ' '))
		pos++;	/* skip spaces before start of next pattern */
	pattern_start = pos;
	while (pos <= strlen(filespec_string)) {
	  if (((pos == strlen(filespec_string)) || (filespec_string[pos] == ',')) &&
				(pos > pattern_start) && (filespec_string[pattern_start] != ',')) {
			pattern = malloc(pos-pattern_start+1);
			strncpy(pattern,&filespec_string[pattern_start],pos-pattern_start);
			pattern[pos-pattern_start] = 0;
			filespec_list_add_filespec(list,filespec_create(pattern));
			pos++;
			while ((pos < strlen(filespec_string)) && (filespec_string[pos] == ' '))
				pos++;	/* skip spaces before start of next pattern */
			pattern_start = pos;
		}
		else pos++;
	}
	
}

/*******************************************************************************
filespec_list_check_match~
*******************************************************************************/

int filespec_list_check_match(filespec_list_struct *list, char *filename) {

  filespec_struct *filespec;
	int found_match = FALSE;
	
	if (list->items == NULL) return TRUE;  /* no items in filespec list, all files match */
	else {
		/* test - match all filenames that start with 'c' */
		filespec = list->items;
		while (filespec && !found_match) {
			if (!fnmatch(filespec->pattern,filename,0)) found_match = TRUE;
			filespec = filespec->next;
		}
		return found_match;
	}

}

/*******************************************************************************
filespec_list_to_string~

Returns a comma-separated string containing all filespecs in the list.
If the list is empty, the string will be "*".
*******************************************************************************/

char *filespec_list_to_string(filespec_list_struct *list, char *separator) {

	int length = 0;
	filespec_struct *filespec;
	char *filespec_string;

	if (!list->items) return strdup("*");

	for (filespec = list->items; filespec; filespec = filespec->next) {
		length += strlen(filespec->pattern)+strlen(separator);
	}
	filespec_string = (char*) malloc(length+1);
	strcpy(filespec_string,"");
	for (filespec = list->items; filespec; filespec = filespec->next) {
		strcat(filespec_string,filespec->pattern);
		if (filespec->next)	strcat(filespec_string,separator);
	}
	return filespec_string;

}





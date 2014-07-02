/*  ini.c - routines to manipulate windows-style INI files
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

/*

An INI file is in the same format that windows uses, that is it contains a
number of sections, each of which contain a number of keys and values. An
example of an INI file is this:

[Section]
key=value

[Another section]
another key=another value
more keys=more values

Before manipulating an ini file, it must be loaded into memory with ini_load()
or a new one must be created with ini_new(). Using the resulting data structure,
key can be read with ini_key_get_string() and ini_key_get_value() and "set"
using ini_key_set_string() and ini_key_set_int(). Note that all this data is
read from and written to memory only, and to update the ini file you must use
ini_save(). The structure of the saved file is not preserved.

The only distinction between string values and int values is the way they are
handled. ini_key_get_string() will return the value as a string, and
ini_key_get_int() will return the same value as a integer (if the value is not
an integer, 0 is returned).

The section names, keys and values manipulated by these functions are all case
sensitive. Comments within the ini file are not supported. Any whitespace
directly before or after the = symbol is treated as part of the key or value
(depending on which side it is on).

*/

#include "common.h"

#define MAX_SECTION_NAME_LEN 100
#define MAX_KEY_LEN 100
#define MAX_VALUE_LEN 1000


typedef struct _ini_key_struct ini_key_struct;
struct _ini_key_struct {
  char *key;
	char *value;
	ini_key_struct *next;
};

typedef struct _ini_section_struct ini_section_struct;
struct _ini_section_struct {
  char *name;
  ini_key_struct *keys;
  ini_key_struct *last_key;
	ini_section_struct *next;
};

struct _ini_struct {
  char *filename;
	ini_section_struct *sections;
	ini_section_struct *last_section;
};



/*******************************************************************************
ini_section_new~
*******************************************************************************/

static ini_section_struct *ini_section_new(const char *name) {
  ini_section_struct *ini_section;
	ini_section = malloc(sizeof(ini_section_struct));
	ini_section->name = strdup(name);
	ini_section->keys = NULL;
	ini_section->last_key = NULL;
	ini_section->next = NULL;
	return ini_section;
}

/*******************************************************************************
ini_section_find~

Returns a pointer to the specified section, if one exists in ini, otherwise
NULL.
*******************************************************************************/

static ini_section_struct *ini_section_find(ini_struct *ini, const char *name) {
  ini_section_struct *ini_section = ini->sections;
	while (ini_section) {
	  if (!strcmp(ini_section->name,name)) return ini_section;
		ini_section = ini_section->next;
	}
	return NULL;
}

/*******************************************************************************
ini_section_find_or_add~
*******************************************************************************/

static ini_section_struct *ini_section_find_or_add(ini_struct *ini, const char *name) {
  ini_section_struct *ini_section = ini_section_find(ini,name);
	if (!ini_section) {
	  ini_section = ini_section_new(name);
		if (ini->last_section) ini->last_section->next = ini_section;
	  else ini->sections = ini_section;
		ini->last_section = ini_section;
	}
	return ini_section;
}

/*******************************************************************************
ini_key_new~
*******************************************************************************/

static ini_key_struct *ini_key_new(const char *key, const char *value) {
  ini_key_struct *ini_key = malloc(sizeof(ini_key_struct));

	ini_key->key = strdup(key);
	ini_key->value = strdup(value);
	ini_key->next = NULL;
	return ini_key;
}

/*******************************************************************************
ini_key_find~
*******************************************************************************/

static ini_key_struct *ini_key_find(ini_struct *ini, ini_section_struct *ini_section, const char *key) {
  ini_key_struct *ini_key = ini_section->keys;
	while (ini_key) {
	  if (!strcmp(ini_key->key,key)) return ini_key;
		ini_key = ini_key->next;
	}
	return NULL;
}











/*******************************************************************************
ini_key_set_string~
*******************************************************************************/

void ini_key_set_string(ini_struct *ini, const char *section, const char *key, const char *value) {
  ini_section_struct *ini_section = ini_section_find_or_add(ini,section);
	ini_key_struct *ini_key;
	
	if (ini_section) { /* should always be true */
		ini_key = ini_key_find(ini,ini_section,key);
		if (ini_key) {
		  free(ini_key->key);
			free(ini_key->value);
			ini_key->key = strdup(key);
			ini_key->value = strdup(value);
		}
		else {
		  ini_key = ini_key_new(key,value);
			if (ini_section->last_key) ini_section->last_key->next = ini_key;
			else ini_section->keys = ini_key;
			ini_section->last_key = ini_key;
		}
	}

};

/*******************************************************************************
ini_key_set_int~
*******************************************************************************/

void ini_key_set_int(ini_struct *ini, const char *section, const char *key, const int value) {
  char value_str[30];
	sprintf(value_str,"%d",value);
	ini_key_set_string(ini,section,key,value_str);
};

/*******************************************************************************
ini_key_get_string~
*******************************************************************************/

char *ini_key_get_string(ini_struct *ini, const char *section, const char *key, const char *default_value) {
  ini_section_struct *ini_section = ini_section_find(ini,section);
	ini_key_struct *ini_key;
	
	if ((ini_section) && (ini_key = ini_key_find(ini,ini_section,key)))
	  return strdup(ini_key->value);		
	else if (default_value == NULL) return NULL;
	else return strdup(default_value);
	
}

/*******************************************************************************
ini_key_get_int~
*******************************************************************************/

int ini_key_get_int(ini_struct *ini, const char *section, const char *key, const int default_value) {
  char default_value_str[30];
	char *value_str;

	sprintf(default_value_str,"%d",default_value);
	value_str = ini_key_get_string(ini,section,key,default_value_str);
  if ((strlen(value_str) > 0) &&	
			(((value_str[0] == '-') && is_number(&value_str[1])) ||
			is_number(value_str)))
		return atoi(value_str);
	else return default_value;
}

/*******************************************************************************
ini_key_delete~
*******************************************************************************/

void ini_key_delete(ini_struct *ini, const char *section, const char *key) {
  ini_section_struct *ini_section = ini_section_find(ini,section);
	ini_key_struct *ini_key, *del_key = NULL;
	
	if (ini_section && ini_section->keys) {
		ini_key = ini_section->keys;
	  if (!strcmp(ini_key->key,key)) { /* first key in section */
			del_key = ini_key;
			ini_section->keys = del_key->next;
		}
		else {
			while (ini_key && !del_key) {
				if (ini_key->next && !strcmp(ini_key->next->key,key)) {
					del_key = ini_key->next;
					ini_key->next = del_key->next;
				}
				ini_key = ini_key->next;
			}
		}
		if (del_key) {
		  free(del_key->key);
			free(del_key->value);
			free(del_key);
		}
	}

}

/*******************************************************************************
ini_section_delete~
*******************************************************************************/

void ini_section_delete(ini_struct *ini, const char *section) {
  ini_section_struct *ini_section = ini->sections, *del_section = NULL;
	ini_key_struct *ini_key, *next_key;
	
	if (ini_section) {
	  if (!strcmp(ini_section->name,section)) { /* first section */
			del_section = ini_section;
			ini->sections = del_section->next;
		}
		else {
			while (ini_section && !del_section) {
				if (ini_section->next && !strcmp(ini_section->next->name,section)) {
					del_section = ini_section->next;
					ini_section->next = del_section->next;
				}
				ini_section = ini_section->next;
			}
		}
		if (del_section) {
			ini_key = del_section->keys;
			while (ini_key) {
				next_key = ini_key->next;
				free(ini_key->key);
				free(ini_key->value);
				free(ini_key);
				ini_key = next_key;
			}
		  free(del_section->name);
			free(del_section);
		}
	}

}

/*******************************************************************************
ini_load~
*******************************************************************************/

ini_struct *ini_load(const char *filename) {
  FILE *file;
	ini_struct *ini;
	int c;
	enum {ST_START_OF_LINE,ST_SECTION_NAME,ST_KEY,ST_VALUE,ST_WAIT_FOR_EOL};
	int state = ST_START_OF_LINE;
  char section_name[MAX_SECTION_NAME_LEN+1];
	char key[MAX_KEY_LEN+1];
	char value[MAX_VALUE_LEN+1];
	int section_name_len = 0, key_len = 0, value_len = 0;

  section_name[0] = 0;
  file = fopen(filename,"r");
	if (!file) return NULL; /* calling function can check errno to get error */

  ini = ini_new();
  while ((c = fgetc(file)) != EOF) {
		switch (state) {
			case ST_START_OF_LINE:
			  if (c == '[') {
				  state = ST_SECTION_NAME;
					section_name[0] = 0;
					section_name_len = 0;
				}
				else if (c == '=') state = ST_WAIT_FOR_EOL;  /* skip empty keys */
				else if (c != '\n') {
				  state = ST_KEY;
					sprintf(key,"%c",c);
					key_len = 1;
					value[0] = 0;
					value_len = 0;
				}
			  break;
			case ST_SECTION_NAME:
			  if ((section_name_len >= MAX_SECTION_NAME_LEN) || (c == ']')) {
					state = ST_WAIT_FOR_EOL; /* ignore anything after the ] */
					ini_section_find_or_add(ini,section_name);
				}
				else if (c == '\n') {
					state = ST_START_OF_LINE;
					ini_section_find_or_add(ini,section_name);
				}
				else {
				  section_name[section_name_len+1] = 0;
					section_name[section_name_len] = c;
					section_name_len++;
				}
			  break;
			case ST_KEY:
			  if (key_len >= MAX_KEY_LEN) state = ST_WAIT_FOR_EOL;
				else if (c == '\n') state = ST_START_OF_LINE;
				else if (c == '=') {
				  state = ST_VALUE;
				}
				else {
				  key[key_len+1] = 0;
					key[key_len] = c;
					key_len++;
				}
				break;
			case ST_VALUE:
				if (c == '\n') {
				  state = ST_START_OF_LINE;
					if ((strlen(section_name) > 0) && (strlen(key) > 0))
						ini_key_set_string(ini,section_name,key,value);
				}
				else {
				  value[value_len+1] = 0;
					value[value_len] = c;
					value_len++;
				}
				break;
			case ST_WAIT_FOR_EOL:
			  if (c == '\n') state = ST_START_OF_LINE;
				break;
			default:
		}
	}
  fclose(file);
	return ini;

}

/*******************************************************************************
ini_new~
*******************************************************************************/

ini_struct *ini_new(void) {
  ini_struct *ini = malloc(sizeof(ini_struct));
	ini->filename = NULL;
	ini->sections = NULL;
	ini->last_section = NULL;
  return ini;
}

/*******************************************************************************
ini_save~

Returns 0 on success, -1 on error (and errno is set)
*******************************************************************************/

int ini_save(ini_struct *ini, const char *filename) {
  ini_section_struct *ini_section;
	ini_key_struct *ini_key;
  FILE *file;
	
  file = fopen(filename,"w");
	if (!file) return -1; /* calling function can check errno to get error */

	ini_section = ini->sections;
	while (ini_section) {
	  fprintf(file,"[%s]\n",ini_section->name);
		ini_key = ini_section->keys;
		while (ini_key) {
		  fprintf(file,"%s=%s\n",ini_key->key,ini_key_get_string(ini,ini_section->name,ini_key->key,"default"));
			ini_key = ini_key->next;
		}
		if (ini_section->next) fprintf(file,"\n");
		ini_section = ini_section->next;
	}
	fclose(file);
	return 0;

}

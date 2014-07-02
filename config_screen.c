/*  config_screen.c - configuration screen
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

#define NUM_VALUES 10     /* number of values allocate for a menu, ie. max. that can be used */
#define NUM_ITEMS 30      /* number of items to allocate for a menu, ie. max. that can be used */
#define NUM_SECTIONS 20   /* number of sections to allocate for a menu, ie. max. that can be used */

struct config_menu_item {
  char text[80];
	char ini_key[80];  /* key name to be used in an INI file */
	char hint[80];
	int num_values; /* This indicates the number of possible values the item may have. These
										 values must be specified in the array values.
										 If this number is 0, the value is a user-supplied string. In this case,
										 values[0] speifies the default value, values[1] specifies the string
										 displayed if the value is an empty string and values[2] specifies the
										 prompt given to the user when entering a value */
	char *values[NUM_VALUES];
	char *ini_values[NUM_VALUES];  /* values corresponding to above array that are written to the ini file */
};

struct config_menu_section {
  char text[80];
	char ini_section[80];  /* name of this section in the ini file */
	int num_items;
	struct config_menu_item items[NUM_ITEMS];
};

struct config_menu {
  int num_sections;
	struct config_menu_section sections[NUM_SECTIONS];
};

static const struct config_menu config_menu =
{4,{
	{"File Window","file_window",7,{
		{"Initial display format","display_format",
			"Specify which details are to be displayed in the file window",
			3,{"NORMAL","DATE","ATTRIBUTES"},{"NORMAL","DATE","ATTRIBUTES"}},
		{"Initial sort criteria","sort_criteria",
			"Select the sort criteria to be used in the initial file window display.",
			4,{"NAME","EXTENSION","DATE & TIME","SIZE"},{"name","ext","date","size"}},
		{"Initial sort order","sort_desc",
			"Select the sort order to be used in the initial file window display.",
			2,{"DESCENDING","ASCENDING"},{"1","0"}},
		{"Initial sort by path in showall/branch","sort_path",
			"Select \"YES\" if the Showall display files are to be listed in path order.",
			2,{"NO","YES"},{"0","1"}},
		{"Initial sort is case sensitive","sort_case",
			"Select \"YES\" for sort order to be case sensitive.",
			2,{"NO","YES"},{"0","1"}},
		{"Small file window access","small_file_window",
			"Select if the highlight bar can be moved to the small file window.",
			2,{"SELECTABLE","BYPASSED"},{"1","0"}},
		{"Number of columns","cols",
			"Select the number of columns to be displayed in the file window.",
			7,{"AUTO","1","2","3","4","5","6"},{"0","1","2","3","4","5","6"}}
	}},
	{"Directory Window","dir_window",1,{
		{"Initial highlighted directory","initial_dir",
			"Select where the highlight bar in the initial directory tree is positioned.",
			3,{"CURRENT DIRECTORY","HOME DIRECTORY","ROOT"},{"CURRENT","HOME","ROOT"}}
	}},
	{"Miscellaneous","misc",5,{
		{"Skip Quit command prompt","skip_quit_prompt",
		"Select whether or not the Quit command should bypass the dialog prompt.",
		2,{"NO","YES"},{"0","1"}},
		{"Audible error indicator","beep",
			"Select whether or not errors cause an audible alarm.",
		2,{"ON","OFF"},{"1","0"}},
		{"Date format","date_format",
			"Select Month-Day-Year or Day-Month-Year date format.",
		2,{"MM-DD-YYYY","DD-MM-YYYY"},{"%m-%d-%Y","%d-%m-%Y"}},
		{"Time format","time_format",
			"Select am/pm or 24 hour time format.",
		2,{"13:00:00","1:00:00 pm"},{"%H:%M:%S","%I:%M:%S %P"}},
		{"Use extended characters for lines","use_ext_chars",
			"Select \"NO\" to use plain ASCII characters for borders and lines",
		2,{"YES","NO"},{"1","0"}}
	}},
	{"External programs","external_programs",2,{
	  {"Editor","editor",
			"Program used to edit files.",
			0,{"","(internal)","Enter the command to start your preferred text editor program"},{}},
		{"Viewer","viewer",
			"Program used to view files",
			0,{"","(internal)","Enter the command to start your preferred viewer program"},{}}
	}}
}};


/*

The following commented-out lines are test data only, and are handy
for testing the configuration screen with a lot of options, since
at the moment there aren't a lot of real options.

static const struct config_menu config_menu =
{13,{
 {"Application Menu","app_menu",2,{
  {"Opening screen is the Application Menu","open_with_app_menu",
	 "Select \"YES\" to start XTreeGold with the Application Menu.",
	 2,{"NO","YES"},{"0","1"}},
  {"Pause after application program execution","pause_after_exec",
	 "Select \"NO\" to bypass the \"strike any key\" message when returning to Gold.",
	 2,{"YES","NO"},{"1","0"}}
 }},
 {"Directories","directories",1,{
  {"Editor program","editor",
	 "Enter the full path and file name of your preferred text editor program.",
	 0,{"pico -w","(internal)","Enter the command to start your editor"},{}}
 }},
 {"Disk logging","disk_logging",3,{
  {"Disk logging method","log_quick",
	 "Select \"STANDARD\" if disk errors are reported during disk logging.",
	 2,{"QUICK","STANDARD"},{"1","0"}},
	{"Log disk commands only read the root directory","log_root",
	 "Select \"YES\" to only log the root directory.",
	 2,{"YES","NO"},{"1","0"}},
	{"Log disk commands only read the directory tree","log_tree",
	 "Select \"YES\" to only log directorys and use alternate commands to log files.",
	 2,{"NO","YES"},{"0","1"}}
 }},
 {"Display","display",5,{
  {"Display monitor type","color_display",
	 "Select \"MONOCHROME\" if you have a Composite, LCD or \"Paper White\" monitor.",
	 2,{"COLOR","MONOCHROME"},{"1","0"}},
	{"Display is \"flicker free\"","flicker_free",
	 "Select \"NO\" if your display exhibits \"snow\" during screen updates.",
	 2,{"YES","NO"},{"1","0"}},
	{"EGA 43 line or VGA 51 line display mode","ega_43_mode",
	 "If you have an EGA/VGA display, select \"ON\" to use 43/51 line display mode.",
	 2,{"OFF","ON"},{"0","1"}},
	{"EGA/VGA cursor underline shape","alternate_cursor",
	 "Select \"ALTERNATE\" if the EGA cursor does not display properly.",
	 2,{"STANDARD","ALTERNATE"},{"0","1"}},
	{"Enable high intensity DOS EGA background colors","high_intensity",
	 "Select \"YES\" to disable the EGA blinking attribute for DOS programs.",
	 2,{"NO","YES"},{"0","1"}}
 }},
 {"File Window","file_window",8,{
  {"File name separator","separator_space",
	 "Select the character to be used as the file name separator.",
	 2,{"\".\"","\" \""},{"0","1"}},
	{"File type detection when viewing files","autodetect",
	 "Select \"MANUAL\" if you want to select the format for each file you View.",
	 2,{"AUTOMATIC","MANUAL"},{"1","0"}},
	{"Initial number of display columns","num_columns",
	 "Select the number of columns initially displayed in the file window.",
	 3,{"THREE COLUMNS","ONE COLUMN","TWO COLUMNS"},{"3","2","1"}},
	{"Initial sort criteria","initial_sort_criteria",
	 "Select the sort criteria to be used in the initial file window display.",
	 5,{"NAME","EXTENSION","DATE","SIZE","UNSORTED"},{"name","ext","date","size","unsorted"}},
	{"Initial sort order","initial_sort_order_ascending",
	 "Select the sort order to be used in the initial file window display.",
	 2,{"ASCENDING","DESCENDING"},{"1","0"}},
	{"Initial sort by path in showall","initial_sort_path",
	 "Select \"YES\" if the Showall display files are to be listed in path order.",
	 2,{"NO","YES"},{"0","1"}},
	{"Skip Alt-Copy/Move sorting by path","skip_cm_path_sort",
	 "Select \"YES\" to skip sorting the file display by path for Alt-Copy/Move.",
	 2,{"NO","YES"},{"0","1"}},
	{"Small file window access","small_file_window",
	 "Select if the highlight bar can be moved to the small file window.",
	 2,{"SELECTABLE","BYPASSED"},{"1","0"}}
 }},
 {"File and Tree Windows","file_tree_window",2,{
  {"File and tree displays are lower case","lower_case",
	 "Select \"YES\" if you want the file and tree windows displayed in lower case.",
	 2,{"NO","YES"},{"0","1"}},
	{"Mouse scroll bar display","mouse_scrollbar",
	 "Select if scroll bars or lines and arrows are to be displayed.",
	 2,{"SCROLL BAR","ARROWS"},{"0","1"}}
 }},
 {"International","international",3,{
  {"Date format","date_format",
	 "Select Month-Day-Year or Day-Month-Year date format.",
	 2,{"MM-DD-YY","DD-MM-YY"},{"MM-DD-YY","DD-MM-YY"}},
	{"Time format","time_format_24hr",
	 "Select am/pm or 24 hour time format.",
	 2,{"1:00:00 pm","13:00:00"},{"0","1"}},
	{"Numeric format","number_format_comma",
	 "Select commas or periods for number formatting.",
	 2,{"1,234,567","1.234.567"},{"1","0"}}
 }},
 {"Memory Utilization","memory",3,{
  {"Application Menu programs","app_menu",
	 "Select \"ALL MEMORY\" if your programs display \"out of memory\" messages.",
	 2,{"ALL MEMORY","AVAILABLE MEMORY"},{"1","0"}},
	{"Mouse double-click on file name","double_click",
	 "\"AVAILABLE MEMORY\" executes Open, \"ALL MEMORY\" executes Alt/Open.",
	 2,{"ALL MEMORY","AVAILABLE MEMORY"},{"1","0"}},
	{"Text editor","editor",
	 "Select \"ALL MEMORY\" if the configured text editor requires it.",
	 2,{"AVAILABLE MEMORY","ALL MEMORY"},{"0","1"}}
 }},
 {"Miscellaneous","misc",7,{
  {"Archive file attribute on copied files","archive_copied",
	 "For Copy command, select how the destination file archive attribute is set.",
	 2,{"COPIED","ONLY ARCHIVE SET"},{"1","0"}},
	{"Audible error indicator","beep",
	 "Select whether or not errors cause an audible alarm.",
	 2,{"ON","OFF"},{"1","0"}},
	{"Ignore error message for missing overlays","ignore_overlay_error",
	 "Select \"YES\" to bypass missing overlay error messages.",
	 2,{"ON","OFF"},{"1","0"}},
	{"Skip Edit command prompt","skip_edit_prompt",
	 "Select whether or not the Edit command should bypass the dialog prompt.",
	 2,{"NO","YES"},{"0","1"}},
	{"Skip Quit command prompt","skip_quit_prompt",
	 "Select whether or not the Quit command should bypass the dialog prompt.",
	 2,{"NO","YES"},{"0","1"}},
	{"Pulldown help is automatic","pulldown_help_auto",
	 "Select whether or not the pulldown help lines are always displayed.",
	 2,{"YES","NO"},{"1","0"}},
	{"Pulldown items flash when selected","pulldown_flash",
	 "Select whether or not pulldown items flash on and off when they are selcted.",
	 2,{"YES","NO"},{"1","0"}}
 }},
 {"Mouse","mouse",1,{
  {"Acceleration","high_acceleration",
	 "Select \"HIGH\" if the Mouse pointer speed seems too slow.",
	 2,{"STANDARD","HIGH"},{"0","1"}}
 }},
 {"Printer","printer",2,{
  {"Printer bit 7 character masking","printer_7bit",
	 "Select \"YES\" if your printer cannot print graphics characters.",
	 2,{"NO","YES"},{"0","1"}},
	{"Printer tab expansion","tab_expansion",
	 "Enter the Tab field width in columns, or \"0\" to print Tab characters.",
	 0,{"","not specified","Enter a value"},{}}
 }},
 {"Security","security",3,{
  {"Allow modifications to Application Menu","appmenu","",
	 2,{"YES","NO"},{"1","0"}},
	{"Allow modifications to files in hex view mode","hex_editor","",
	 2,{"YES","NO"},{"1","0"}},
	{"System/Hidden file and directory access","system_hidden_access","",
	 2,{"YES","NO"},{"1","0"}}
 }},
 {"Tree Window","tree_window",3,{
  {"Initial highlighted directory","initial_dir",
	 "Select where the highlight bar in the initial directory tree is positioned.",
	 2,{"CURRENT DIRECTORY","ROOT"},{"cwd","root"}},
	{"Show actual path for SUBST drives","actual_subst_path",
	 "Select \"YES\" if the substituted drive path should be shown on the Path line.",
	 2,{"YES","NO"},{"1","0"}},
	{"Tree window highlight bar","tree_hilight_bar_scrolling",
	 "Select \"SCROLLING\" to have the cursor move, \"FIXED\" to have the tree move.",
	 2,{"SCROLLING","FIXED"},{"1","0"}}
 }}
}};*/

struct menu_line {
  enum {SECTION_LINE,ITEM_LINE,EMPTY_LINE} line_type;
	char shortcut;
	union {
		int index;
		char *string;
	} value;
	struct {
		struct config_menu_section *section;
		struct config_menu_item *item;
	} contents; /* eventually need to remove section and item out of this struct */
};

struct menu_page {
	int num_lines;
	int changed;
	struct menu_line *menu_lines;
	struct menu_page *next, *prev;
};

enum {
	NEXT_BUTTON,
	MAIN_BUTTON,
	PREV_BUTTON
};

static WINDOW *config_screen_window, *menu_window, *hint_window, *title_window,
							*prompt_window;

/*******************************************************************************
config_screen_set_hint~

Sets the hint displayed at the bottom of the screen.
*******************************************************************************/

static void config_screen_set_hint(char *hint) {
	wset_predefined_background_color(hint_window,CONFIG_LO_COLOR);
	wset_predefined_color(hint_window,CONFIG_LO_COLOR);
  werase(hint_window);
	mvwaddstr(hint_window,0,0,hint);
	wrefresh(hint_window);
}

/*******************************************************************************
config_screen_set_display_pageno~

If the page number specified is non-zero, display "Page x" in the top-right hand
corner of the screen, otherwise display "Main Menu".
*******************************************************************************/

static void config_screen_set_display_pageno(int pageno) {
	wset_predefined_color(title_window,CONFIG_HI_COLOR);
	if (pageno) mvwprintw(title_window,0,get_window_width(title_window)-9,"  Page %-2d",pageno);
	else mvwprintw(title_window,0,get_window_width(title_window)-9,"Main Menu",pageno);
	wrefresh(title_window);
}

/*******************************************************************************
config_screen_check_if_pages_changed~
*******************************************************************************/

int config_screen_check_if_pages_changed(struct menu_page *pages) {
	struct menu_page *page = pages;
	int changed = FALSE;
	
	while (page && !changed) {
		if (page->changed) changed = TRUE;
		page = page->next;
	}
	return changed;
}

/*******************************************************************************
config_screen_new_page~

Allocate memory and initialize a new page structure.
*******************************************************************************/

static struct menu_page *config_screen_new_page(int height) {
	struct menu_page *menu_page = malloc(sizeof(struct menu_page));
	menu_page->menu_lines = malloc(sizeof(struct menu_line)*height);
	menu_page->num_lines = 0;
	menu_page->changed = FALSE;
	menu_page->next = 0;
	menu_page->prev = 0;
	return menu_page;
}

/*******************************************************************************
config_screen_free_page~

Frees memory allocated for a page structure.
*******************************************************************************/

static void config_screen_free_page(struct menu_page *page) {
	free(page->menu_lines);
	free(page);
}

/*******************************************************************************
config_screen_get_next_shortcut~
*******************************************************************************/

static char config_screen_get_next_shortcut(char shortcut) {

		if (!shortcut) return 0;
		else if (shortcut == '9') return 'A';
		else if (shortcut == 'L') return 'O';  /* M, N and P reserved for next page/main menu/previous page buttons */
		else if (shortcut == 'O') return 'Q';
		else if (shortcut == 'Z') return 0;
		else return shortcut+1;

}

/*******************************************************************************
config_screen_make_pages~

Make a series of pages from the supplied menu. This will work out what sections
and items to put on which pages, and initialise values of the items.
*******************************************************************************/

static struct menu_page *config_screen_make_pages(struct config_menu menu, ini_struct *ini) {
	struct menu_page *head = NULL, *page = NULL;
	int section_no, item_no;
	int page_height = get_window_height(menu_window)-2; /* allow room for next/prev/main buttons */
	int lineno = 0;
	int height, width;
	int index, check_index;
	char *value;
	char shortcut = '1';
	
	getmaxyx(menu_window,height,width);
	
	head = page = config_screen_new_page(page_height);

	/* add all sections */
	for (section_no = 0; section_no < menu.num_sections; section_no++) {
		if (lineno+2+menu.sections[section_no].num_items > page_height) {
			page->num_lines = lineno;
			/* new page */
			shortcut = '1';
			page->next = config_screen_new_page(page_height);
			page->next->prev = page;
			page = page->next;
			lineno = 0;
		}

		if (lineno > 0) {
			page->menu_lines[lineno].line_type = EMPTY_LINE;
			lineno++;
		}
		page->menu_lines[lineno].line_type = SECTION_LINE;
		page->menu_lines[lineno].contents.section = &menu.sections[section_no];
		page->menu_lines[lineno].contents.item = NULL;
		lineno++;

		/* add all items in this section */
		for (item_no = 0; item_no < menu.sections[section_no].num_items; item_no++) {
			if (lineno >= page_height) {
				page->num_lines = lineno;
				/* new page */
				shortcut = '1';
				page->next = config_screen_new_page(page_height);
				page->next->prev = page;
				page = page->next;
				lineno = 0;
			}
			else {
				page->menu_lines[lineno].line_type = ITEM_LINE;
				page->menu_lines[lineno].shortcut = shortcut;
				shortcut = config_screen_get_next_shortcut(shortcut);
				page->menu_lines[lineno].contents.section = &menu.sections[section_no];
				page->menu_lines[lineno].contents.item = &menu.sections[section_no].items[item_no];
				
				/* set the value of the menu item based on it's value in ini */
				value = ini_key_get_string(ini,page->menu_lines[lineno].contents.section->ini_section,
															 page->menu_lines[lineno].contents.item->ini_key,
															 "");
				
				
				if (menu.sections[section_no].items[item_no].num_values == 0)
					page->menu_lines[lineno].value.string = value;						
				else {
					/* find index of value */
					index = 0;
					for (check_index = 0;
							 check_index < page->menu_lines[lineno].contents.item->num_values;
							 check_index++) {
						if (!strcmp(value,page->menu_lines[lineno].contents.item->ini_values[check_index])) {
							index = check_index;
						}
					}
					page->menu_lines[lineno].value.index = index;
				}

				lineno++;
			}
		}
	}
	page->num_lines = lineno;
	
	return head;		
}

/*******************************************************************************
config_screen_free_pages~
*******************************************************************************/

void config_screen_free_pages(struct menu_page *page) {
	struct menu_page *next;
	
	while (page) {
		next = page->next;
		config_screen_free_page(page);
		page = next;
	}

}

/*******************************************************************************
config_screen_draw_menu_item~

Display a menu item and it's value on the screen.
*******************************************************************************/

static void config_screen_draw_menu_item(struct menu_page *page, int lineno, int selected) {

	char *display_value = NULL;
	int width = get_window_width(menu_window);

	if ((lineno < 0) || (lineno >= page->num_lines) || (page->menu_lines[lineno].line_type != ITEM_LINE)) return;

	if (selected) {
		wset_predefined_background_color(menu_window,CONFIG_HILIGHT_COLOR);
		wset_predefined_color(menu_window,CONFIG_HILIGHT_COLOR);
	}
	else {
		wset_predefined_background_color(menu_window,CONFIG_LO_COLOR);
		wset_predefined_color(menu_window,CONFIG_LO_COLOR);
	}
	wmove(menu_window,lineno,4);
	wclrtoeol(menu_window);

	mvwprintw(menu_window,lineno,7,"%s",page->menu_lines[lineno].contents.item->text);
	if (page->menu_lines[lineno].contents.item->num_values == 0) {
	  if (strlen(page->menu_lines[lineno].value.string) == 0)
			display_value = truncate_string(page->menu_lines[lineno].contents.item->values[1],width-58);
		else
			display_value = truncate_string(page->menu_lines[lineno].value.string,width-58);
	}
	else display_value = truncate_string(page->menu_lines[lineno].contents.item->values[page->menu_lines[lineno].value.index],width-58);

	mvwprintw(menu_window,lineno,57,"%s",display_value);
	
	if (!selected) wset_predefined_color(menu_window,CONFIG_HI_COLOR);
	if (page->menu_lines[lineno].shortcut) mvwaddch(menu_window,lineno,5,page->menu_lines[lineno].shortcut);

	free(display_value);
	
}

/*******************************************************************************
config_screen_draw_page~

Draw a page of menu sections and items (and values) on the screen.
*******************************************************************************/

static void config_screen_draw_page(struct menu_page *page) {

	int lineno;

	wset_predefined_background_color(menu_window,CONFIG_LO_COLOR);
	werase(menu_window);
	
	for (lineno = 0; lineno < page->num_lines; lineno++) {
		switch (page->menu_lines[lineno].line_type) {
			case SECTION_LINE:
				wset_predefined_color(menu_window,CONFIG_HI_COLOR);
				mvwprintw(menu_window,lineno,0," %s",page->menu_lines[lineno].contents.section->text);
				break;
			case ITEM_LINE:
				config_screen_draw_menu_item(page,lineno,FALSE);
				break;
			default:
		}
	}
	wrefresh(menu_window);

}

/*******************************************************************************
config_screen_check_if_button~
*******************************************************************************/

static int config_screen_check_if_button(struct menu_page *page, int lineno, int button) {

	if ((button == NEXT_BUTTON) && page->next && (lineno == -1))
		return TRUE;
	else if ((button == MAIN_BUTTON) &&
					 ((!page->next && lineno == -1) || (page->next && (lineno == -2))))
		return TRUE;
	else if ((button == PREV_BUTTON) && page->prev &&
					 ((lineno == -3) || (!page->next && (lineno == -2))))
		return TRUE;
	else return FALSE;

}


/*******************************************************************************
config_screen_draw_bottom_buttons~

Draw the bottom buttons (next page, main menu and previous page) at the bottom
of the screen. The buttons drawn depend on whether or not there are previous
or next pages available.

The buttons are numbered accordingly, i.e. if there is a next page button that
is button -1 (the first button) and main menu is button -2, otherwise main menu
is button -1. The number of previous page (if it exists) is -3 if a next page
button exists, otherwise it is -2.
*******************************************************************************/

static void config_screen_draw_bottom_buttons(struct menu_page *page, int lineno) {

	int cur_butno = -1;
	int x = 1, y = get_window_height(menu_window)-1;

	wset_predefined_background_color(menu_window,CONFIG_LO_COLOR);
	wmove(menu_window,y-1,1);
	wclrtoeol(menu_window);

	wmove(menu_window,get_window_height(menu_window)-1,1);
	/* next button */
	if (page->next) {
		wset_predefined_color(menu_window,lineno == cur_butno ? CONFIG_HILIGHT_COLOR : CONFIG_LO_COLOR);
		if (lineno == cur_butno) config_screen_set_hint("Show the next screen of configuration items.");
		mvwaddstr(menu_window,y,x,"  Next page  ");
		if (lineno != cur_butno) { /* add hilight to N */
			wset_predefined_color(menu_window,CONFIG_HI_COLOR);
			mvwaddch(menu_window,y,x+2,'N');
		}
		x += 13;
		cur_butno--;
		wset_predefined_color(menu_window,CONFIG_LO_COLOR);
		mvwaddstr(menu_window,y,x++," ");
	}

	/* main menu button (always present) */
	wset_predefined_color(menu_window,lineno == cur_butno ? CONFIG_HILIGHT_COLOR : CONFIG_LO_COLOR);
	if (lineno == cur_butno) config_screen_set_hint("Return to the opening screen selections.");
	mvwaddstr(menu_window,y,x,"  Main menu  ");
	if (lineno != cur_butno) { /* add hilight to M */
		wset_predefined_color(menu_window,CONFIG_HI_COLOR);
		mvwaddch(menu_window,y,x+2,'M');
	}
	x += 13;
	cur_butno--;
	
	wset_predefined_color(menu_window,CONFIG_LO_COLOR);
	mvwaddstr(menu_window,y,x++," ");

	/* previous page button */
	if (page->prev) {
		wset_predefined_color(menu_window,lineno == cur_butno ? CONFIG_HILIGHT_COLOR : CONFIG_LO_COLOR);
		if (lineno == cur_butno) config_screen_set_hint("Show the previous screen of configuration items.");
		mvwaddstr(menu_window,y,x,"  Previous page  ");
		if (lineno != cur_butno) { /* add hilight to P */
			wset_predefined_color(menu_window,CONFIG_HI_COLOR);
			mvwaddch(menu_window,y,x+2,'P');
		}
	}

}

/*******************************************************************************
config_screen_count_bottom_buttons~

Returns the number of buttons (next page, main menu and previous page) that are
displayed at the bottom of menu_window.
*******************************************************************************/

int config_screen_count_bottom_buttons(struct menu_page *page) {
	int count = 1;
	if (page->next) count++;
	if (page->prev) count++;
	return count;
}

/*******************************************************************************
config_screen_get_string_value~

Brings up a window at the bottom of the screen which prompts the user to enter
a string value.
*******************************************************************************/

char *config_screen_get_string_value(char *prompt) {

	int width = get_window_width(prompt_window), x;
	char *display_prompt = truncate_string(prompt,width-8);
	char *input_string;

	wset_predefined_background_color(prompt_window,CONFIG_LO_COLOR);
	werase(prompt_window);
	wset_predefined_color(prompt_window,CONFIG_LO_COLOR);

	for (x = 1; x < width-1; x++) {
		mvwaddch(prompt_window,0,x,special_char[HLINE_CHAR]);
		mvwaddch(prompt_window,3,x,special_char[HLINE_CHAR]);
	}

	mvwaddch(prompt_window,1,1,special_char[VLINE_CHAR]);
	mvwaddch(prompt_window,1,width-2,special_char[VLINE_CHAR]);
	mvwaddch(prompt_window,2,1,special_char[VLINE_CHAR]);
	mvwaddch(prompt_window,2,width-2,special_char[VLINE_CHAR]);

	mvwaddch(prompt_window,0,1,special_char[ULCORNER_CHAR]);
	mvwaddch(prompt_window,3,1,special_char[LLCORNER_CHAR]);
	mvwaddch(prompt_window,0,width-2,special_char[URCORNER_CHAR]);
	mvwaddch(prompt_window,3,width-2,special_char[LRCORNER_CHAR]);

	wset_predefined_color(prompt_window,CONFIG_HI_COLOR);
	mvwaddstr(prompt_window,1,4,display_prompt);
	mvwaddstr(prompt_window,2,4,"->");

	wrefresh(prompt_window);
	
	input_string = get_line(prompt_window,2,6,width-10,-1,"",NULL);
	
	touchline(config_screen_window,get_window_height(config_screen_window)-4,4);
	touchline(hint_window,0,1);
	wrefresh(config_screen_window);
	wrefresh(hint_window);
	
	free(display_prompt);

	return input_string;

}

/*******************************************************************************
config_screen_change_value~

If the menu item uses a list of values, the value is cycled to the next available
one. Otherwise, the user is prompted to enter a string value.
*******************************************************************************/

void config_screen_change_value(struct menu_page *page, int lineno, ini_struct *ini) {

	char *new_string_value;

	if (page->menu_lines[lineno].line_type == ITEM_LINE) {
		if (page->menu_lines[lineno].contents.item->num_values == 0) {
			/* get user-specified string */
			if ((new_string_value = config_screen_get_string_value(page->menu_lines[lineno].contents.item->values[2]))) {
				page->menu_lines[lineno].value.string = new_string_value;
				ini_key_set_string(ini,page->menu_lines[lineno].contents.section->ini_section,
													 page->menu_lines[lineno].contents.item->ini_key,
													 page->menu_lines[lineno].value.string);
			}
		}
		else {
			/* cycle through list of values */
			page->menu_lines[lineno].value.index++;
			if (page->menu_lines[lineno].value.index >= page->menu_lines[lineno].contents.item->num_values)
				page->menu_lines[lineno].value.index = 0;
			ini_key_set_string(ini,page->menu_lines[lineno].contents.section->ini_section,
												 page->menu_lines[lineno].contents.item->ini_key,
												 page->menu_lines[lineno].contents.item->ini_values[page->menu_lines[lineno].value.index]);
		}
		page->changed = TRUE;
		config_screen_draw_menu_item(page,lineno,TRUE);
		wrefresh(menu_window);
	}

}

/*******************************************************************************
config_screen_do_menu~

Returns TRUE if any changes have been made, otherwise FALSE.
*******************************************************************************/

/* A note about lineno: If lineno is >= 0, then this indicates a menu item is
   selected (the value of lineno giving the line in menu_window that is
	 selected). If lineno is < 0, this means that one of the bottom buttons is
	 selected. -1 is the first button, -2 the second and -3 the third. These
	 buttons are next page, main menu and previous page depending on the existence
	 of next/previous pages. */

static int config_screen_do_menu(struct config_menu menu, ini_struct *ini) {

  int finished = FALSE;
	int height, width, x;
	key_type key;
	int lineno = -1;  /* either next or main menu button */
	int check_lineno;
	int pageno = 1;
	struct menu_page *pages = config_screen_make_pages(menu,ini);
	struct menu_page *page = pages;
	int changed;
	
	if (!pages) return FALSE;

/* draw line above hint window */	
	getmaxyx(config_screen_window,height,width);
	wset_predefined_color(config_screen_window,CONFIG_LO_COLOR);
	mvwaddch(config_screen_window,height-4,0,special_char[LTEE_CHAR]);
	mvwaddch(config_screen_window,height-4,width-1,special_char[RTEE_CHAR]);
	wmove(config_screen_window,height-4,1);
	for (x = 1; x < width-1; x++) waddch(config_screen_window,special_char[HLINE_CHAR]);
	wrefresh(config_screen_window);
	
	config_screen_set_display_pageno(1);


	
	
	while ((lineno < page->num_lines) && (page->menu_lines[lineno].line_type != ITEM_LINE)) {
		lineno++;
	}
	if (lineno >= page->num_lines) lineno = -1;
	config_screen_draw_page(page);	
	config_screen_draw_menu_item(page,lineno,TRUE);
	config_screen_draw_bottom_buttons(page,lineno);
	config_screen_set_hint(page->menu_lines[lineno].contents.item->hint);
	wrefresh(menu_window);
	
	while (!finished) {
		key = uppercase_key(read_key());
		switch (key) {
			case DOWN_KEY:
				if (lineno >= 0) config_screen_draw_menu_item(page,lineno,FALSE);
				else lineno = -1;
				lineno++;
				while ((lineno < page->num_lines) && (page->menu_lines[lineno].line_type != ITEM_LINE)) {
					lineno++;
				}
				if (lineno >= page->num_lines) lineno = -1;
				else {
					config_screen_draw_menu_item(page,lineno,TRUE);
					config_screen_set_hint(page->menu_lines[lineno].contents.item->hint);
				}
				config_screen_draw_bottom_buttons(page,lineno);
				wrefresh(menu_window);
				break;
			case UP_KEY:
				if (lineno >= 0) config_screen_draw_menu_item(page,lineno,FALSE);
				else lineno = page->num_lines;
				lineno--;
				while ((lineno >= 0) && (page->menu_lines[lineno].line_type != ITEM_LINE)) {
					lineno--;
				}
				if (lineno >= 0) {
					config_screen_draw_menu_item(page,lineno,TRUE);
					config_screen_set_hint(page->menu_lines[lineno].contents.item->hint);
				}
				config_screen_draw_bottom_buttons(page,lineno);
				wrefresh(menu_window);
				break;
			case RIGHT_KEY:
				config_screen_draw_menu_item(page,lineno,FALSE);
				if (lineno >= 0) {
					if (config_screen_count_bottom_buttons(page) >= 2) lineno = -2;
					else lineno = -1;
				}
				else if (lineno > -config_screen_count_bottom_buttons(page)) {
					lineno--;
				}
				else lineno = -1;
				config_screen_draw_bottom_buttons(page,lineno);
				wrefresh(menu_window);
				break;
			case LEFT_KEY:
				config_screen_draw_menu_item(page,lineno,FALSE);
				if (lineno >= 0) {
					lineno = -1;
				}
				else if (lineno < -1) {
					lineno++;
				}
				else lineno = -config_screen_count_bottom_buttons(page);
				config_screen_draw_bottom_buttons(page,lineno);
				wrefresh(menu_window);
				break;
			default:
				if ((key == ENTER_KEY) && (lineno >= 0)) {
					/* a menu item is selected */
					config_screen_change_value(page,lineno,ini);
					wrefresh(menu_window);
				}
				else if (page->next && ((key == 'N') ||
								  ((key == ENTER_KEY) && config_screen_check_if_button(page,lineno,NEXT_BUTTON)))) {
				  /* next page */
					page = page->next;
					lineno = -1;
					config_screen_draw_page(page);	
					config_screen_draw_menu_item(page,lineno,TRUE);
					config_screen_draw_bottom_buttons(page,lineno);
					/* update page number (only for display purposes) */
					pageno++;
					config_screen_set_display_pageno(pageno);
					wrefresh(menu_window);
				}
				else if (page->prev && ((key == 'P') ||
								 ((key == ENTER_KEY) && config_screen_check_if_button(page,lineno,PREV_BUTTON)))) {
					/* previous page */
					page = page->prev;
					lineno = -config_screen_count_bottom_buttons(page);
					config_screen_draw_page(page);
					if (lineno >= 0) config_screen_draw_menu_item(page,lineno,TRUE);
					config_screen_draw_bottom_buttons(page,lineno);
					/* update page number (only for display purposes) */
					pageno--;
					config_screen_set_display_pageno(pageno);
					wrefresh(menu_window);
				}
				else if ((key == 'M') ||
								 ((key == ENTER_KEY) && config_screen_check_if_button(page,lineno,MAIN_BUTTON))) {
					finished = TRUE;
					wrefresh(menu_window);
				}
				for (check_lineno = 0; check_lineno < page->num_lines; check_lineno++) {
					if ((page->menu_lines[check_lineno].line_type == ITEM_LINE) &&
					    (page->menu_lines[check_lineno].shortcut == key)) {
						config_screen_draw_menu_item(page,lineno,FALSE);
						lineno = check_lineno;
						config_screen_draw_bottom_buttons(page,lineno);
						config_screen_set_hint(page->menu_lines[lineno].contents.item->hint);
						config_screen_draw_menu_item(page,lineno,TRUE);
						wrefresh(menu_window);
						config_screen_change_value(page,lineno,ini);
					}
				}
		}
	}

	changed = config_screen_check_if_pages_changed(pages);	
	config_screen_free_pages(pages);
	
	return changed;
}

/*******************************************************************************
config_screen_draw~

Draws the borders and title for the configuration screen.
*******************************************************************************/

static void config_screen_draw(void) {

	int width, height, y, x;

	getmaxyx(config_screen_window,height,width);
	wset_predefined_background_color(config_screen_window,CONFIG_LO_COLOR);
	werase(config_screen_window);
	wrefresh(config_screen_window);
	wset_predefined_color(config_screen_window,CONFIG_LO_COLOR);


	for (x = 1; x < width-1; x++) {
		mvwaddch(config_screen_window,0,x,special_char[HLINE_CHAR]);
		mvwaddch(config_screen_window,2,x,special_char[HLINE_CHAR]);
		mvwaddch(config_screen_window,height-2,x,special_char[HLINE_CHAR]);
	}

	for (y = 1; y < height-2; y++) {
		mvwaddch(config_screen_window,y,0,special_char[VLINE_CHAR]);
		mvwaddch(config_screen_window,y,width-1,special_char[VLINE_CHAR]);
	}

	mvwaddch(config_screen_window,0,0,special_char[ULCORNER_CHAR]);
	mvwaddch(config_screen_window,2,0,special_char[LTEE_CHAR]);
	mvwaddch(config_screen_window,height-2,0,special_char[LLCORNER_CHAR]);
	mvwaddch(config_screen_window,0,width-1,special_char[URCORNER_CHAR]);
	mvwaddch(config_screen_window,2,width-1,special_char[RTEE_CHAR]);
	mvwaddch(config_screen_window,height-2,width-1,special_char[LRCORNER_CHAR]);

	mvwaddstr(config_screen_window,height-1,1,"Up, Down Select function   ENTER Execute");

	wrefresh(config_screen_window);

	wset_predefined_background_color(title_window,CONFIG_HI_COLOR);
	werase(title_window);
	wset_predefined_color(title_window,CONFIG_HI_COLOR);
	mvwaddstr(title_window,0,0,"XTC - Configuration");
	wrefresh(title_window);
	
	wset_predefined_background_color(title_window,CONFIG_HI_COLOR);
	wset_predefined_color(title_window,CONFIG_HI_COLOR);
	werase(title_window);
	mvwaddstr(title_window,0,0,"XTC - Configuration");

}

/*******************************************************************************
config_screen_draw_main_menu_item~
*******************************************************************************/

void config_screen_draw_main_menu_item(int itemno, int selected) {
	static struct {
		char text[80];
		char shortcut;
		int lineno;
	} items[6] = {{"Modify configuration items",'1',3},
								{"Display color selection",'2',5},
								{"Read permanent settings from disk",'3',8},
								{"Restore factory default settings",'4',10},
								{"Save configuration and quit",'S',13},
								{"Quit configuration program",'Q',15}};
								



	if (selected) {
		wset_predefined_background_color(menu_window,CONFIG_HILIGHT_COLOR);
		wset_predefined_color(menu_window,CONFIG_HILIGHT_COLOR);
	}
	else {
		wset_predefined_background_color(menu_window,CONFIG_LO_COLOR);
		wset_predefined_color(menu_window,CONFIG_LO_COLOR);
	}
	mvwaddstr(menu_window,items[itemno].lineno,21,"                                      ");
	mvwaddstr(menu_window,items[itemno].lineno,24,items[itemno].text);
	if (!selected) wset_predefined_color(menu_window,CONFIG_HI_COLOR);
	mvwaddch(menu_window,items[itemno].lineno,22,items[itemno].shortcut);

}

/*******************************************************************************
config_screen_draw_main_menu~
*******************************************************************************/

void config_screen_draw_main_menu(int selected_itemno) {

	int itemno;

	config_screen_set_display_pageno(0);
	wset_predefined_background_color(menu_window,CONFIG_LO_COLOR);
	werase(menu_window);	
	for (itemno = 0; itemno < 6; itemno++)
		config_screen_draw_main_menu_item(itemno,FALSE);
	config_screen_draw_main_menu_item(selected_itemno,TRUE);
	wrefresh(menu_window);
	wrefresh(title_window);

}

/*******************************************************************************
config_screen_dialog_draw_buttons~
*******************************************************************************/

void config_screen_dialog_draw_buttons(WINDOW *window, int yes, int yesno) {
	int width = get_window_width(window);

	wmove(window,3,0);
	
	if (yesno) {
		/* draw yes and no buttons */
	
		wmove(window,3,width/2-8);
		if (yes) wset_predefined_color(window,CONFIG_HILIGHT_COLOR);
		else wset_predefined_color(window,CONFIG_HI_COLOR);
		waddstr(window,"  Y");
		if (!yes) wset_predefined_color(window,CONFIG_LO_COLOR);
		waddstr(window,"es  ");

		wset_predefined_color(window,CONFIG_LO_COLOR);
		waddstr(window,"  ");

		if (!yes) wset_predefined_color(window,CONFIG_HILIGHT_COLOR);
		else wset_predefined_color(window,CONFIG_HI_COLOR);
		waddstr(window,"  N");
		if (yes) wset_predefined_color(window,CONFIG_LO_COLOR);
		waddstr(window,"o  ");
	}
	else {
		/* draw ok button - always hilighted */
		wmove(window,3,width/2-3);
		wset_predefined_color(window,CONFIG_HILIGHT_COLOR);
		waddstr(window,"  Ok  ");
	}

	wrefresh(window);
	
}

/*******************************************************************************
config_screen_dialog~
*******************************************************************************/

int config_screen_dialog(char *message, int yesno) {
	
	WINDOW *dialog_window;
	int length;
	int height, width;
	int yes = FALSE;
	int finished = FALSE;
	char *display_message;
	key_type key;
	
	getmaxyx(stdscr,height,width);

	display_message = truncate_string(message,width-8);
	if (strlen(display_message) < 15) length = 15;
	else length = strlen(display_message);
	
	dialog_window = newwin(5,length+6,height-7,width/2-(length+6)/2);
	wset_predefined_background_color(dialog_window,CONFIG_LO_COLOR);
	wset_predefined_color(dialog_window,CONFIG_LO_COLOR);
	werase(dialog_window);
	wborder(dialog_window,special_char[VLINE_CHAR],special_char[VLINE_CHAR],
					special_char[HLINE_CHAR],special_char[HLINE_CHAR],special_char[ULCORNER_CHAR],
					special_char[URCORNER_CHAR],special_char[LLCORNER_CHAR],special_char[LRCORNER_CHAR]);
	wset_predefined_color(dialog_window,CONFIG_HI_COLOR);
	mvwaddstr(dialog_window,1,3,display_message);	
	wrefresh(dialog_window);
	config_screen_dialog_draw_buttons(dialog_window,yes,yesno);
	while (!finished) {
		key = uppercase_key(read_key());
		if ((key == LEFT_KEY) || (key == RIGHT_KEY)) {
			yes = !yes;
			config_screen_dialog_draw_buttons(dialog_window,yes,yesno);
		}
		else if ((key == 'Y') || ((key == ENTER_KEY) && yes)) {
			yes = TRUE;
			finished = TRUE;
		}
		else if ((key == 'N') || (key == F_KEY(10)) || ((key == ENTER_KEY) && !yes)) {
			yes = FALSE;
			finished = TRUE;
		}
	}
	delwin(dialog_window);
	touchline(config_screen_window,get_window_height(config_screen_window)-4,2);
	wrefresh(config_screen_window);
	touchline(menu_window,get_window_height(menu_window)-3,3);
	wrefresh(menu_window);
	free(display_message);
	return yes;

}


/*******************************************************************************
config_screen_do_main_menu~
*******************************************************************************/

void config_screen_do_main_menu(ini_struct *ini) {

	int finished = FALSE;
	key_type key;
	int itemno = 0;
	char *config_filename;
	char *error_message;
	int changed = FALSE;

	config_screen_draw_main_menu(itemno);

	while (!finished) {
		key = uppercase_key(read_key());
		if (key == UP_KEY) {
			config_screen_draw_main_menu_item(itemno,FALSE);
			itemno--;
			if (itemno < 0) itemno = 5;
			config_screen_draw_main_menu_item(itemno,TRUE);
			wrefresh(menu_window);
		}
		else if (key == DOWN_KEY) {
			config_screen_draw_main_menu_item(itemno,FALSE);
			itemno++;
			if (itemno >= 6) itemno = 0;
			config_screen_draw_main_menu_item(itemno,TRUE);
			wrefresh(menu_window);
		}
		else if ((key == '1') || ((key == ENTER_KEY) && (itemno == 0))) {
			/* Modify configuration items */
			config_screen_draw_main_menu_item(itemno,FALSE);
			itemno = 0;
			config_screen_draw_main_menu_item(itemno,TRUE);
			wrefresh(menu_window);

			changed = config_screen_do_menu(config_menu,ini);
			config_screen_draw();
			config_screen_draw_main_menu(itemno);
		}
		else if ((key == 'S') || ((key == ENTER_KEY) && (itemno == 4))) {
			/* Save configuration and quit */
			config_screen_draw_main_menu_item(itemno,FALSE);
			itemno = 4;
			config_screen_draw_main_menu_item(itemno,TRUE);
			wrefresh(menu_window);

			if (config_screen_dialog("Save configuration?",TRUE)) {
				if ((config_filename = get_config_filename())) {
					if (ini_save(ini,config_filename)) {
						error_message = malloc(strlen("Error writing config file ~/.xtcrc: ")+strlen(strerror(errno))+1);
						sprintf(error_message,"Error writing config file ~/.xtcrc: %s",strerror(errno));
						config_screen_dialog(error_message,FALSE);
						free(error_message);
					}
					free(config_filename);
					finished = TRUE;
					config_load();
				}
				else
					config_screen_dialog("Error writing config file ~/.xtcrc: $HOME is not set.",FALSE);
			}
		}
		else if ((key == 'Q') || ((key == ENTER_KEY) && (itemno == 5))) {
			/* Quit configuration program */
			if (changed) {
				config_screen_draw_main_menu_item(itemno,FALSE);
				itemno = 5;
				config_screen_draw_main_menu_item(itemno,TRUE);
				wrefresh(menu_window);
			}

			if (!changed || config_screen_dialog("Quit and throw away changes?",TRUE))
				finished = TRUE;
		}
	}

}

/*******************************************************************************
config_screen_resize~

Called when the screen/xterm window is resized. Redraws the configuration screen
to conform with the new size.
*******************************************************************************/

void config_screen_resize(void) {
	int height, width;

	getmaxyx(stdscr,height,width);
	resize_window(&menu_window,height-7,width-2);
	resize_window(&hint_window,1,width-4);
	resize_window(&title_window,1,width-4);
	resize_window(&prompt_window,4,width);

	config_screen_draw();

}

/*******************************************************************************
config_screen_main~
*******************************************************************************/

void config_screen_main(void) {
	int height, width;
	ini_struct *ini = config_read();
	int prev_mode = current_mode;
	
	getmaxyx(stdscr,height,width);

	config_screen_window = newwin_force(height,width,0,0);
	menu_window = newwin_force(height-7,width-2,3,1);
	hint_window = newwin_force(1,width-4,height-3,2);
	title_window = newwin_force(1,width-4,1,2);
	prompt_window = newwin_force(4,width,height-4,0);
	current_mode = CONFIG_MODE;

	config_screen_draw();
	config_screen_do_main_menu(ini);

	current_mode = prev_mode;
	delwin(config_screen_window);
	delwin(hint_window);
	delwin(menu_window);
	delwin(title_window);
	delwin(prompt_window);	
	redraw_screen();
	
}

/*******************************************************************************
config_read~

Reads the contents of the configuration file (if it exists), and sets default
values for those that don't exist or are invalid in the configuration file.

If the configuration file doesn't exist or can't be read, all values are set
to default.
*******************************************************************************/

ini_struct *config_read(void) {

	ini_struct *ini = NULL;
	int section_no, item_no;
	int index, check_index;
	char *value;
	char *config_filename;

	if ((config_filename = get_config_filename())) {
		ini = ini_load(config_filename);
		free(config_filename);
	}
	if (!ini) ini = ini_new();
	
	for (section_no = 0; section_no < config_menu.num_sections; section_no++) {
		for (item_no = 0; item_no < config_menu.sections[section_no].num_items; item_no++) {
			value = ini_key_get_string(ini,config_menu.sections[section_no].ini_section,
																	config_menu.sections[section_no].items[item_no].ini_key,
																	NULL);

			if (value == NULL) {
				if (config_menu.sections[section_no].items[item_no].num_values == 0)
					/* use values[0] instead of ini_values[0] */
					ini_key_set_string(ini,config_menu.sections[section_no].ini_section,
														 config_menu.sections[section_no].items[item_no].ini_key,
														 config_menu.sections[section_no].items[item_no].values[0]);
				else
					ini_key_set_string(ini,config_menu.sections[section_no].ini_section,
														 config_menu.sections[section_no].items[item_no].ini_key,
														 config_menu.sections[section_no].items[item_no].ini_values[0]);
			}
			else if (config_menu.sections[section_no].items[item_no].num_values > 0) {
				index = 0;
				for (check_index = 0;
						 check_index < config_menu.sections[section_no].items[item_no].num_values;
						 check_index++) {
					if (!strcmp(value,config_menu.sections[section_no].items[item_no].ini_values[check_index])) {
						index = check_index;
					}
				}
				ini_key_set_string(ini,config_menu.sections[section_no].ini_section,
													 config_menu.sections[section_no].items[item_no].ini_key,
													 config_menu.sections[section_no].items[item_no].ini_values[index]);
			}
			if (value) free(value);
		}
	}
	return ini;

}

/*******************************************************************************
get_config_filename~

Returns the name of the configuration file (user's home directory + "/.xtcrc")
*******************************************************************************/

char *get_config_filename(void) {

	char *home = getenv("HOME");
	char *config_filename;
	
	if (!home) return NULL;
	else {
		config_filename = malloc(strlen(home)+1+strlen(".xtcrc")+1);
		sprintf(config_filename,"%s/%s",home,".xtcrc");
		return config_filename;
	}

}

/*******************************************************************************
config_load~

Reads configuration information into the global variable config, which the rest
of the program uses to find out the value of various configuration options.

Note: The default paramater passed to ini_key_get_ini and ini_get_get_string
is not relevant as these values have all been set in config_read().
*******************************************************************************/

void config_load(void) {

	ini_struct *ini = config_read();
	char *tempstr;
	
	/* [file_window] display_format */
	tempstr = ini_key_get_string(ini,"file_window","display_format",0);
	if (!strcasecmp(tempstr,"DATE")) config.file_window.display_format = CONFIG_FILE_WINDOW_DISPLAY_FORMAT_DATE;
	else if (!strcasecmp(tempstr,"ATTRIBUTES")) config.file_window.display_format = CONFIG_FILE_WINDOW_DISPLAY_FORMAT_ATTRIBUTES;
	else config.file_window.display_format = CONFIG_FILE_WINDOW_DISPLAY_FORMAT_NORMAL;
	free(tempstr);

	/* [file_window] sort_criteria */
	tempstr = ini_key_get_string(ini,"file_window","sort_criteria",0);
	if (!strcasecmp(tempstr,"EXT")) config.file_window.sort_criteria = CONFIG_FILE_WINDOW_SORT_CRITERIA_EXT;
	else if (!strcasecmp(tempstr,"DATE")) config.file_window.sort_criteria = CONFIG_FILE_WINDOW_SORT_CRITERIA_DATE;
	else if (!strcasecmp(tempstr,"SIZE")) config.file_window.sort_criteria = CONFIG_FILE_WINDOW_SORT_CRITERIA_SIZE;
	else config.file_window.sort_criteria = CONFIG_FILE_WINDOW_SORT_CRITERIA_NAME;
	free(tempstr);

	/* [file_window] sort_desc */
	config.file_window.sort_desc = ini_key_get_int(ini,"file_window","sort_desc",0);

	/* [file_window] sort_path */
	config.file_window.sort_path = ini_key_get_int(ini,"file_window","sort_path",0);

	/* [file_window] sort_case */
	config.file_window.sort_case = ini_key_get_int(ini,"file_window","sort_case",0);

	/* [file_window] small_file_window */
	config.file_window.small_file_window = ini_key_get_int(ini,"file_window","small_file_window",0);

	/* [file_window] cols */
	config.file_window.cols = ini_key_get_int(ini,"file_window","cols",0);

	/* [dir_window] initial_dir */
	tempstr = ini_key_get_string(ini,"dir_window","initial_dir",0);
	if (!strcasecmp(tempstr,"HOME")) config.dir_window.initial_dir = CONFIG_DIR_WINDOW_INITAL_DIR_HOME;
	else if (!strcasecmp(tempstr,"ROOT")) config.dir_window.initial_dir = CONFIG_DIR_WINDOW_INITAL_DIR_ROOT;
	else config.dir_window.initial_dir = CONFIG_DIR_WINDOW_INITAL_DIR_CURRENT;
	free(tempstr);

	/* [misc] skip_quit_prompt */
	config.misc.skip_quit_prompt = ini_key_get_int(ini,"misc","skip_quit_prompt",0);

	/* [misc] beep */
	config.misc.beep = ini_key_get_int(ini,"misc","beep",0);

	/* [misc] date_format */
	config.misc.date_format = ini_key_get_string(ini,"misc","date_format","");

	/* [misc] time_format */
	config.misc.time_format = ini_key_get_string(ini,"misc","time_format","");

	/* [misc] use_ext_chars */
	config.misc.use_ext_chars = ini_key_get_int(ini,"misc","use_ext_chars",0);

	/* [external_programs] editor */
	config.external_programs.editor = ini_key_get_string(ini,"external_programs","editor","");

	/* [external_programs] viewer */
	config.external_programs.viewer = ini_key_get_string(ini,"external_programs","viewer","");
	
}

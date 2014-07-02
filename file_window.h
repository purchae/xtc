enum {
	FILE_WINDOW_NORMAL_SIZE,
	FILE_WINDOW_MINI_SIZE
};  /* used for file_window_size */

enum {
	FILE_WINDOW_NORMAL_LIST,
	FILE_WINDOW_BRANCH_LIST
};  /* used for file_window_list_type */

enum {
	NO_PATHS,
	RELATIVE_PATHS,
	ABSOLUTE_PATHS
};  /* used for copy_tagged, move_tagged and symlink_tagged */

extern WINDOW* minifl_separator_window; /* in file_window.c */
extern WINDOW* file_window; /* in file_window.c */
extern int file_window_size; /* in file_window.c */
extern int file_window_list_type; /* in file_window.c */

void file_window_add_file_and_update_list(char *filename);
void file_window_remove_file_and_update_list(char *filename);
int file_window_update_file_and_update_list(char *filename);

void file_window_change_display_format(void);
int file_window_count_tagged_files(int show_error);
void file_window_set_filespec(void);
void file_window_change_sort_order(void);
file_info_struct *file_window_get_selected_file(void);
void file_window_hide_separator(void);
void file_window_show_separator(void);
void file_window_init(void);
void file_window_draw(void);
void file_window_select(int entrynum);
int file_window_set_items_from_branch(dir_info_struct* dir);
void file_window_set_items_from_dir(dir_info_struct* dir);
void file_window_resize_normal(void);
void file_window_resize_mini(void);

int file_window_process_key(key_type key);
void file_window_activate(void);
void file_window_set_cols(void);

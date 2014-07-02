struct _dir_line_struct {
	dir_info_struct* dir;
};
typedef struct _dir_line_struct dir_line_struct;

enum {
	DIR_WINDOW_NORMAL_MODE,
	DIR_WINDOW_SELECT_MODE
};

dir_info_struct *dir_window_logadd_path(char *path);
dir_info_struct *dir_window_get_selected_dir(void);
void dir_window_draw(void);
void dir_window_init(void);

int dir_window_process_key(key_type key);
void dir_window_activate(void);

dir_info_struct *dir_window_do_select(dir_info_struct *initial_dir);
int dir_window_expand_to_path(char *path, int update);
int dir_window_set_path(char *path);
void dir_window_set_initial_path(void);

WINDOW* dir_window;
dir_info_struct* dir_window_rootdir;
int dir_window_mode;

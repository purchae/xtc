void info_window_update(void);

extern WINDOW *path_window;  /* in info_window.c */
extern WINDOW* info_window;  /* in info_window.c */
extern int info_window_drawn;  /* in info_window.c */
extern int info_window_input_path_pos;  /* in info_window.c */
extern WINDOW* command_window;  /* in info_window.c */

void info_window_draw(void);
void info_window_init(void);
void info_window_reinit(void);
void info_window_update_filespec(void);
void info_window_update_path(char *path);
void info_window_update_input_path(char *path);
void info_window_update_disk_stats(void);
void info_window_update_stats(int type);
void info_window_update_local_stats(void);
void info_window_update(void);

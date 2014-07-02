void hide_cursor(void);
void show_cursor(void);
void redraw_screen(void);
void resize_screen(int sig);
int confirm_quit(void);
void main_window_draw(void);
WINDOW *newwin_force(int height, int width, int y, int x);
void resize_window(WINDOW **win, int height, int width);
void command_window_show_from_to(char *op, char *from, char *to);
int ask_replace(int file_pos, char *src_filename, char *dest_filename);
void window_border(WINDOW *window);
char* get_line(WINDOW* window, int y, int x, int input_length, int max_length,
 							char *default_string, char* (*ext_func)(char*,short));


void reloc_file_op(
	char *prompt,                              /* e.g. "MOVE file:" */
	char *initial_name,                        /* file/dirname WITHOUT the path */
	char *initial_filename,                    /* file/dirname WITH the path */
	char *base_path,                           /* "current" path to use for initial selection and resolving relative dirs */
	char* (*op_func)(char*,char*,char*,char*)  /* function to do the actual operation */
);

void reloc_tagged_files_op(
	char *prompt,                              /* e.g. "MOVE file:" */
	char *base_path,                           /* "current" path to use for initial selection and resolving relative dirs */
	void (*op_func)(char*,int,int),            /* function to do the actual operation */
	int with_paths                             /* whether or not to prompt the user about paths */
);


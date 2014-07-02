void config_screen_resize(void);
void config_screen_main(void);
ini_struct *config_read(void);
char *get_config_filename(void);
void config_load(void);

struct {
	struct {
		int display_format;
		int sort_criteria;
		int sort_desc;
		int sort_path;
		int sort_case;
		int small_file_window;
		int cols;
	} file_window;
	struct {
		int initial_dir;
	} dir_window;
	struct {
		int skip_quit_prompt;
		int beep;
		char *date_format;
		char *time_format;
		int use_ext_chars;
	} misc;
	struct {
		char *editor;
		char *viewer;
	} external_programs;
} config;

#define CONFIG_DIR_WINDOW_INITAL_DIR_CURRENT 0
#define CONFIG_DIR_WINDOW_INITAL_DIR_HOME    1
#define CONFIG_DIR_WINDOW_INITAL_DIR_ROOT    2

#define CONFIG_FILE_WINDOW_SORT_CRITERIA_NAME 0
#define CONFIG_FILE_WINDOW_SORT_CRITERIA_EXT  1
#define CONFIG_FILE_WINDOW_SORT_CRITERIA_DATE 2
#define CONFIG_FILE_WINDOW_SORT_CRITERIA_SIZE 3

#define CONFIG_FILE_WINDOW_DISPLAY_FORMAT_NORMAL     0
#define CONFIG_FILE_WINDOW_DISPLAY_FORMAT_DATE       1
#define CONFIG_FILE_WINDOW_DISPLAY_FORMAT_ATTRIBUTES 2

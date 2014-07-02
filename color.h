/* colors */

#define FILES_COLOR 0
#define DIRS_COLOR 1
#define HILIGHT_COLOR 2
#define LINES_COLOR 3
#define TEXT_COLOR 4
#define PANEL_COLOR 5
#define INPUT_COLOR 6
#define SLINK_COLOR 7
#define CONFIG_LO_COLOR 8
#define CONFIG_HI_COLOR 9
#define CONFIG_HILIGHT_COLOR 10


enum {
	HLINE_CHAR,
	VLINE_CHAR,
	ULCORNER_CHAR,
	URCORNER_CHAR,
	LLCORNER_CHAR,
	LRCORNER_CHAR,
	TTEE_CHAR,
	BTEE_CHAR,
	LTEE_CHAR,
	RTEE_CHAR,
	DIAMOND_CHAR,
	BULLET_CHAR,
	LARROW_CHAR,
	RARROW_CHAR,
	LAST_CHAR
};  /* used for special_chars */

struct _predefined_color_struct {
	int fg, bg, bw_attrs;
};
typedef struct _predefined_color_struct predefined_color_struct;

extern const predefined_color_struct predefined_colors[];  /* in color.c */
extern long special_char[LAST_CHAR];  /* in color.c */
extern int in_color;  /* in color.c */

void wset_color(WINDOW *window, short fg, short bg);
void set_color(short fg, short bg);
void wset_background_color(WINDOW *window, short bg);
void wset_predefined_color(WINDOW *window, int colornum);
void wset_predefined_background_color(WINDOW *window, int colornum);
void remove_color(void);
void set_special_chars(int use_acs);

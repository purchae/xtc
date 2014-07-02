#define RESPONSE_YES       1
#define RESPONSE_NO        2
#define RESPONSE_OK        4
#define RESPONSE_CANCEL    8
#define RESPONSE_HELP      16
#define RESPONSE_RETRYFILE 32
#define RESPONSE_SKIPFILE  64
#define RESPONSE_ABSOLUTE  128
#define RESPONSE_RELATIVE  256

void status_line_init(void);
void status_line_reinit(void);
void status_line_clear(void);
void status_line_display_command_keys(char *keystring);
void status_line_set_message(char *message);
int status_line_get_response(char *prompt, int options, int cursor_shown);
void status_line_error_message(char *error_message);
void status_line_progress_bar_set(int percent);
int status_line_confirm_string(char *string);

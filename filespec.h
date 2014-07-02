typedef struct _filespec_list_struct filespec_list_struct;
extern filespec_list_struct *filespec_list;  /* in filespec.c */

filespec_list_struct *filespec_list_create(void);
void filespec_list_clear(filespec_list_struct *list);
void filespec_list_set(filespec_list_struct *list, char *filespec_string);
int filespec_list_check_match(filespec_list_struct *list, char *filename);
char *filespec_list_to_string(filespec_list_struct *list, char *separator);

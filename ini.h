typedef struct _ini_struct ini_struct;

void ini_key_set_string(ini_struct *ini, const char *section, const char *key, const char *value);
void ini_key_set_int(ini_struct *ini, const char *section, const char *key, const int value);
char *ini_key_get_string(ini_struct *ini, const char *section, const char *key, const char *default_value);
int ini_key_get_int(ini_struct *ini, const char *section, const char *key, const int default_value);
void ini_key_delete(ini_struct *ini, const char *section, const char *key);
void ini_section_delete(ini_struct *ini, const char *section);
ini_struct *ini_load(const char *filename);
ini_struct *ini_new(void);
int ini_save(ini_struct *ini, const char *filename);

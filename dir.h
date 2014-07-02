typedef struct _file_info_struct file_info_struct;
typedef struct _dir_info_struct dir_info_struct;

struct _file_info_struct {
	char *name;
	dir_info_struct *dir;
	int tagged;
	file_info_struct *next;
	/* items from stat */
	off_t size;
	time_t mtime;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	char *username;
	char *groupname;
};

struct _dir_info_struct {
	char *name;
	int logged;
	dir_info_struct *parent;
	dir_info_struct *subdirs;
	dir_info_struct *next;
	file_info_struct *files;
	/* items from stat */
	time_t mtime;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	char *username;
	char *groupname;
};

enum {
	ALL_FILES,
	TAGGED_FILES,
	MATCHING_FILES
};



/* file functions */

char *file_get_display_line(file_info_struct *file, int length, int format);
file_info_struct *file_find_from_full_name(char *filename, dir_info_struct *rootdir);
char *file_get_full_name(file_info_struct *file);
file_info_struct *file_read_and_add(char *filename);
void file_free(file_info_struct *file);
int file_update(file_info_struct *file);
file_info_struct *file_read(char *filename);

/* dir functions */

dir_info_struct *dir_find_from_path(char *path, dir_info_struct *rootdir);
char *dir_get_path(dir_info_struct *dir);
int dir_check_if_parent(dir_info_struct *parent, dir_info_struct *child);
int dir_update(dir_info_struct *dir);
void dir_remove_file(dir_info_struct *dir, file_info_struct *file);
void dir_insert_subdir(dir_info_struct *dir, dir_info_struct *new_subdir);
void dir_free(dir_info_struct *dir);
void dir_remove_subdir(dir_info_struct *dir, dir_info_struct *subdir);
int dir_add_file(dir_info_struct *dir, file_info_struct *file);
dir_info_struct *dir_read(char *path);
int dir_log(dir_info_struct *dir);
void dir_unlog(dir_info_struct* dir);
off_t dir_calculate_size(dir_info_struct* dir, int (*check_func)(file_info_struct*));
off_t dir_calculate_total_size(dir_info_struct* dir, int (*check_func)(file_info_struct*));
int dir_count_files(dir_info_struct* dir, int (*check_func)(file_info_struct*));
int dir_count_total_files(dir_info_struct* dir, int (*check_func)(file_info_struct*));

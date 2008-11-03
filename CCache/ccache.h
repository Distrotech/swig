#define CCACHE_VERSION "2.4"

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <utime.h>
#include <stdarg.h>
#include <dirent.h>
#include <limits.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

#define STATUS_NOTFOUND 3
#define STATUS_FATAL 4
#define STATUS_NOCACHE 5

#define MYNAME "ccache"

#define LIMIT_MULTIPLE 0.8

/* default maximum cache size */
#ifndef DEFAULT_MAXSIZE
#define DEFAULT_MAXSIZE (1000*1000)
#endif

/* file copy mode */
#ifdef ENABLE_ZLIB
#define COPY_UNCOMPRESSED 0
#define COPY_FROM_CACHE 1
#define COPY_TO_CACHE 2
#endif

enum stats {
	STATS_NONE=0,
	STATS_STDOUT,
	STATS_STATUS,
	STATS_ERROR,
	STATS_TOCACHE,
	STATS_PREPROCESSOR,
	STATS_COMPILER,
	STATS_MISSING,
	STATS_CACHED,
	STATS_ARGS,
	STATS_LINK,
	STATS_NUMFILES,
	STATS_TOTALSIZE,
	STATS_MAXFILES,
	STATS_MAXSIZE,
	STATS_NOTC,
	STATS_DEVICE,
	STATS_NOINPUT,
	STATS_MULTIPLE,
	STATS_CONFTEST,
	STATS_UNSUPPORTED,
	STATS_OUTSTDOUT,

	STATS_END
};

typedef unsigned uint32;

#include "mdfour.h"

void hash_start(void);
void hash_string(const char *s);
void hash_int(int x);
void hash_file(const char *fname);
char *hash_result(void);
void hash_buffer(const char *s, int len);

void cc_log(const char *format, ...);
void fatal(const char *msg);

void copy_fd(int fd_in, int fd_out);
int copy_file(const char *src, const char *dest);
int move_file(const char *src, const char *dest);
int test_if_compressed(const char *filename);

int create_dir(const char *dir);
int create_cachedirtag(const char *dir);
void x_asprintf(char **ptr, const char *format, ...);
char *x_strdup(const char *s);
void *x_realloc(void *ptr, size_t size);
void *x_malloc(size_t size);
void traverse(const char *dir, void (*fn)(const char *, struct stat *));
char *str_basename(const char *s);
char *dirname(char *s);
int lock_fd(int fd);
size_t file_size(struct stat *st);
int safe_open(const char *fname);
char *x_realpath(const char *path);
char *gnu_getcwd(void);
int create_empty_file(const char *fname);
const char *get_home_directory(void);

void stats_update(enum stats stat);
void stats_zero(void);
void stats_summary(void);
void stats_tocache(size_t size);
void stats_read(const char *stats_file, unsigned counters[STATS_END]);
int stats_set_limits(long maxfiles, long maxsize);
size_t value_units(const char *s);
void display_size(unsigned v);
void stats_set_sizes(const char *dir, size_t num_files, size_t total_size);

int unify_hash(const char *fname);

#ifndef HAVE_VASPRINTF
int vasprintf(char **, const char *, va_list );
#endif
#ifndef HAVE_ASPRINTF
int asprintf(char **ptr, const char *format, ...);
#endif

#ifndef HAVE_SNPRINTF
int snprintf(char *,size_t ,const char *, ...);
#endif

void cleanup_dir(const char *dir, size_t maxfiles, size_t maxsize);
void cleanup_all(const char *dir);
void wipe_all(const char *dir);

int execute(char **argv, 
	    const char *path_stdout,
	    const char *path_stderr);
char *find_executable(const char *name, const char *exclude_name);

typedef struct {
	char **argv;
	int argc;
} ARGS;


ARGS *args_init(int , char **);
void args_add(ARGS *args, const char *s);
void args_add_prefix(ARGS *args, const char *s);
void args_pop(ARGS *args, int n);
void args_strip(ARGS *args, const char *prefix);
void args_remove_first(ARGS *args);

#if HAVE_COMPAR_FN_T
#define COMPAR_FN_T __compar_fn_t
#else
typedef int (*COMPAR_FN_T)(const void *, const void *);
#endif

/* work with silly DOS binary open */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* mkstemp() on some versions of cygwin don't handle binary files, so
   override */
#ifdef __CYGWIN__
#undef HAVE_MKSTEMP
#endif

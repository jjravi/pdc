#include "pdc_utils.h"
#include "pdc_private.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define log(M, ...) fprintf(stdout, "[%s:%d] " M "\n", strrchr(__FILE__, '/') > 0 \
  ? strrchr(__FILE__, '/') + 1 : __FILE__ , __LINE__, ##__VA_ARGS__); 
#define error(M, ...) fprintf(stderr, "[%s:%d] " M " %s\n", strrchr(__FILE__, '/') > 0 \
  ? strrchr(__FILE__, '/') + 1 : __FILE__ , __LINE__, ##__VA_ARGS__, strerror(errno)); 

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <link.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "list.h"
// #include "tmplibrary.h"
// #include "debug.h"

#ifdef Linux
#include "memfd.h"
#endif

// #include "decompress.h"

extern char **environ;

/*

   So.. We don't want to bother with reflective bla-bla-bla. Just
   upload buffer to temporary file, load it as a library using standard
   glibc calls, then delete

 */

static inline
const char *gettemptpl() {
  static const char *templates[] = {
#ifdef Linux
    "/dev/shm/XXXXXX",
    "/run/shm/XXXXXX",
    "/run/",
#endif
    "/tmp/XXXXXX",
    "/var/tmp/XXXXXX",
    NULL
  };

  static const char *tmpdir = NULL;
  if (! tmpdir) {
    int i;
    for (i=0; templates[i]; i++) {
      char *buf = alloca(strlen(templates[i]+1));
      strcpy(buf, templates[i]);
      int fd = mkstemp(buf);
      int found = 0;
      if (fd != -1) {
        int page_size = sysconf(_SC_PAGESIZE);
        if (ftruncate(fd, page_size) != -1) {
          void *map = mmap(
            NULL,
            page_size,
            PROT_READ|PROT_EXEC,
#ifdef Linux
            MAP_PRIVATE|MAP_DENYWRITE,
#else
            MAP_PRIVATE,
#endif
            fd,
            0
            );
          if (map != MAP_FAILED) {
            munmap(map, page_size);
            found = 1;
          } else {
            printf("Couldn't use %s -> %m\n", buf);
          }
        }

        unlink(buf);
        close(fd);

        if (found) {
          tmpdir = templates[i];
          break;
        }
      }
      printf("TRY: %s -> %d (%m)\n", buf, fd);

    }
    if (!tmpdir) {
      abort();
    }
  }

  return tmpdir;
}

char *remove_relative_dirs(char *workingDir, char *application)
{
  char *ret_value = NULL;
  int   k, levels_up = 0;
  char *appName = application;

  char *dotdot;

  FUNC_ENTER(NULL);

  while ((dotdot = strstr(appName, "../")) != NULL) {
    levels_up++;
    appName = dotdot + 3;
  }
  for (k = 0; k < levels_up; k++) {
    char *slash = strrchr(workingDir, '/');
    if (slash)
      *slash = 0;
  }
  k = strlen(workingDir);
  if ((appName[0] == '.') && (appName[1] == '/'))
    appName += 2;
  sprintf(&workingDir[k], "/%s", appName);

  ret_value = strdup(workingDir);

  FUNC_LEAVE(ret_value);
}

char *PDC_find_in_path(char *workingDir, char *application)
{
  struct stat fileStat;
  char *      ret_value = NULL;
  char *      pathVar   = getenv("PATH");
  char        colon     = ':';

  char  checkPath[PATH_MAX];
  char *next = strchr(pathVar, colon);
  int   offset;

  FUNC_ENTER(NULL);

  while (next) {
    *next++ = 0;
    sprintf(checkPath, "%s/%s", pathVar, application);
    if (stat(checkPath, &fileStat) == 0) {
      PGOTO_DONE(strdup(checkPath));
    }
    pathVar = next;
    next    = strchr(pathVar, colon);
  }
  if (application[0] == '.') {
    sprintf(checkPath, "%s/%s", workingDir, application);
    if (stat(checkPath, &fileStat) == 0) {
      char *foundPath = strrchr(checkPath, '/');
      char *appName   = foundPath + 1;
      if (foundPath == NULL) {
        PGOTO_DONE(remove_relative_dirs(workingDir, application));
      }
      *foundPath = 0;
      // Change directory (pushd) to the where we find the application
      if (chdir(checkPath) == 0) {
        if (getcwd(checkPath, sizeof(checkPath)) == NULL) {
          printf("Path is too large\n");
        }

        offset = strlen(checkPath);
        // Change back (popd) to where we started
        if (chdir(workingDir) != 0) {
          printf("Check dir failed\n");
        }
        sprintf(&checkPath[offset], "/%s", appName);
        PGOTO_DONE(strdup(checkPath));
      }
    }
  }
done:
  FUNC_LEAVE(ret_value);
}

/*
 * NOTE:
 *   Because we use dlopen to dynamically open
 *   an executable, it may be necessary for the server
 *   to have the LD_LIBRARY_PATH of the client.
 *   This can/should be part of the UDF registration
 *   with the server, i.e. we provide the server
 *   with:
 *      a) the full path to the client executable
 *         which must be compiled with the "-fpie -rdynamic"
 *         flags.
 *      b) the contents of the PATH and LD_LIBRARY_PATH
 *         environment variables.
 */

char *PDC_get_argv0_()
{
  char *       ret_value          = NULL;
  static char *_argv0             = NULL;
  char         fullPath[PATH_MAX] = {0,};
  char currentDir[PATH_MAX] = {0,};
  pid_t  mypid = getpid();
  char * next;
  char * procpath = NULL;
  FILE * shellcmd = NULL;
  size_t cmdLength;

  FUNC_ENTER(NULL);

  if (_argv0 == NULL) {
    // UNIX derived systems e.g. linux, allow us to find the
    // command line as the user (or another application)
    // invoked us by reading the /proc/{pid}/cmdline
    // file.  Note that I'm assuming standard posix
    // file paths, so the directory seperator is a foward
    // slash ('/') and relative paths will include dot ('.')
    // dot-dot ("..").
    sprintf(fullPath, "/proc/%u/cmdline", mypid);
    procpath = strdup(fullPath);
    shellcmd = fopen(procpath, "r");
    if (shellcmd == NULL) {
      free(procpath);
      PGOTO_ERROR(NULL, "fopen failed!");
    }
    else {
      cmdLength = fread(fullPath, 1, sizeof(fullPath), shellcmd);
      if (procpath)
        free(procpath);
      if (cmdLength > 0) {
        _argv0 = strdup(fullPath);
        /* truncate the cmdline if any whitespace (space or tab) */
        if ((next = strchr(_argv0, ' ')) != NULL) {
          *next = 0;
        }
        else if ((next = strchr(_argv0, '\t')) != NULL) {
          *next = 0;
        }
      }
      fclose(shellcmd);
    }
    if (_argv0[0] != '/') {
      if (getcwd(currentDir, sizeof(currentDir)) == NULL) {
        PGOTO_ERROR(NULL, "Very long path name detected.");
      }
      next = PDC_find_in_path(currentDir, _argv0);
      if (next == NULL)
        PGOTO_ERROR(NULL, "WARNING: Unable to locate application (%s) in user $PATH", _argv0);
      else {
        /* Get rid of the copy (strdup) of fullPath now in _argv0.
         * and replace it with the next (modified/fully_qualified?) version.
         */
        free(_argv0);
        _argv0 = next;
      }
    }
  }
  if (_argv0 == NULL)
    PGOTO_ERROR(NULL, "ERROR: Unable to resolve user application name!");

  ret_value = _argv0;
done:
  FUNC_LEAVE(ret_value);
}

char *PDC_get_realpath(char *fname, char *app_path)
{
  char *ret_value = NULL;
  int   notreadable;
  char  fullPath[PATH_MAX] = {0,};

  FUNC_ENTER(NULL);

  do {
    if (app_path) {
      sprintf(fullPath, "%s/%s", app_path, fname);
      app_path = NULL;
    }
    notreadable = access(fullPath, R_OK);
    if (notreadable && (app_path == NULL)) {
      perror("access");
      PGOTO_DONE(NULL);
    }
  } while (notreadable);

  ret_value = strdup(fullPath);
done:
  FUNC_LEAVE(ret_value);
}


#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>
#include "symbols.h"

typedef struct {
  void * data;
  int size;
  int current;
} lib_t;

lib_t libdata;

static bool load_library_from_file(char * path, lib_t *libdata) {
  struct stat st;
  FILE * file;
  size_t read;

  if ( stat(path, &st) < 0 ) {
    error("failed to stat");
    return false;
  }

  log("lib size is %zu", st.st_size); 

  libdata->size = st.st_size;
  libdata->data = malloc( st.st_size );
  libdata->current = 0;

  file = fopen(path, "r");

  read = fread(libdata->data, 1, st.st_size, file); 
  log("read %zu bytes", read);

  fclose(file);

  return true;
}

static int my_func(const char *libpath, const char *libname, const char *objname,
  const void *addr, const size_t size,
  const symbol_bind binding, const symbol_type type,
  void *custom __attribute__((unused)))
{
  printf("%s (%s):", libpath, libname);

  if (*objname)
    printf(" %s:", objname);
  else
    printf(" unnamed");

  if (size > 0)
    printf(" %zu-byte", size);

  if (binding == LOCAL_SYMBOL)
    printf(" local");
  else
    if (binding == GLOBAL_SYMBOL)
      printf(" global");
    else
      if (binding == WEAK_SYMBOL)
        printf(" weak");

  if (type == FUNC_SYMBOL)
    printf(" function");
  else
    if (type == OBJECT_SYMBOL || type == COMMON_SYMBOL)
      printf(" variable");
    else
      if (type == THREAD_SYMBOL)
        printf(" thread-local variable");

  printf(" at %p\n", addr);
  fflush(stdout);

  return 0;
}

int PDC_get_ftnPtr_(const char *ftn, const char *loadpath, void **ftnPtr)
{
  int          ret_value  = 0;
  static void *appHandle  = NULL;
  void *       ftnHandle  = NULL;
  char *       this_error = NULL;

  FUNC_ENTER(NULL);

  if (appHandle == NULL) {
    if ((appHandle = dlopen(loadpath, RTLD_NOW)) == NULL) {
      this_error = dlerror();
      PGOTO_ERROR(-1, "dlopen failed: %s", this_error);
    }
  }

  // symbols(my_func, NULL);
  ftnHandle = dlsym(appHandle, ftn);
  if (ftnHandle == NULL)
    PGOTO_ERROR(-1, "dlsym failed: %s", dlerror());

  *ftnPtr = ftnHandle;

  ret_value = 0;

done:
  FUNC_LEAVE(ret_value);
}

int PDC_get_var_type_size(pdc_var_type_t dtype)
{
  int ret_value = 0;

  FUNC_ENTER(NULL);

  /* TODO: How to determine the size of compound types and or
   * the other enumerated types currently handled by the default
   * case which returns 0.
   */
  switch (dtype) {
    case PDC_INT:
      ret_value = sizeof(int);
      goto done;
      break;
    case PDC_FLOAT:
      ret_value = sizeof(float);
      goto done;
      break;
    case PDC_DOUBLE:
      ret_value = sizeof(double);
      goto done;
      break;
    case PDC_CHAR:
      ret_value = sizeof(char);
      goto done;
      break;
    case PDC_INT16:
      ret_value = sizeof(int16_t);
      goto done;
      break;
    case PDC_INT8:
      ret_value = sizeof(int8_t);

      goto done;
      break;
    case PDC_INT64:
      ret_value = sizeof(int64_t);
      goto done;
      break;
    case PDC_UINT64:
      ret_value = sizeof(uint64_t);
      goto done;
      break;
    case PDC_UINT:
      ret_value = sizeof(uint);
      goto done;
      break;
    case PDC_UNKNOWN:
    default:
      PGOTO_ERROR(
        0,
        "PDC_get_var_type_size: WARNING - Using an unknown datatype"); /* Probably a poor default */
      break;
  }
done:
  FUNC_LEAVE(ret_value);
}

void print_symbols()
{
  FUNC_ENTER();
  symbols(my_func, NULL);
  FUNC_LEAVE(NULL);
}


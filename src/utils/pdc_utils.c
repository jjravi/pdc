#include "pdc_utils.h"
#include "pdc_private.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

char *
remove_relative_dirs(char *workingDir, char *application)
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

char *
PDC_find_in_path(char *workingDir, char *application)
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

char *
PDC_get_argv0_()
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

char *
PDC_get_realpath(char *fname, char *app_path)
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

int
PDC_get_ftnPtr_(const char *ftn, const char *loadpath, void **ftnPtr)
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
  ftnHandle = dlsym(appHandle, ftn);
  if (ftnHandle == NULL)
    PGOTO_ERROR(-1, "dlsym failed: %s", dlerror());

  *ftnPtr = ftnHandle;

  ret_value = 0;

done:
  fflush(stdout);
  FUNC_LEAVE(ret_value);
}

int
PDC_get_var_type_size(pdc_var_type_t dtype)
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


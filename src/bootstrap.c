#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN
#error UNIMPLEMENTED
#else
#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <dirent.h>
#endif

#define MAX_LENGTH_NAME 128
#define MAX_LENGTH_PATH 1024
#define CACHE_PATH   "res/.cache"
#define DEPENDS_PATH ".cut/depends.list"

/* CUT BOOTSTRAP:
 * This is the bootstrap for CUT, it runs without CUT dependencies
 */

// TODO: This is terrible code, refactor ASAP

////////////////////////////////////////////////////////////////////////////////
// STRUCTS
////////////////////////////////////////////////////////////////////////////////
struct record {
  char header[MAX_LENGTH_NAME];
  char package[MAX_LENGTH_NAME];
  int  lastmod;
};

struct cache {
  struct record *records;
  int            size;
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
////////////////////////////////////////////////////////////////////////////////
struct cache   CACHE    = { NULL, 0 };
char         **PACKAGES =   NULL;
const char    *CUT_HOME;
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
struct cache parse_cache()
{
  char          buffer[MAX_LENGTH_PATH];
  FILE         *cfile;
  struct cache  cache;

  sprintf(buffer, "%sCUT/%s", CUT_HOME, CACHE_PATH);
  cfile = fopen(buffer, "r");

  if (cfile) {
    cache.size = 0;
    for (char c = fgetc(cfile); c != EOF; c = fgetc(cfile)) if (c == '\n') cache.size++;
    fseek(cfile, 0, SEEK_SET);
    cache.records = calloc(cache.size + 1, sizeof(struct record));

    for (int i = 0; i < cache.size; i++) {
      fscanf(cfile, "%s%s%d", 
            cache.records[i].header, 
            cache.records[i].package, 
            &cache.records[i].lastmod);
    }

    fclose(cfile);
  }

  return cache;
}

////////////////////////////////////////////////////////////////////////////////
void check_includes(char list[][MAX_LENGTH_NAME], const char *filename)
{
  FILE       *incf      = fopen(filename, "r");
  const char *directive = "include <";
  const int   size      = strlen(directive);
  
  if (incf) {
    for (char c = fgetc(incf); c != EOF; c = fgetc(incf)) {
      while (c == '#') {
        char buffer[size + 1];

        memset(buffer, 0, size + 1);
        for (int i = 0; i < size; i++) {
          c = fgetc(incf);
          if (c == '#' || c == EOF) break;
          buffer[i] = c;
        }
        if (!strcmp(buffer, directive)) {
          char file[MAX_LENGTH_NAME];
        
          memset(file, 0, sizeof(file));
          c = fgetc(incf);
          for (int i = 0; c != '>' && c != EOF; c = fgetc(incf), i++) {
            file[i] = c;
          }

          for (int i = 0; i < CACHE.size; i++) {
            if (!strcmp(CACHE.records[i].header, file)) {
              char n[MAX_LENGTH_PATH];

              sprintf(n, "%s%s/inc", CUT_HOME, CACHE.records[i].package);
              for (int i = 0; i < CACHE.size; i++) {
                if (!strcmp(n, list[i])) break;
                if (!list[i][0]) {
                  sprintf(list[i], "%s", n);
                  printf("    -I%s\n", list[i]);
                  break;
                }
              }

              sprintf(n, "%s%s/inc/%s", CUT_HOME, CACHE.records[i].package, file);

              //printf("%s\n", n);
              //if (!strcmp(file, "macro.h")) exit(1);
              check_includes(list, n);
            }
          }
        }
      } 
      while (c != '\n' && c != EOF) c = fgetc(incf);
    }

    fclose(incf);
  }
}

////////////////////////////////////////////////////////////////////////////////
void depends(const char *path, const char *name)
{
  char buffer[MAX_LENGTH_PATH];
  char list[CACHE.size][MAX_LENGTH_NAME];

  printf("%s:\n", name);

  for (int i = 0; i < CACHE.size; i++) memset(list[i], 0, MAX_LENGTH_NAME);

  sprintf(buffer, "%s%s", path, name);
  check_includes(list, buffer);

  for (int i = 0; i < CACHE.size; i++) {
    if (!list[i][0]) break;
    for (int j = 0; list[i][j] && j < MAX_LENGTH_NAME; j++) {
      if (list[i][j] == CUT_HOME[j]) buffer[j] = list[i][j];
      else {
        do buffer[j] = list[i][j];
        while (list[i][++j] && list[i][j] != '/' && j < MAX_LENGTH_NAME);
        buffer[j] = 0;
        break;
      }
    }
    for (int j = 0; j < CACHE.size; j++) {
      if (!PACKAGES[j]) {
        PACKAGES[j] = malloc(strlen(buffer) + 1);
        sprintf(PACKAGES[j], "%s", buffer);
        break;
      }
      if (!strcmp(buffer, PACKAGES[j])) break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void cache(const char *path, const char *name)
{
  char        package[256];
  int         time;

  {
    int  l = strlen(path);
    int  i, j;
    char pkg_path[1024];

    for (i = 0, j = 0; CUT_HOME[i] == path[i]; i++);
    while (path[i] != '/') package[j++] = path[i++];
    memcpy(pkg_path, path, i);
    pkg_path[i] = 0;
    while (i < l - 5) package[j++] = path[i++];
    package[j] = 0;

    {
      char  buffer[MAX_LENGTH_PATH << 1];
      FILE *result;

      sprintf(buffer, "stat -c %%Y %s", pkg_path);

      result = popen(buffer, "r");
      memset(buffer, 0, sizeof(buffer));

      for (int c = fgetc(result), i = 0; c != EOF; c = fgetc(result), i++) buffer[i] = c;

      time = atoi(buffer);

      fclose(result);
    }
  }
  
  printf("%-32s%-32s%-16d\n", name, package, time);
}

////////////////////////////////////////////////////////////////////////////////
void list_dir(const char *path, void (*func)(const char*, const char*))
{
  DIR *dir  = opendir(path);

  if (dir) {
    struct dirent *d;

    while ((d = readdir(dir))) {
      if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..") || d->d_name[0] == '.') continue;
      
      switch (d->d_type) {
        case DT_DIR:
          char buffer[MAX_LENGTH_PATH];

          sprintf(buffer, "%s%s/", path, d->d_name);
          list_dir(buffer, func);
          break;
        case DT_REG:
          {
            int len = strlen(d->d_name);

            if (!strcmp(path + strlen(path) - 4, "inc/") &&
                ((len > 2 && !strcmp(d->d_name + len - 2, ".h")) ||
                 (len > 4 && !strcmp(d->d_name + len - 4, ".hpp"))))
            {
              // This is a header file in an "inc" folder
              func(path, d->d_name);
            }
            break;
          }
        default:
          break;
      }
    }

    closedir(dir);
  }
}

////////////////////////////////////////////////////////////////////////////////
void print_section(char *section)
{
  FILE *depends = fopen(DEPENDS_PATH, "r");

  if (depends) {
    for (char c = fgetc(depends); c != EOF; c = fgetc(depends)) {
      for (int i = 0; c == section[i]; c = fgetc(depends), i++) {
        if (section[i] == '.') {
          // readline
          while (c != '\n' && c != EOF) c = fgetc(depends);
          while ((c = fgetc(depends)) == ' ') {
            fgetc(depends); fgetc(depends);
            // readline
            while (c != '\n' && c != EOF) {
              c = fgetc(depends);
              putchar(c);
            }
          }

          fflush(stdout);
          fseek(depends, 0, SEEK_END);
          break;
        }
      }
      // readline
      while (c != '\n' && c != EOF) c = fgetc(depends);
    }
    fclose(depends);
  }
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  CUT_HOME = getenv("CUT_HOME");

  if (!CUT_HOME[0]) {
    fprintf(stderr, "%s\n", "No CUT_HOME environment variable defined... exiting!");
    fprintf(stderr, "%s\n", "Try: export CUT_HOME=\"/path/to/CUT/../\"");
    exit(1);
  }

  if (argc > 1) {
    if (!strcmp(argv[1], "--cache")) {
      // Build cache
      list_dir(CUT_HOME, cache);
    } else if (!strcmp(argv[1], "--depends")) {
      // Dependency walk
      CACHE    = parse_cache();
      PACKAGES = calloc(CACHE.size, sizeof(char*));

      for (int i = 2; i < argc; i++) {
        //printf("%s\n", argv[i]);
        list_dir(argv[i], depends);
      }

      printf("packages:\n");
      for (int i = 0; PACKAGES[i]; i++) {
        printf("    %s\n", PACKAGES[i]);
        free(PACKAGES[i]);
      }

      free(PACKAGES);
      free(CACHE.records);
    } else if (!strcmp(argv[1], "--include")) {
      print_section(argv[2]);
    } else if (!strcmp(argv[1], "--library")) {
      print_section("packages");
    }
  }

  return 0;
}
////////////////////////////////////////////////////////////////////////////////

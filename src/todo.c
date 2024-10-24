// CUT
#include <directory.h>
#include <terminal.h>
#include <filestream.h>
#include <str.h>
#include <string.charstream.h>
#include <collection.str.h>

#define WIDTH_FILE 40
#define WIDTH_LINE  5
#define WIDTH_PRIO 15
#define WIDTH_WHAT 20
#define WIDTH_DESC 40

void printrep(char *file, char *line, char *prio, char *what, char *desc) {
  char filebuf[WIDTH_FILE + 1];
  char linebuf[WIDTH_LINE + 1];
  char priobuf[WIDTH_PRIO + 1];
  char whatbuf[WIDTH_WHAT + 1];
  char descbuf[WIDTH_DESC + 1];

  ljust(file, filebuf, WIDTH_FILE);
  rjust(line, linebuf, WIDTH_LINE);
  cjust(prio, priobuf, WIDTH_PRIO);
  cjust(what, whatbuf, WIDTH_WHAT);
  ljust(desc, descbuf, WIDTH_DESC);

  if (!strcmp(prio, "(low)")) {
    printf(TEXT_GREEN);
  } else if (!strcmp(prio, "(normal)")) {
    printf(TEXT_YELLOW);
  } else if (!strcmp(prio, "(high)")) {
    printf(TEXT_RED);
  } else if (!strcmp(prio, "PRIORITY")) {
    printf(FONT_ULINE);
    printf(FONT_BOLD);
  }

  printf("%s%s%s%s%s\n", filebuf, linebuf, priobuf, whatbuf, descbuf);

  printf(FONT_RESET);
}

int todo(int argc, char *argv[])
{
  if (strcmp(argv[0], ""))
  {
    printrep("FILE", "LINE", "PRIORITY", "TYPE", "DESCRIPTION");
  }

  const char *dirname = argc > 1 ? argv[1] : ".";

  for (DirectoryIterator *di = dopen(dirname); di; dnext(&di))
  {
    char fullname[4096];
    char extension[32];

    sprintf(fullname, "%s%s", di->path, di->current.name);
    fileext(extension, di->current.name);

    if (di->current.type == DIRTYPE_FILE && (!strcmp(extension, ".c") || !strcmp(extension, ".h")))
    {
      CharStream *file = (CharStream*) NEW (FileStream) (fopen(fullname, "r"));

      if (file) {
        String *line;

        while ((line = CharStream_getline(file))) {
          int num      = 0;
          int icomment = String_cont(line, "//");
          int index    = String_cont(line, "TODO:");

          if (icomment >= 0 && index > icomment) {
            String *desc  = NEW (String) (String_trim(String_substr(line, index + 5, 0))->base);
            Array  *split = (Array*)String_split(line, ":");
            char    linenum[WIDTH_LINE + 1];

            sprintf(linenum, "%d", num);

            if (split->size == 3) {
              printrep(di->current.name, linenum, 
                String_trim(Array_at(split, 0))->base, 
                String_trim(Array_at(split, 1))->base,
                String_trim(Array_at(split, 2))->base);
            } else {
              printrep(di->current.name, linenum, "(normal)", "Not categorized", desc->base);
            }

            DELETE (desc);
            DELETE (split);
          } else {
            DELETE (line);
          }
        }

        DELETE (file);
      }
    }
    else if (di->current.type == DIRTYPE_DIRECTORY && di->current.name[0] != '.')
    {
      char *nargv[2] = { "", fullname };

      todo(2, nargv);
    }
  }

  return 0;
}
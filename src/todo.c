#include <stdio.h>
#include <string.h>

#include <directory.h>
#include <array.h>
#include <terminal.h>
#include <file.h>
#include <stream.h>
#include <str.h>

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
  } else if (!strcmp(prio, "(medium)")) {
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

int todo()
{
  String *todo  = newString("TODO:");
  String *colon = newString(":");
  Array  *items;
  
  if (argc < 2) {
    printrep("FILE", "LINE", "PRIORITY", "TYPE", "DESCRIPTION");
    items = directory(".");
  } else items = directory(argv[1]);

  for (int i = 0; i < items->size; i++) {
    DirectoryItem *di = at(items, i);

    if (di->type == DIRITEM_FILE) {
      Stream *file = fromFileStream(fopen(di->name, "r"));

      if (file) {
        String *line;

        for (int num = 0, index; (line = sgetline(file)); num++) {
          if ((index = contains(line, todo)) > -1) {
            String *desc = substring(newString(line->content), index + todo->length, 0);
            String *what = NULL;
            String *prio = NULL;

            if ((index = contains(desc, colon))) {
              prio = trim(substring(newString(desc->content), 0, index));
              substring(desc, index + 1, 0);

              // Correctly formed prioriy
              if (prio->content[0] == '(' && prio->content[prio->length - 1] == ')') {
                if ((index = contains(desc, colon))) {
                  what = trim(substring(newString(desc->content), 0, index));
                  trim(substring(desc, index + 1, 0));
                }
              }
            }
            
            if (prio && desc && what) {
              char line[WIDTH_LINE + 1];

              sprintf(line, "%d", num);
              printrep(di->name, line, prio->content, what->content, desc->content);
            }

            deleteString(&prio);
            deleteString(&what);
            deleteString(&desc);
          }
          deleteString(&line);
        }
      }

      sclose(file);
    } else if (di->type == DIRITEM_DIRECTORY) {
      char *nargv[2] = { "", di->name };
      main(2, nargv);
    }
  }
  while (items->size) popobj(items, freedi);
  deleteArray(&items);
  deleteString(&todo);
  deleteString(&colon);

  return 0;
}
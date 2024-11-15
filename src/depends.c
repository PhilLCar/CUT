#include <directory.h>
#include <oop.h>
#include <exception.h>
#include <filestream.h>
#include <string.charstream.h>
#include <objectarray.h>
#include <collection.str.h>
#include <string.h>
#include <list.h>
#include <cacherecord.h>

int record_comparer(CacheRecord *record, String *filename)
{
  return String_Compare(record->file, filename);
}

void get_includes(ObjectArray *cache, List *travelled, const char *filename)
{
  CharStream *stream = (CharStream*)NEW (FileStream)(fopen(filename, "r"));

  while (!stream->base.eos) {
    String *line = CharStream_GetLine(stream);

    if (line) {
      if (String_StartsWith(line, "#include")) {
        String_SubString(line, 8, 0);
        String_Trim(line);

        if (String_StartsWith(line, "<")) {
          int end = line->length - String_Cnt(line, ">");

          String_SubString(line, 1, -end);

          CacheRecord *record = ObjectArray_In(cache, line, (Comparer)record_comparer);

          if (record) {
            String *filename = NULL;//find(record);

            if (!List_In(travelled, filename, (Comparer)String_Compare))
            {
              List *step = List_Push(travelled, filename);

              get_includes(cache, step, filename->base);

              List_Pop(step, NULL);
            }

            DELETE (filename);
          }
        }
      }
    }

    DELETE (line);
  }

  DELETE (stream);
}

int main(int argc, char *argv[])
{
  // char cachefile[2048];

  // root = getenv("CUT_HOME");

  // if (!root[0]) {
  //   THROW(NEW (Exception)("No CUT_HOME environment variable defined... exiting!"));
  // } else {
  //   rootlen = strlen(root);

  //   sprintf(cachefile, "%s%s", root, "CUT/.cut/.cache");
  // }

  // FILE *cache = fopen(cachefile, "w+");

  // build_cache(cache, root);

  // fclose(cache);

  // depends
}
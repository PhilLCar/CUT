#include <diagnostic.h>
#include <directory.h>
#include <oop.h>
#include <exception.h>
#include <filestream.h>
#include <string.charstream.h>
#include <objectarray.h>
#include <collection.str.h>
#include <string.h>
#include <list.h>
#include <cachefile.h>
#include <args.h>
#include <set.h>
#include <mapfile.h>
#include <string.file.h>

const char *home;

int sources = 0;

void option_sources(Args *args, ArgValue value)
{
  sources = value.as_integer;
}

OPTIONS(
  { "sources",           's', "Finds dependencies in sources instead of headers", ARG_TYPE_BOOLEAN, option_sources },
  { "working-directory", '+', "Specifies the working directory",                  ARG_TYPE_CHARPTR, NULL           }
);


int record_comparer(CacheRecord *record, String *filename)
{
  return String_Compare(record->file, filename);
}

String *find(const char *base, const char *destination)
{
  String *found = NULL;
  char    path[2048];

  for (DirectoryIterator *di = dopen(base); di; dnext(&di)) {
    dfullname(di, path, sizeof(path));

    if (!strcmp(destination, di->current.name))
    {
      found = NEW (String) (path);
    } else if (di->current.type == DIRTYPE_DIRECTORY) {
      if (di->current.name[0] != '.') {
        found = find(path, destination);
      }
    }

    if (found) {
      dclose(&di);
      break;
    }
  }

  return found;
}

void get_includes(CacheFile *cache, Set *packages, Set *includes, List *travelled, const char *filename)
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

          CacheRecord *record = ObjectArray_In((ObjectArray*)cache, line, (Comparer)record_comparer);

          if (record) {
            String *packagename = find(home,              record->package->base);
            String *filename    = find(packagename->base, record->file->base);

            if (filename && !List_In(travelled, filename, (Comparer)String_Compare)) {
              List *step = List_Push(travelled, filename, 1);

              get_includes(cache, packages, includes, step, filename->base);

              Set_Add(packages, packagename);
              Set_Add(includes, Path_Folder(filename->base));

              List_Pop(step, NULL);
            } else {
              DELETE (packagename);
            }
          }
        }
      }
    }

    DELETE (line);
  }

  DELETE (stream);
}

void get_files(MapFile *depends, CacheFile *cache, Set *packages, const char *folder)
{
  for (DirectoryIterator *di = dopen(folder); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY) {
      if (di->current.name[0] != '.') {
        char directory[2048];

        dfullname(di, directory, sizeof(directory));
        get_files(depends, cache, packages, directory);
      }
    } else {
      char ext[8];

      fileext(di->current.name, ext, sizeof(ext));

      if ((sources && (!strcmp(ext, ".c") || !strcmp(ext, ".cpp")))
      || (!sources && (!strcmp(ext, ".h") || !strcmp(ext, ".hpp")))) {
        List *travelled = NEW (List)();
        char  filename[2048];

        dfullname(di, filename, sizeof(filename));

        Set *includes = Map_Set((Map*)depends,
                          NEW (String) (di->current.name),
                          NEW (Set) (TYPEOF (String), (Comparer)String_Compare)
                        )->second.object;
        
        get_includes(cache, packages, includes, travelled, filename);

        DELETE (travelled);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  CHECK_MEMORY

  Args       *args        = NEW (Args) (argc, argv, NULL);
  const char *workdir     = Args_Name(args, "working-directory").as_charptr;
  String     *dependsfile = Path_Combine(workdir, ".cut/depends.map");
  String     *cachefile   = NULL;

  home = getenv("CUT_HOME");

  if (!home || !home[0]) {
    THROW (NEW (Exception) ("No CUT_HOME environment variable defined... exiting!"));
  } else {
    cachefile = Path_Combine(home, "CUT/.cut/.cache");
  }

  CacheFile *cache    = NEW (CacheFile)(cachefile->base, FILEACCESS_READ);
  MapFile   *depends  = NEW (MapFile)(dependsfile->base, FILEACCESS_WRITE);
  Set       *packages = Map_Set((Map*)depends,
                          NEW (String) ("packages"),
                          NEW (Set) (TYPEOF (String), (Comparer)String_Compare)
                        )->second.object;

  get_files(depends, cache, packages, workdir);

  DELETE (cache)
  DELETE (depends);
  DELETE (dependsfile);
  DELETE (cachefile);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
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
#include <mapfile.h>
#include <string.file.h>

typedef struct {
  const char *home;
  int         source;
  int         update;
} Env;

void option_source(Args *args, ArgValue value)
{
  ((Env*)args->base)->source = value.as_integer;
}

void option_update(Args *args, ArgValue value)
{
  ((Env*)args->base)->update = value.as_integer;
}

OPTIONS(
  { "source",            's', "Finds dependencies in sources instead of headers",    ARG_TYPE_BOOLEAN, option_source },
  { "update",            'u', "Only rebuild the outdated parts of the depends file", ARG_TYPE_BOOLEAN, option_update },
  { "working-directory", '+', "Specifies the working directory",                     ARG_TYPE_CHARPTR, NULL          }
);

void add_dependecy(const List *travelled, Comparer orderer, ObjectArray *dependencies, String *value)
{
  int insert = -1;
  int prev   = -1;

  for (int i = 0; i < dependencies->base.size; i++) {
    String *current = Array_At((Array*)dependencies, i);

    if (String_Equals(current, value)) {
      prev = i;
      break;
    }

    // If other dependency depends on it, place it before
    if (insert < 0 && List_In(travelled, current, orderer)) {
      insert = i;
    }
  }

  if (prev >= 0) {
    if (insert >= 0) {
     ObjectArray_Insert(dependencies, insert, ObjectArray_RemoveAt(dependencies, prev, 1));
    }

    DELETE(value);
  } else {
    ObjectArray_Push(dependencies, value);
  }
}

int package_orderer(const String *element, const String *against)
{
  return String_Contains(element, against);
}

int include_orderer(const String *element, const String *against)
{
  String* path   = Path_Folder(element->base);
  int     result = String_Compare(path, against);

  DELETE (path);

  return result;
}

int record_comparer(const CacheRecord *record, const String *filename)
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

void get_includes(CacheFile *cache, ObjectArray *packages, ObjectArray *includes, List *travelled, const char *filename, Env *env)
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
            String *packagename = find(env->home,         record->package->base);
            String *filename    = find(packagename->base, record->file->base);

            if (filename && packagename) {
              long modified = statfile(filename->base);

              if ((!env->update || modified > record->lastmod) && !List_In(travelled, filename, (Comparer)String_Compare)) {
                List   *step        = List_Push(travelled, filename, 0);
                String *includename = Path_Folder(filename->base);

                add_dependecy(travelled, (Comparer)package_orderer, packages, packagename);
                add_dependecy(travelled, (Comparer)include_orderer, includes, includename);

                get_includes(cache, packages, includes, step, filename->base, env);

                List_Pop(step, NULL);
              } else {
                DELETE (packagename);
              }
              
              DELETE (filename);
            } else {
              THROW (NEW (Exception) ("File or package wasn't found"));
            }
          }
        }
      }
    }

    DELETE (line);
  }

  DELETE (stream);
}

void get_files(MapFile *depends, CacheFile *cache, ObjectArray *packages, const char *folder, Env *env)
{
  for (DirectoryIterator *di = dopen(folder); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY) {
      if (di->current.name[0] != '.') {
        char directory[2048];

        dfullname(di, directory, sizeof(directory));
        get_files(depends, cache, packages, directory, env);
      }
    } else {
      char ext[8];

      fileext(di->current.name, ext, sizeof(ext));

      if ((env->source && (!strcmp(ext, ".c") || !strcmp(ext, ".cpp")))
      || (!env->source && (!strcmp(ext, ".h") || !strcmp(ext, ".hpp")))) {
        List        *travelled = NEW (List)();
        ObjectArray *includes  = Map_ValueAt((Map*)depends, di->current.name);
        char         filename[2048];

        dfullname(di, filename, sizeof(filename));

        includes = includes 
          ? includes 
          : Map_Set((Map*)depends,
              NEW (String) (di->current.name),
              NEW (ObjectArray) (TYPEOF (String))
            )->second.object;
        
        get_includes(cache, packages, includes, travelled, filename, env);

        DELETE (travelled);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  Env env = { getenv("CUT_HOME"), 0, 0 };

  CHECK_MEMORY

  Args       *args        = NEW (Args) (argc, argv, &env);
  const char *workdir     = Args_Name(args, "working-directory").as_charptr;
  String     *dependsfile = Path_Combine(workdir, ".cut/depends.map");
  String     *cachefile   = NULL;

  if (!env.home || !env.home[0]) {
    THROW (NEW (Exception) ("No CUT_HOME environment variable defined... exiting!"));
  } else {
    cachefile = Path_Combine(env.home, "CUT/.cut/.cache");
  }

  CacheFile   *cache    = NEW (CacheFile)(cachefile->base, FILEACCESS_READ);
  MapFile     *depends  = NEW (MapFile)(dependsfile->base, FILEACCESS_WRITE | (env.update * FILEACCESS_READ));
  ObjectArray *packages = Map_ValueAt((Map*)depends, "packages");
  
  packages = packages 
    ? packages 
    : Map_Set((Map*)depends,
        NEW (String) ("packages"),
        NEW (ObjectArray) (TYPEOF (String))
      )->second.object;

  get_files(depends, cache, packages, workdir, &env);

  DELETE (cache)
  DELETE (depends);
  DELETE (dependsfile);
  DELETE (cachefile);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
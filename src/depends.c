#include <diagnostic.h>
#include <directory.h>
#include <oop.h>
#include <exception.h>
#include <filestream.h>
#include <charstream.h>
#include <set.h>
#include <collection.str.h>
#include <string.h>
#include <list.h>
#include <cachefile.h>
#include <args.h>
#include <mapfile.h>
#include <path.h>

typedef struct {
  const char *home;
  int         source;
  int         update;
} Env;

void option_source(Args *args, ArgValue value)
{
  ((Env*)args->env)->source = value.as_integer;
}

void option_update(Args *args, ArgValue value)
{
  ((Env*)args->env)->update = value.as_integer;
}

OPTIONS(
  { "source",            's', "Finds dependencies in sources instead of headers",    ARG_TYPE_BOOLEAN, option_source },
  { "update",            'u', "Only rebuild the outdated parts of the depends file", ARG_TYPE_BOOLEAN, option_update },
  { "working-directory", '+', "Specifies the working directory",                     ARG_TYPE_CHARPTR, NULL          }
);

int record_comparer(const CacheRecord *record, const String *filename)
{
  return String_Compare(record->key, filename);
}

String *find(const char *base, const char *destination)
{
  String *found = NULL;
  char    path[2048];

  // TODO: Optimize by going one layer at-a-time instead of recursively
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

void get_includes(CacheFile *cache, Set *packages, Set *includes, List *travelled, const char *filename, Env *env)
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

          CacheRecord *record = ObjectArray_ContainsKey((ObjectArray*)cache, line);

          if (record) {
            // TODO: cache find results
            String *packagename = find(env->home,         record->value->base);
            String *filename    = find(packagename->base, record->key->base);

            if (filename && packagename) {
              if (!List_Contains(travelled, filename)) {
                List   *step        = List_Push(travelled, filename, 1);
                String *includename = Path_Folder(filename->base);

                Set_Add(packages, packagename);
                Set_Add(includes, includename);

                get_includes(cache, packages, includes, step, filename->base, env);

                List_Pop(step, NULL);
              } else {
                DELETE (packagename);
              }
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

void get_files(MapFile *depends, CacheFile *cache, Set *packages, const char *folder, Env *env)
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
        List *travelled = NEW (List)();
        Set  *includes;
        char  filename[2048];

        // TODO: compare to cache timestamp

        dfullname(di, filename, sizeof(filename));

        includes = IFNULL(
          Map_ValueAtKey((Map*)depends, di->current.name),
          Map_Set((Map*)depends,
                    NEW (String) (di->current.name),
                    NEW (Set) (TYPEOF (String))
                  )->second.object);
        
        get_includes(cache, packages, includes, travelled, filename, env);

        DELETE (travelled);
      }
    }
  }
}

void get_dependencies(String *package, Set *dependencies) {
  String  *packagedepends = Path_Combine(package->base, ".cut/depends.map");
  MapFile *depends        = NEW (MapFile) (packagedepends->base, ACCESS_READ);

  Array *set = Map_ValueAtKey((Map*)depends, "packages");

  for (int i = 0; i < set->size; i++) {
    String *pkg = Array_At(set, i);

    Set_Add(dependencies, String_Copy(pkg));

    get_dependencies(pkg, dependencies);
  }

  DELETE (depends);
  DELETE (packagedepends);
}

void order_packages(Array *packages)
{
  // for (int i = 0; i < packages->size; i++) {
  //   String *package = Array_At(packages, i);

  //   Set *dependencies = NEW (Set) (TYPEOF (String));
  // }

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

  CacheFile *cache    = NEW (CacheFile) (cachefile->base, ACCESS_WRITE | ACCESS_READ);
  MapFile   *depends  = NEW (MapFile) (dependsfile->base, ACCESS_WRITE | (env.update * ACCESS_READ));
  Set       *packages = IFNULL(
      Map_ValueAtKey((Map*)depends, "packages"), 
      Map_Set((Map*)depends,
                NEW (String) ("packages"),
                NEW (Set) (TYPEOF (String))
              )->second.object);

  get_files(depends, cache, packages, workdir, &env);

  //order_packages(packages);

  DELETE (cache)
  DELETE (depends);
  DELETE (dependsfile);
  DELETE (cachefile);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
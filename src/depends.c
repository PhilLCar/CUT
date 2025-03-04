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
#include <rootfile.h>
#include <graphfile.h>

typedef struct {
  const char *home;
  int         source;
  int         update;
  struct {
    const char *pkgpath;
    long        pkgtime;

  } globals;
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

int update_cache(Env *env, CacheFile *cache, CacheFile *files)
{
  String   *rootpath = Path_Combine(env->globals.pkgpath, ".cut/roots");
  RootFile *roots    = NEW (RootFile) (rootpath->base, ACCESS_READ);
  long      pkgtime  = 0;

  for (Iterator *it = NEW (Iterator)(roots); !done(it); next(it)) {
    String *root = it->base;

    for (int i = 0; i < 1; i++)
    {
      String *folder = String_Cat(Path_Combine(env->globals.pkgpath, root->base), i == 0 ? "/inc" : "/src");

      for (DirectoryIterator *dj = dopen(folder->base); dj; dnext(&dj)) {
        if (dj->current.type == DIRTYPE_FILE) {
          char filepath[PATH_MAX];

          dfullname(dj, sizeof(filepath), filepath);

          long timestamp = statfile(filepath);

          if (timestamp > pkgtime) pkgtime = timestamp;

          CacheFile_Set(files, NEW (String) (dj->current.name), NEW (String) (filepath),             timestamp);
          CacheFile_Set(cache, NEW (String) (dj->current.name), NEW (String) (env->globals.pkgpath), timestamp);
        }
      }

      DELETE (folder);
    }
  }

  DELETE (roots);
  DELETE (rootpath);

  return pkgtime;
}

int update_graph(Env *env, CacheFile *cache, CacheFile *files, GraphFile *dependencies)
{
  String   *rootpath = Path_Combine(env->globals.pkgpath, ".cut/roots");
  RootFile *roots    = NEW (RootFile) (rootpath->base, ACCESS_READ);

  for (Iterator *it = NEW (Iterator)(roots); !done(it); next(it)) {
    String *root = it->base;

    for (int j = 0; j < 1; j++)
    {
      String *folder = String_Cat(Path_Combine(env->globals.pkgpath, root->base), j == 0 ? "/inc" : "/src");

      for (DirectoryIterator *di = dopen(folder->base); di; dnext(&di)) {
        if (di->current.type == DIRTYPE_FILE) {
          CacheRecord *record = CacheFile_GetKey(files, di->current.name);

          if (record && record->timestamp > env->globals.pkgtime) {
            CharStream *stream = (CharStream*)NEW (FileStream)(fopen(record->value, "r"));

            while (!stream->base.eos) {
              String *line = CharStream_GetLine(stream);

              if (line) {
                if (String_StartsWith(line, "#include")) {
                  String_SubString(line, 8, 0);
                  String_Trim(line);

                  if (String_StartsWith(line, "<")) {
                    int end = line->length - String_Cnt(line, ">");

                    String_SubString(line, 1, -end);

                    if (CacheFile_Get(files, line)) {
                      Graph_AddKey((Graph*)dependencies, di->current.name);
                      Graph_AddKey((Graph*)dependencies, line->base);

                      Graph_SetKey((Graph*)dependencies, di->current.name, line->base, 0);
                    } else {
                      return 0;
                    }
                  }
                }
              }

              DELETE (line);
            }

            DELETE (stream);
          }
        }
      }

      DELETE (folder);
    }
  }

  DELETE (roots);
  DELETE (rootpath);

  return 1;
}

void get_includes(CacheFile *cache, Set *packages, Set *includes, List *travelled, const char *filename, Env *env)
{
  
}

// TODO: Provide functions to partially update cache

void get_files(MapFile *depends, CacheFile *cache, Set *packages, const char *package, Env *env)
{  
  for (Iterator *it = NEW (Iterator) (cache); !done(it); next(it))
  {
    KeyVal *keyval = it->base;

    if (String_Eq(keyval->base.second, package)) {
      // This is part of the package
      String *filename = keyval->base.first;

      char ext[8];

      fileext(filename->base, sizeof(ext), ext);

      if ((env->source && (!strcmp(ext, ".c") || !strcmp(ext, ".cpp")))
      || (!env->source && (!strcmp(ext, ".h") || !strcmp(ext, ".hpp")))) {
        Set *includes = IFNULL(
          Map_ValueAt((Map*)depends, filename),
          Map_Set((Map*)depends,
                    String_Copy(filename),
                    NEW (Set) (TYPEOF (String))
                  )->second);
        
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

  Args       *args         = NEW (Args) (argc, argv, &env);
  const char *workdir      = Args_Name(args, "working-directory").as_charptr;
  String     *dependsfile  = Path_Combine(workdir, ".cut/depends.map");
  String     *cachepath    = NULL;
  String     *filespath    = NULL;
  String     *packagespath = NULL;
  String     *dependsgraph = NULL;

  if (!env.home || !env.home[0]) {
    THROW (NEW (Exception) ("No CUT_HOME environment variable defined... exiting!"));
  } else {
    cachepath    = Path_Combine(env.home, "CUT/.cut/.cache");
    filespath    = Path_Combine(env.home, "CUT/.cut/files.cache");
    packagespath = Path_Combine(env.home, "CUT/.cut/packages.cache");
    dependsgraph = Path_Combine(env.home, "CUT/.cut/dependencies.json");
  }

  // TODO: finish this

  CacheFile *cache    = NEW (CacheFile) (cachepath->base,    ACCESS_WRITE | ACCESS_READ);
  CacheFile *files    = NEW (CacheFile) (filespath->base,    ACCESS_WRITE | ACCESS_READ);
  CacheFile *packages = NEW (CacheFile) (packagespath->base, ACCESS_WRITE | ACCESS_READ);
  GraphFile *graph    = NEW (GraphFile) (dependsgraph->base, ACCESS_WRITE | ACCESS_READ);
  
  int pkgtime = update_cache(&env, packages, files, cache);
  int success = update_graph(&env, packages, files, cache, graph);
  
  MapFile   *depends  = NEW (MapFile)   (dependsfile->base, ACCESS_WRITE | (env.update * ACCESS_READ));
  
  Set       *packages = IFNULL(
      Map_ValueAtKey((Map*)depends, "packages"), 
      Map_Set((Map*)depends,
                NEW (String) ("packages"),
                NEW (Set) (TYPEOF (String))
              )->second);

  // Custom comparer to add dependencies in order
  // packages->comparer = 

  get_files(depends, cache, packages, workdir, &env);

  DELETE (cache)
  DELETE (depends);
  DELETE (dependsfile);
  DELETE (cachepath);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
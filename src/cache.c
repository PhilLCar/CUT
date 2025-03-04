#include <diagnostic.h>
#include <directory.h>
#include <exception.h>
#include <cachefile.h>
#include <graph.h>
#include <str.h>
#include <path.h>
#include <args.h>
#include <rootfile.h>
#include <graphfile.h>
#include <mapset.h>

typedef struct {
  const char *home;
  int         source;
} Env;

void option_source(Args *args, ArgValue value)
{
  ((Env*)args->env)->source = value.as_integer;
}

OPTIONS(
  { "source", 's', "Finds dependencies in sources instead of headers", ARG_TYPE_BOOLEAN, option_source }
);

void build_cache(Env *env, CacheFile *packages, CacheFile *files, CacheFile *cache)
{
  // TODO: standardize DirectoryIterator with Iterator
  for (DirectoryIterator *di = dopen(env->home); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY && di->current.name[0] != '.') {
      char      pkgpath[PATH_MAX];
      String   *rootpath;

      dfullname(di, sizeof(pkgpath), pkgpath);

      rootpath = Path_Combine(pkgpath, ".cut/roots");

      if (fileexists(rootpath->base, FILE_EXISTS)) {
        // We're in a CUT project
        CacheRecord *package = CacheFile_Set(packages, NEW (String) (di->current.name), NEW (String) (pkgpath), 0);
        long         pkgtime = 0;
        RootFile    *roots   = NEW (RootFile) (rootpath->base, ACCESS_READ);

        for (Iterator *it = NEW (Iterator) (roots); !done(it); next(it)) {
          String *root = it->base;

          for (int i = 0; i < 1; i++)
          {
            String *folder = String_Cat(Path_Combine(pkgpath, root->base), i == 0 ? "/inc" : "/src");

            for (DirectoryIterator *dj = dopen(folder->base); dj; dnext(&dj)) {
              if (dj->current.type == DIRTYPE_FILE) {
                char filepath[PATH_MAX];

                dfullname(dj, sizeof(filepath), filepath);

                long timestamp = statfile(filepath);

                if (timestamp > pkgtime) pkgtime = timestamp;

                CacheFile_Set(files, NEW (String) (dj->current.name), NEW (String) (filepath),         timestamp);
                CacheFile_Set(cache, NEW (String) (dj->current.name), NEW (String) (di->current.name), timestamp);
              }
            }

            DELETE (folder);
          }
        }

        package->timestamp = pkgtime;

        DELETE (roots);
      }

      DELETE (rootpath);
    }
  }
}

void build_graph(Env *env, CacheFile *packages, CacheFile *files, CacheFile *cache, GraphFile *dependencies)
{
  for (DirectoryIterator *di = dopen(env->home); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY && di->current.name[0] != '.') {
      char      pkgpath[PATH_MAX];
      String   *rootpath;
      RootFile *roots;

      dfullname(di, sizeof(pkgpath), pkgpath);

      rootpath = Path_Combine(pkgpath, ".cut/roots");

      if (fileexists(rootpath->base, FILE_EXISTS)) {
        // We're in a CUT project
        roots = NEW (RootFile) (rootpath->base, ACCESS_READ);

        for (Iterator *it = NEW (Iterator) (roots); !done(it); next(it)) {
          String *root = it->base;

          for (int i = 0; i < 1; i++)
          {
            String *folder = String_Cat(Path_Combine(pkgpath, root->base), i == 0 ? "/inc" : "/src");

            for (DirectoryIterator *dj = dopen(folder->base); dj; dnext(&dj)) {
              if (dj->current.type == DIRTYPE_FILE) {
                CacheRecord *record = CacheFile_GetKey(files, di->current.name);
      
                if (record) {
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
                            Graph_AddKey((Graph*)dependencies, dj->current.name);
                            Graph_AddKey((Graph*)dependencies, line->base);

                            Graph_SetKey((Graph*)dependencies, dj->current.name, line->base, 0);
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
      }

      DELETE (rootpath);
    }
  }
}

int main(int argc, char *argv[])
{
  Env   env  = { "", 0 };
  Args *args = NEW (Args) (argc, argv, &env);

  CHECK_MEMORY

  String *cachepath;
  String *filepath;
  String *pkgpath;
  String *graphpath;

  env.home = getenv("CUT_HOME");

  if (!env.home || !env.home[0]) {
    THROW(NEW (Exception)("No CUT_HOME environment variable defined... exiting!"));
  } else {
    cachepath = Path_Combine(env.home, "CUT/.cut/.cache");
    filepath  = Path_Combine(env.home, "CUT/.cut/files.cache");
    pkgpath   = Path_Combine(env.home, "CUT/.cut/packages.cache");
    graphpath = Path_Combine(env.home, "CUT/.cut/dependencies.json");
  }

  CacheFile *cache    = NEW (CacheFile)(cachepath->base, ACCESS_WRITE);
  CacheFile *files    = NEW (CacheFile)(filepath->base,  ACCESS_WRITE);
  CacheFile *packages = NEW (CacheFile)(pkgpath->base,   ACCESS_WRITE);
  GraphFile *depends  = NEW (GraphFile)(graphpath->base, ACCESS_WRITE);

  CHECK_MEMORY

  build_cache(&env, packages, files, cache);
  build_graph(&env, packages, files, cache, depends);

  CHECK_MEMORY

  DELETE (depends);
  DELETE (packages);
  DELETE (files);
  DELETE (cache);
  DELETE (graphpath);
  DELETE (pkgpath);
  DELETE (filepath);
  DELETE (cachepath);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
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

/*
The goal of this program is to build cache data that is effective enough
while remaining simple enough to be compatible with the bootstrap

*/

typedef struct {
  const char *home;
  int         source;
} Env;

void option_source(Args *args, ArgValue value)
{
  ((Env*)args->env)->source = value.as_integer;
}

OPTIONS(
  { "source", 's', "Include sources in search for dependencies", ARG_TYPE_BOOLEAN, option_source }
);

void build_cache(Env *env, CacheFile *cache, CacheFile *pkgCache, CacheFile *fileCache)
{
  // TODO: (low): Standardize: standardize DirectoryIterator with Iterator
  for (DirectoryIterator *di = dopen(env->home); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY && di->current.name[0] != '.') {
      char      pkgpath[PATH_MAX];
      String   *rootpath;

      dfullname(di, sizeof(pkgpath), pkgpath);

      rootpath = Path_Combine(pkgpath, ".cut/roots");

      if (fileexists(rootpath->base, FILE_EXISTS)) {
        // We're in a CUT project
        CacheRecord *package = CacheFile_Set(pkgCache, NEW (String) (di->current.name), NEW (String) (pkgpath), 0);
        long         pkgtime = 0;
        RootFile    *roots   = NEW (RootFile) (rootpath->base, ACCESS_READ);

        for (Iterator *it = NEW (Iterator) (roots); !done(it); next(it)) {
          String *root = it->base;

          for (int i = 0; i <= env->source; i++)
          {
            String *folder = String_Cat(Path_Combine(pkgpath, root->base), i == 0 ? "/inc" : "/src");

            for (DirectoryIterator *dj = dopen(folder->base); dj; dnext(&dj)) {
              if (dj->current.type == DIRTYPE_FILE) {
                char filepath[PATH_MAX];

                dfullname(dj, sizeof(filepath), filepath);

                long timestamp = statfile(filepath);

                if (timestamp > pkgtime) pkgtime = timestamp;

                CacheFile_Set(fileCache, NEW (String) (dj->current.name), NEW (String) (filepath),         timestamp);
                CacheFile_Set(cache,     NEW (String) (dj->current.name), NEW (String) (di->current.name), timestamp);
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

void build_graph(Env *env, CacheFile *cache, CacheFile *fileCache,  Graph *fileDeps, Graph *pkgDeps)
{
  for (Iterator *it = NEW (Iterator) (fileCache); !done(it); next(it))
  {
    String *file = ((Pair*)it->base)->first;
    String *path = ((CacheRecord*)((Pair*)it->base)->second)->value;

    CharStream *stream = (CharStream*)FileStream_Open(path->base, ACCESS_READ);

    while (!stream->base.eos) {
      String *line = CharStream_GetLine(stream);

      if (line) {
        if (String_StartsWith(line, "#include")) {
          String_SubString(line, 8, 0);
          String_Trim(line);

          if (String_StartsWith(line, "<")) {
            int end = line->length - String_Cnt(line, ">");

            String_SubString(line, 1, -end);

            CacheRecord *src = CacheFile_Get(cache, file);
            CacheRecord *dst = CacheFile_Get(cache, line);

            if (src && dst) {
              // Files graph
              Graph_AddKey(fileDeps, file->base);
              Graph_AddKey(fileDeps, line->base);

              Graph_SetKey(fileDeps, file->base, line->base, 1);

              // Package graph
              Graph_AddKey(pkgDeps, src->value->base);
              Graph_AddKey(pkgDeps, dst->value->base);

              Graph_SetKey(pkgDeps, src->value->base, dst->value->base, 1);
            }
          }
        }
      }

      DELETE (line);
    }

    DELETE (stream);
  }
}

int main(int argc, char *argv[])
{
  Env   env  = { "", 0 };
  Args *args = NEW (Args) (argc, argv, &env);

  CHECK_MEMORY

  String *cachePath;
  String *fileCachePath;
  String *pkgCachePath;
  String *fileGraphPath;
  String *pkgGraphPath;

  env.home = getenv("CUT_HOME");

  if (!env.home || !env.home[0]) {
    THROW(NEW (Exception)("No CUT_HOME environment variable defined... exiting!"));
  } else {
    // TODO: (low): Refactor: Maybe ".cut" should be at the root of HOME and not CUT
    cachePath     = Path_Combine(env.home, "CUT/.cut/.cache");
    fileCachePath = Path_Combine(env.home, "CUT/.cut/files.cache");
    pkgCachePath  = Path_Combine(env.home, "CUT/.cut/packages.cache");
    fileGraphPath = Path_Combine(env.home, "CUT/.cut/files.json");
    pkgGraphPath  = Path_Combine(env.home, "CUT/.cut/packages.json");
  }

  CacheFile *cache     = NEW (CacheFile)(cachePath->base,     ACCESS_WRITE);
  CacheFile *fileCache = NEW (CacheFile)(fileCachePath->base, ACCESS_WRITE);
  CacheFile *pkgCache  = NEW (CacheFile)(pkgCachePath->base,  ACCESS_WRITE);
  GraphFile *fileDeps  = NEW (GraphFile)(fileGraphPath->base, ACCESS_WRITE);
  GraphFile *pkgDeps   = NEW (GraphFile)(pkgGraphPath->base,  ACCESS_WRITE);

  CHECK_MEMORY

  build_cache(&env, cache, fileCache, pkgCache);
  build_graph(&env, cache, fileCache, fileDeps, pkgDeps);

  CHECK_MEMORY

  DELETE (pkgDeps)
  DELETE (fileDeps);
  DELETE (pkgCache);
  DELETE (fileCache);
  DELETE (cache);
  DELETE (pkgGraphPath);
  DELETE (fileGraphPath);
  DELETE (pkgCachePath);
  DELETE (fileCachePath);
  DELETE (cachePath);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
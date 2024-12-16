#include <diagnostic.h>
#include <directory.h>
#include <exception.h>
#include <cachefile.h>
#include <str.h>
#include <path.h>
#include <args.h>

typedef struct {
  int source;
} Env;

void option_source(Args *args, ArgValue value)
{
  ((Env*)args->env)->source = value.as_integer;
}

OPTIONS(
  { "source", 's', "Finds dependencies in sources instead of headers", ARG_TYPE_BOOLEAN, option_source }
);

void build_cache(CacheFile *file, const char *directory, int homelen, Env *env)
{
  for (DirectoryIterator *di = dopen(directory); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY) {
      if (di->current.name[0] != '.') {
        char directory[2048];

        dfullname(di, sizeof(directory), directory);
        build_cache(file, directory, homelen, env);
      }
    } else {
      char ext[8];

      fileext(di->current.name, sizeof(ext), ext);

      if ((env->source && (!strcmp(ext, ".c") || !strcmp(ext, ".cpp")))
      || (!env->source && (!strcmp(ext, ".h") || !strcmp(ext, ".hpp")))) {
        char package[128];
        char filename[2048];

        dfullname(di, sizeof(filename), filename);
        sprintf(package, "%s", di->path + homelen);

        for (int i = 0; package[i]; i++) {
          if (package[i] == PATH_MARKER) {
            package[i] = 0;
            break;
          }
        }

        ObjectArray_Push(&file->base, CacheRecord_FromValues(di->current.name, package, statfile(filename)));
      }
    }
  }
}

int main(int argc, char *argv[])
{
  Env   env  = { 0 };
  Args *args = NEW (Args) (argc, argv, &env);

  CHECK_MEMORY

  String *home;
  String *cachefile;

  const char *homevar = getenv("CUT_HOME");

  if (!homevar || !homevar[0]) {
    THROW(NEW (Exception)("No CUT_HOME environment variable defined... exiting!"));
  } else {
    home      = NEW (String)(homevar);
    cachefile = Path_Combine(homevar, "CUT/.cut/.cache");
  }

  CacheFile *cache = NEW (CacheFile)(cachefile->base, ACCESS_WRITE);

  build_cache(cache, home->base, home->length, &env);

  DELETE (cache);
  DELETE (cachefile);
  DELETE (home);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
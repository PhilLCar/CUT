#include <diagnostic.h>
#include <directory.h>
#include <exception.h>
#include <cachefile.h>
#include <graph.h>
#include <str.h>
#include <path.h>
#include <args.h>
#include <rootfile.h>

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

String* find(const char *filename, Env *env, String **package)
{
  String *file = NULL;

  for (DirectoryIterator *di = dopen(env->home); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY) {
      char      pkgpath[PATH_MAX];
      String   *rootpath;
      Array    *roots;

      dfullname(di, sizeof(pkgpath), pkgpath);

      rootpath = Path_Combine(pkgpath, ".cut/roots");
      roots    = (Array*)NEW (RootFile) (roots->base, ACCESS_READ);

      for (int i = 0; i < roots->size; i++) {
        String *root = Array_At(roots, i);
        String *inc  = String_Cat(Path_Combine(pkgpath, root->base), "/inc");

        for (DirectoryIterator *dj = dopen(inc->base); dj; dnext(&dj)) {
          if (dj ->current.type == DIRTYPE_FILE && !strcmp(dj->current.name, filename)) {
            char filepath[PATH_MAX];

            dfullname(dj, sizeof(filepath), filepath);

            *package = NEW (String) (pkgpath);
            file     = NEW (String) (filepath);
          }
        }

        DELETE (inc);

        if (file) {
          break;
        }
      }

      DELETE (roots);
      DELETE (rootpath);

      if (file) {
        dclose(&di);
        break;
      }
    }
  }

  return file;
}

void get_includes(Graph *includes, List *travelled, const char *filename, Env *env)
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

          if (String_StartsWith(line, env->home)) {
            //if (Graph_Key())
          }
        }
      }
    }

    DELETE (line);
  }

  DELETE (stream);
}

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
  Env   env  = { "", 0 };
  Args *args = NEW (Args) (argc, argv, &env);

  CHECK_MEMORY

  String *home;
  String *cachefile;

  env.home = getenv("CUT_HOME");

  if (!env.home || !env.home[0]) {
    THROW(NEW (Exception)("No CUT_HOME environment variable defined... exiting!"));
  } else {
    home      = NEW (String)(env.home);
    cachefile = Path_Combine(env.home, "CUT/.cut/.cache");
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
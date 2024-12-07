#include <diagnostic.h>
#include <directory.h>
#include <exception.h>
#include <cachefile.h>
#include <str.h>
#include <string.file.h>
#include <args.h>

OPTIONS();

void build_cache(CacheFile *file, const char *directory, int homelen)
{
  for (DirectoryIterator *di = dopen(directory); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY) {
      if (di->current.name[0] != '.') {
        char directory[2048];

        dfullname(di, directory, sizeof(directory));
        build_cache(file, directory, homelen);
      }
    } else {
      char ext[8];

      fileext(di->current.name, ext, sizeof(ext));

      if (!strcmp(ext, ".h") || !strcmp(ext, ".hpp")) {
        char package[128];
        char filename[2048];

        dfullname(di, filename, sizeof(filename));
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
  Args *args = NEW (Args) (argc, argv, NULL);

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

  CacheFile *cache = NEW (CacheFile)(cachefile->base, FILEACCESS_WRITE);

  build_cache(cache, home->base, home->length);

  DELETE (cache);
  DELETE (cachefile);
  DELETE (home);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
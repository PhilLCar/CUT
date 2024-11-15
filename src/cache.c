#include <directory.h>
#include <exception.h>
#include <diagnostic.h>
#include <cachefile.h>

const char *root;
int         rootlen;

long statfile(const char *filename)
{
  char  buffer[2048];
  FILE *result;

  sprintf(buffer, "stat -c %%Y %s", filename);

  result = popen(buffer, "r");

  memset(buffer, 0, sizeof(buffer));

  for (int c = fgetc(result), i = 0; c != EOF; c = fgetc(result), i++) buffer[i] = c;

  pclose(result);

  return atol(buffer);
}

void build_cache(CacheFile *file, const char *utilities)
{
  for (DirectoryIterator *di = dopen(utilities); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY) {
      if (di->current.name[0] != '.') {
        char directory[2048];

        dfullname(di, directory, sizeof(directory));
        build_cache(file, directory);
      }
    } else {
      char ext[8];

      fileext(di->current.name, ext, sizeof(ext));

      if (!strcmp(ext, ".h") || !strcmp(ext, ".hpp")) {
        char package[128];
        char filename[2048];

        dfullname(di, filename, sizeof(filename));
        sprintf(package, "%s", di->path + rootlen);

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
  char cachefile[2048];

  root = getenv("CUT_HOME");

  if (!root[0]) {
    THROW(NEW (Exception)("No CUT_HOME environment variable defined... exiting!"));
  } else {
    rootlen = strlen(root);

    sprintf(cachefile, "%s%s", root, "CUT/.cut/.cache");
  }

  CacheFile *cache = NEW (CacheFile)(cachefile, FILEACCESS_WRITE);

  build_cache(cache, root);

  DELETE (cache);
}
#include <directory.h>
#include <exception.h>
#include <diagnostic.h>

const char *root;
int         rootlen;

void build_cache(FILE *stream, const char *utilities)
{
  for (DirectoryIterator *di = dopen(utilities); di; dnext(&di)) {
    if (di->current.type == DIRTYPE_DIRECTORY) {
      if (di->current.name[0] != '.') {
        char directory[2048];

        dfullname(di, directory, sizeof(directory));
        build_cache(stream, directory);
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

        fprintf(stream, "%-32s %-31s %-15ld\n", di->current.name, package, statfile(filename));
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

  FILE *cache = fopen(cachefile, "w+");

  build_cache(cache, root);

  fclose(cache);
}
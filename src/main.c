// C
#include <stdio.h>

// CUT
#include <args.h>
#include <directory.h>
#include <filestream.h>

// other
#include "todo.c"

#define MAX_ROOTS 8

typedef struct _global {
  const char *directory;
  int         library;
  const char *roots[MAX_ROOTS];
  const char *args;
} Global;

void option_dir(Args* args, ArgValue value)
{
  Global *env = args->env;
  env->directory = value.as_charptr ? value.as_charptr : ".";
}

void option_new(Args* args, ArgValue value)
{
  option_dir(args, value);
}

void option_lib(Args* args, ArgValue value)
{
  Global *env  = args->env;
  env->library = value.as_integer;
}

void option_roots(Args* args, ArgValue value)
{
  Global *env = args->env;
  
  int   list_size = 0;
  char *list      = (char*)value.as_charptr;

  if (list) {
    env->roots[list_size++] = list;

    for (int i = 0; list[i]; i++) {
      if (list[i] == ',') {
        list[i] = 0;
        env->roots[list_size++] = &list[i + 1];
      }
    }
  }
}

void option_build(Args* args, ArgValue value)
{
  option_dir(args, value);
}

void option_depends(Args* args, ArgValue value)
{
  option_dir(args, value);
}

void option_remove(Args* args, ArgValue value)
{
  option_dir(args, value);
}

void option_args(Args* args, ArgValue value)
{
  Global *env = args->env;
  env->args = value.as_charptr;
}

void option_todo(Args* args, ArgValue value)
{
  exit(0);
}

void option_test(Args* args, ArgValue value)
{
  exit(0);
}

OPTIONS(
  { "dir",      ' ', "Sets the working directory, default to current directory", ARG_TYPE_CHARPTR, option_dir     },
  { "init",     ' ', "Initializes a new CUT directory (program by default)",     ARG_TYPE_CHARPTR, option_new     },
  { "lib",      ' ', "Sets the CUT directory as a library",                      ARG_TYPE_BOOLEAN, option_lib     },
  { "roots",    ' ', "Comma separated list of project roots",                    ARG_TYPE_CHARPTR, option_roots   },
  { "build",    ' ', "Builds the CUT directory, defaults to current directory",  ARG_TYPE_CHARPTR, option_build   },
  { "depends",  ' ', "Rebuilds the dependency graph",                            ARG_TYPE_CHARPTR, option_depends },
  { "remove",   ' ', "Removes CUT tracking for this directory",                  ARG_TYPE_CHARPTR, option_remove  },
  { "",         'a', "Adds the arguments to the underlying process (if any)",    ARG_TYPE_CHARPTR, option_args    },
  { "todo",     ' ', "Runs the \"todo\" subprocess on the specified directory",  ARG_TYPE_CHARPTR, option_todo    },
  { "test",     ' ', "Runs the \"test\" subprocess on the specified directory",  ARG_TYPE_CHARPTR, option_test    },
  { "command",  '+', "The cut command to execute",                               ARG_TYPE_CHARPTR, NULL           }
);

int main(int argc, char *argv[])
{
  // Global env = {
  //   .directory = ".",
  //   .library   = 0,
  //   .roots     = { NULL }
  // };

  // Args *args = NEW (Args) (argc, argv, &env);

  // TODO: (high): Refactor: Remove
  CHECK_MEMORY
  char *nargv[2] = { "test", "." };
  todo(2, nargv);
  STOP_WATCHING
  // TODO: (low): Test: Remove this shiiit

  // printf("Current folder: %s\n", env.directory);
  // printf("Library?: %s\n", env.library ? "true" : "false");

  // for (int i = 0; env.roots[i] && i < MAX_ROOTS; i++)
  // {
  //   printf("Item %d: %s\n", i, env.roots[i]);
  // }

  // for (DirectoryIterator *di = dopen(env.directory); di; dnext(&di))
  // {
  //   printf("%s\n", di->current.name);
  // }


  // DELETE (args);
  return 0;
}

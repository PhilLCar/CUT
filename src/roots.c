#include <diagnostic.h>
#include <rootfile.h>
#include <args.h>
#include <filestream.h>
#include <collection.str.h>
#include <set.h>
#include <print.h>

typedef struct {
  const char *add;
  const char *rem;
  int         clear;
  int         list;
} Env;

void option_add(Args *args, ArgValue value)
{
  ((Env*)args->env)->add = value.as_charptr;
}

void option_rem(Args *args, ArgValue value)
{
  ((Env*)args->env)->rem = value.as_charptr;
}

void option_clear(Args *args, ArgValue value)
{
  ((Env*)args->env)->clear = 1;
}

void option_list(Args *args, ArgValue value)
{
  ((Env*)args->env)->list = 1;
}

OPTIONS(
  { "add",    'a', "Add the specified roots (comma separated)",    ARG_TYPE_CHARPTR, option_add   },
  { "remove", 'r', "Remove the specified roots (comma separated)", ARG_TYPE_CHARPTR, option_rem   },
  { "clear",  'c', "Clears the all the current roots",             ARG_TYPE_BOOLEAN, option_clear },
  { "list",   'l', "List the current roots",                       ARG_TYPE_BOOLEAN, option_list  },
  { "path",   '-', "Specify the path to the CUT project",          ARG_TYPE_CHARPTR, NULL         }
);

int main(int argc, char *argv[argc])
{
  CHECK_MEMORY
  Env     env   = { NULL, NULL, 0, 0 };
  Args   *args  = NEW (Args) (argc, argv, &env);
  String *path  = String_Cat(NEW (String) (IFNULL(Args_Name(args, "path").as_charptr, ".")), "/.cut/roots");
  Set    *roots = (Set*) NEW (RootFile) (path->base, ACCESS_READ | ACCESS_WRITE);

  CHECK_MEMORY

  if (env.clear) {
    for (int i = 0; i < ((Array*)roots)->size; i++) {
      if (String_Cmp(Array_At((Array*)roots, i), ".")) {
        ObjectArray_RemoveAt((ObjectArray*)roots, i--, 0);
      }
    }
  }

  if (env.add) {
    ObjectArray *tba = String_Split(NEW (String) (env.add), ",");

    for (int i = 0; i < ((Array*)tba)->size; i++) {
      String *root = String_Copy(ObjectArray_At(tba, i));

      Set_Add(roots, root);
    }

    DELETE (tba)
  }

  if (env.rem) {
    ObjectArray *tbr = String_Split(NEW (String) (env.rem), ",");

    for (int i = 0; i < ((Array*)tbr)->size; i++) {
      String *root = ObjectArray_At(tbr, i);

      Set_Remove(roots, root);
    }


    DELETE (tbr)
  }

  if (env.list) {
    print("Roots: %O\n", roots);
  }


  DELETE (roots);
  DELETE (path);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}

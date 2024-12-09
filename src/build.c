#include <diagnostic.h>
#include <args.h>
#include <objectarray.h>
#include <directory.h>
#include <string.file.h>

OPTIONS(
  { "lang",   ' ', "Which language/compiler to use for building (default C/gcc)", ARG_TYPE_CHARPTR, NULL },
  { "debug",  'd', "Should the program be compiled with debug symbols",           ARG_TYPE_BOOLEAN, NULL },
  { "memory", 'm', "Use diagnostic memory functions (much slower)",               ARG_TYPE_BOOLEAN, NULL },
  { "path",   '-', "The root path of the cut project to build",                   ARG_TYPE_CHARPTR, NULL }
);

void get_files(const char *folder, const char *extension, const char *filter, ObjectArray *files)
{
  for (DirectoryIterator *di = dopen(folder); di; dnext(&di)) {
    char fullname[2048];
    char ext[8];

    dfullname(di, fullname, sizeof(fullname));
    fileext(di->current.name, ext, sizeof(ext));

    if (filter && di->current.type == DIRTYPE_DIRECTORY) {
      if (!strcmp(filter, di->current.name)) {
        get_files(fullname, extension, NULL, files);
      } else {
        get_files(fullname, extension, filter, files);
      }
    } else if (!filter && di->current.type == DIRTYPE_FILE && !strcmp(extension, ext)) {
      ObjectArray_Add(files, NEW (String) (fullname));
    }
  }
}

int header_comparer(String *against, String *reference)
{
  String *header = Path_File(against->base);
  int     result = String_Compare(header, reference);

  DELETE (header);

  return result;
}


int main(int argc, char *argv[])
{
  CHECK_MEMORY

  Args *args = NEW (Args) (argc, argv, NULL);

  // String *command = NEW (String) ("gcc");

  // ObjectArray *libraries = NEW (ObjectArray) (TYPEOF (String));

  // TODO: Save additional libraries

  ObjectArray *headers = NEW (ObjectArray) (TYPEOF (String));
  ObjectArray *sources = NEW (ObjectArray) (TYPEOF (String));

  // TODO: Check if program
  int program = 0;

  int c = !strcmp(IFNULL(Args_Name(args, "lang").as_charptr, "c"), "c");
  
  //int debug  = Args_Name(args, "debug").as_integer;
  //int memory = Args_Name(args, "memory").as_integer;

  const char *path = IFNULL(Args_Name(args, "path").as_charptr, ".");

  const char *hdrext = c ? ".h" : ".hpp";
  const char *srcext = c ? ".c" : ".cpp";

  get_files(path, hdrext, "inc", headers);
  get_files(path, srcext, "src", sources);



  for (int i = 0; i < sources->base.size && !program; i++) {
    String *path   = Array_At((Array*)sources, i);
    String *header = String_Concat(Path_FileName(path->base), NEW (String) (hdrext));

    void *in = ObjectArray_In(headers, header, (Comparer)header_comparer);

    printf("%s : %p\n", path->base, in);

    DELETE (header);
  }

  if (program)
  {
    // bin/depends -su
  } else {
    // bin/depends -u
  }

  // Build library (if exists)


  // Build program (if exists)

  DELETE (sources);
  DELETE (headers);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
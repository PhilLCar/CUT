#include <diagnostic.h>
#include <args.h>
#include <objectarray.h>
#include <directory.h>
#include <path.h>
#include <charstream.h>
#include <processstream.h>
#include <mapfile.h>
#include <print.h>

OPTIONS(
  { "lang",   ' ', "Which language/compiler to use for building (default C/gcc)", ARG_TYPE_CHARPTR, NULL },
  { "nowarn", 'w', "Disable 'all warnings' switch on compiler (not recommended)", ARG_TYPE_BOOLEAN, NULL },
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

String *get_lib(String *folder)
{
  String *library = NULL;

  for (DirectoryIterator *di = dopen(folder->base); di; dnext(&di))
  {
    if (di->current.type == DIRTYPE_FILE) {
      library = NEW (String) (di->current.name);

      dclose(&di);
      break;
    }
  }

  return library;
}

Map *get_libs(Array *packages)
{
  Map *libraries = NEW (Map) (TYPEOF (String), TYPEOF (String), (Comparer)String_Compare);

  for (int i = 0; i < packages->size; i++)
  {
    String *package = Array_At(packages, i);
    String *path    = Path_Combine(package->base, "lib");
    String *library = get_lib(path);

    Map_Set(libraries, path, library);
  }

  return libraries;
}

String *run(const char *command)
{
  return CharStream_GetToEnd((CharStream*)ProcessStream_Open(command, ACCESS_READ));
}

String *runs(String *command)
{
  String *result = run(command->base);

  DELETE(command);

  return result;
}

void systems(String *command)
{
  String *result = runs(command);

  DELETE (result);
}

String *get_date()
{
  return run("date +\"%Y-%m-%d %H:%M:%S\"");
}

int header_comparer(String *against, String *reference)
{
  String *header = Path_File(against->base);
  int     result = String_Compare(header, reference);

  DELETE (header);

  return result;
}

String *object(String *init, ObjectArray *includes, Array *libraries, ObjectArray *inputs, String *output)
{
  for (int i = 0; i < includes->base.size; i++) {
    String_Concat(init, String_Format(" -I%O", ObjectArray_At(includes, i)));
  }

  for (int i = 0; i < libraries->size; i++) {
    Pair   *pair    = Array_At(libraries, i);

    String_Concat(init, String_Format(" -L%O", pair->first.object));
    String_Concat(init, String_Format(" -l:%O", pair->second.object));
  }

  for (int i = 0; i < inputs->base.size; i++)
  {
    String_Concat(init, String_Format(" %O", ObjectArray_At(inputs, i)));
  }

  String_Concat(init, String_Format(" -o %O", output));

  return init;
}


int main(int argc, char *argv[])
{
  CHECK_MEMORY

  Args       *args = NEW (Args) (argc, argv, NULL);
  const char *path = IFNULL(Args_Name(args, "path").as_charptr, ".");
  const char *home = getenv("CUT_HOME");
  String     *name;

  if (home) { // Make sure this is an initialized cut directory
    String            *dirpath = Path_Combine(path, ".cut");
    DirectoryIterator *dir     = dopen(dirpath->base);
    char               buffer[PATH_MAX];

    if (!dir) {
      THROW (NEW (Exception) ("This is not an initialized CUT directory! Aborting..."));
    } 

    realpath(path, buffer);

    //systems(String_Cat(NEW (String) ("cut depends -su "), buffer));

    name = String_ToLower(Path_FileName(buffer));

    dclose(&dir);
    DELETE (dirpath);
  } else {
    THROW (NEW (Exception) ("The 'CUT_HOME' environment variable is not set! Aborting..."));
  }

  String *deppath = Path_Combine(path, ".cut/depends.map");
  Map    *depends = (Map*)NEW (MapFile) (deppath->base, ACCESS_READ);

  int c = !strcmp(IFNULL(Args_Name(args, "lang").as_charptr, "c"), "c");
  
  int nowarn = Args_Name(args, "nowarn").as_integer;
  int debug  = Args_Name(args, "debug").as_integer;
  int memory = Args_Name(args, "memory").as_integer;

  const char *hdrext   = c ? ".h"  : ".hpp";
  const char *srcext   = c ? ".c"  : ".cpp";
  const char *compiler = c ? "gcc" : "g++";

  Map    *libraries = get_libs(Map_ValueAt(depends, "packages"));
  String *command   = NEW (String) (compiler);

  ObjectArray *headers = NEW (ObjectArray) (TYPEOF (String));
  ObjectArray *sources = NEW (ObjectArray) (TYPEOF (String));


  get_files(path, hdrext, "inc", headers);
  get_files(path, srcext, "src", sources);

  // All wa
  if (!nowarn) {
    String_Cat(command, " -Wall");
  }

  if (debug) {
    String_Cat(command, " -g");
  }
  
  if (memory) {
    String_Cat(command, " -DMEMORY_WATCH");
  }

  // TODO: delete when versionning is implemented
  String_Cat(command, " -DVERSION_MAJOR=0");
  String_Cat(command, " -DVERSION_MINOR=0");
  String_Cat(command, " -DVERSION_REVISION=0");
  String_Cat(command, " -DVERSION_BUILD=0");
  String_Concat(command, String_Format(" -DBUILD_DATE='%Of'", get_date()));

  for (int i = 0; i < sources->base.size; i++) {
    String *path   = Array_At((Array*)sources, i);
    //String *header = String_Concat(Path_FileName(path->base), NEW (String) (hdrext));

    //void *in = ObjectArray_In(headers, header, (Comparer)header_comparer);

    String *n = Path_File(path->base);

    ObjectArray *tmp = NEW (ObjectArray)(TYPEOF(String));

    ObjectArray_Fill(tmp, NEW (String) ("this"), NEW (String)("test"), NULL);

    print("%Of\n", object(String_Copy(command), Map_ValueAt(depends, n->base), (Array*)libraries, tmp, n));

    DELETE (tmp);
    DELETE (n);

    //DELETE (header);
  }

  DELETE (libraries);
  DELETE (command);
  DELETE (name);
  DELETE (depends);
  DELETE (deppath);
  DELETE (sources);
  DELETE (headers);
  DELETE (args);

  CHECK_MEMORY
  STOP_WATCHING
}
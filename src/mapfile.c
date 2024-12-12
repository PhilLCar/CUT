#include <mapfile.h>
#include <set.h>

#define TYPENAME MapFile

////////////////////////////////////////////////////////////////////////////////
MapFile *_(Construct)(const char *filename, AccessModes mode)
{  
  if (Map_Construct(BASE(0), TYPEOF (String), TYPEOF (Set))) {
    if (filename) {
      this->filename = malloc(strlen(filename) + 1);
      this->mode     = mode;

      strcpy((void*)this->filename, filename);
    }
    
    if (mode & ACCESS_READ) {
      CharStream *stream = (CharStream*) NEW (FileStream) (fopen(filename, "r"));

      if (stream) {
        String      *line    = NULL;
        ObjectArray *current = NULL;

        while ((line = CharStream_GetLine(stream))) {
          if (!line->length) {
            DELETE (line)
          } else if (line->base[0] == ' ') {
            ObjectArray_Push(current, String_Trim(line));
          } else {
            // Remove the ':'
            String_SubString(line, 0, -1);

            current = Map_Set(BASE(0), line, NEW (Set)(TYPEOF (String)))->second.object;
          }
        }

        DELETE (stream);
      } else {
        THROW(NEW (Exception)("File not found!"));
      }
    }
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
void _(Destruct)()
{
  if (this) {
    if (this->mode & ACCESS_WRITE) {
      CharStream *stream = (CharStream*) NEW (FileStream) (fopen(this->filename, "w+"));

      if (stream) {
        for (int i = 0; i < BASE(2)->size; i++) {
          Pair   *pair = Array_At(BASE(2), i);
          String *name = pair->first.object;
          Array  *list = pair->second.object;

          CharStream_PutStr(stream, name->base);
          CharStream_PutLn(stream, ":");

          for (int j = 0; j < list->size; j++) {
            CharStream_PutStr(stream, "    ");
            CharStream_PutLn(stream, ((String*)Array_At(list, j))->base);
          }
        }

        DELETE (stream);
      } else {
        THROW(NEW (Exception)("Couldn't open file!"));
      }
    }

    if (this->filename) {
      free((void*)this->filename);
      this->filename = NULL;
    }
    
    Map_Destruct(BASE(0));
  }
}

#undef TYPENAME

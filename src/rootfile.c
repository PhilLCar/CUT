#include <rootfile.h>

#define TYPENAME RootFile

////////////////////////////////////////////////////////////////////////////////
RootFile *_(Construct)(const char *filename, AccessModes mode)
{  
  if (Set_Construct(BASE(0), TYPE(String))) {
    if (filename) {
      this->filename = malloc(strlen(filename) + 1);
      this->mode     = mode;

      strcpy((void*)this->filename, filename);
    }

    if (mode & ACCESS_READ) {
      CharStream *stream = (CharStream*)FileStream_Open(filename, ACCESS_CREATE | ACCESS_READ);
      String     *line   = NULL;

      // "." is always a root
      Set_Add(BASE(0), NEW (String)("."));

      if (stream->base.base) {
        while ((line = CharStream_GetLine(stream))) {
          Set_Add(BASE(0), line);
        }
      }

      DELETE (stream);
    }
  }

  return this;
}

////////////////////////////////////////////////////////////////////////////////
void _(Destruct)()
{
  if (this) {
    if (this->mode & ACCESS_WRITE) {
      CharStream *stream = (CharStream*)FileStream_Open(this->filename, ACCESS_CREATE | ACCESS_WRITE);

      for (int i = 0; i < BASE(2)->size; i++) {
        String *root = ObjectArray_At(BASE(1), i);

        CharStream_WriteLine(stream, root);
      }

      DELETE (stream); 
    }

    if (this->filename) {
      free((void*)this->filename);
      this->filename = NULL;
    }
    
    Set_Destruct(BASE(0));
  }
}

#undef TYPENAME

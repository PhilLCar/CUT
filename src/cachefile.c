#include <cachefile.h>

#define TYPENAME CacheFile

CacheFile *_(Construct)(const char *filename, FileAccessModes mode)
{
  if (ObjectArray_Construct(BASE(0), TYPEOF (CacheRecord))) {
    if (filename) {
      this->filename = malloc(strlen(filename) + 1);
      this->mode     = mode;

      strcpy((void*)this->filename, filename);
    }

    if (mode & FILEACCESS_READ) {
      CharStream  *stream = (CharStream*)NEW (FileStream)(fopen(filename, "r"));

      if (stream) {
        while (!stream->base.eos) {
          String *line = CharStream_GetLine(stream);

          if (line && line->length) {
            ObjectArray_Push(BASE(0), NEW (CacheRecord)(line));
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

void _(Destruct)()
{
  if (this) {
    if (this->mode & FILEACCESS_WRITE) {
      CharStream *stream = (CharStream*) NEW (FileStream) (fopen(this->filename, "w+"));

      if (stream) {
        for (int i = 0; i < BASE(1)->size; i++) {
          CacheRecord *record = Array_At(BASE(1), i);
          String      *line   = CacheRecord_ToString(record);

          CharStream_PutString(stream, line->base);

          DELETE (line);
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
    
    ObjectArray_Destruct(BASE(0));
  }
}

#undef TYPENAME

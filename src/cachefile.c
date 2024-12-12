#include <cachefile.h>

#define TYPENAME CacheFile

CacheFile *_(Construct)(const char *filename, AccessModes mode)
{
  if (ObjectArray_Construct(BASE(0), TYPEOF (CacheRecord))) {
    if (filename) {
      this->filename = malloc(strlen(filename) + 1);
      this->mode     = mode;

      strcpy((void*)this->filename, filename);
    }

    if (mode & ACCESS_READ) {
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
    if (this->mode & ACCESS_WRITE) {
      CharStream *stream = (CharStream*) NEW (FileStream) (fopen(this->filename, "w+"));

      if (stream) {
        for (int i = 0; i < BASE(1)->size; i++) {
          CacheRecord *record = ObjectArray_At(BASE(0), i);

          CharStream_PutLine(stream, record);
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

#undef TYPENAME

#include <cachefile.h>

#define TYPENAME CacheFile

void _(FromLine)(String *line)
{
  ObjectArray *record = String_Split(line, " ");

  String *key, *value, *timestamp;

  for (int i = 0; i < record->base.size; i++) {
    if (String_Eq(Array_At((Array*)record, i), "")) {
      ObjectArray_RemoveAt(record, i--, 0);
    }
  }

  if (record->base.size != 3) {
    THROW(NEW (Exception)("Bad format!"));
  }

  key       = ObjectArray_Remove(record, 1);
  value     = ObjectArray_Remove(record, 1);
  timestamp = ObjectArray_Remove(record, 1);

  CacheFile_Set(this, key, value, atol(timestamp->base));

  DELETE (timestamp);
  DELETE (record);
}

CacheFile *_(Construct)(const char *filename, AccessModes mode)
{
  if (MapSet_Construct(BASE(0), TYPEOF (String))) {
    if (filename) {
      this->filename = malloc(strlen(filename) + 1);
      this->mode     = mode;

      strcpy((void*)this->filename, filename);
    }

    if (mode & ACCESS_READ) {
      CharStream *stream = (CharStream*)NEW (FileStream)(fopen(filename, "r"));

      if (stream) {
        while (!stream->base.eos) {
          String *line = CharStream_GetLine(stream);

          if (line) CacheFile_FromLine(this, line);
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
      CharStream *stream = (CharStream*) FileStream_Open(this->filename, ACCESS_WRITE | ACCESS_CREATE);

      if (stream) {
        for (int i = 0; i < BASE(3)->size; i++) {
          KeyVal      *keyval = ObjectArray_At(BASE(2), i);
          String      *key    = keyval->base.first;
          CacheRecord *record = keyval->base.second;
          
          String *line = String_Format("%-32O %-63O %-15ld", key, record->value, record->timestamp);

          CharStream_WriteLine(stream, line);

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
    
    MapSet_Destruct(BASE(0));
  }
}

CacheRecord *_(Set)(String *key, String *value, long timestamp)
{
  return MapSet_Set(BASE(0), key, NEW (CacheRecord) (value, timestamp))->base.second;
}

CacheRecord *_(Get)(const String *key)
{
  return CacheFile_GetKey(this, key->base);
}

CacheRecord *_(GetKey)(const char *key)
{
  return MapSet_ValueAtKey(BASE(0), key);
}

#undef TYPENAME

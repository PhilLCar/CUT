#include <cacherecord.h>

#define TYPENAME CacheRecord

CacheRecord *_(Construct)(String *line)
{
  if (this) {
    if (line) {
      ObjectArray *record = String_Split(line, " ");
      String      *lastmod;

      for (int i = 0; i < record->base.size; i++) {
        if (String_Eq(Array_At((Array*)record, i), "")) {
          ObjectArray_RemoveAt(record, i--, 0);
        }
      }

      if (record->base.size != 3) {
        THROW(NEW (Exception)("Bad format!"));
      }

      this->file    = ObjectArray_Remove(record, 1);
      this->package = ObjectArray_Remove(record, 1);
      lastmod       = ObjectArray_Remove(record, 1);
      this->lastmod = atol(lastmod->base);

      DELETE (lastmod);
      DELETE (record);
    } else {
      this->file    = NULL;
      this->package = NULL;
      this->lastmod = 0;
    }
  }

  return this;
}

void _(Destruct)()
{
  if (this) {
    DELETE (this->file);
    DELETE (this->package);
  }
}

CacheRecord *STATIC (FromValues)(const char *filename, const char *packagename, long lastmod)
{
  CacheRecord *record = NEW (CacheRecord)(NULL);

  record->file    = NEW (String)(filename);
  record->package = NEW (String)(packagename);
  record->lastmod = lastmod;

  return record;
}

String *_(ToString)()
{
  char buffer[2048];

  sprintf(buffer, "%-32s %-31s %-15ld\n", this->file->base, this->package->base, this->lastmod);

  return NEW (String)(buffer);
}



#undef TYPENAME
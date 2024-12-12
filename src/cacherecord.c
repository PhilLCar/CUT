#include <cacherecord.h>

#define TYPENAME CacheRecord

CacheRecord *_(Construct)(String *line)
{
  if (this) {
    if (line) {
      ObjectArray *record = String_Split(line, " ");
      String      *timestamp;

      for (int i = 0; i < record->base.size; i++) {
        if (String_Eq(Array_At((Array*)record, i), "")) {
          ObjectArray_RemoveAt(record, i--, 0);
        }
      }

      if (record->base.size != 3) {
        THROW(NEW (Exception)("Bad format!"));
      }

      this->key       = ObjectArray_Remove(record, 1);
      this->value     = ObjectArray_Remove(record, 1);
      timestamp       = ObjectArray_Remove(record, 1);
      this->timestamp = atol(timestamp->base);

      DELETE (timestamp);
      DELETE (record);
    } else {
      this->key       = NULL;
      this->value     = NULL;
      this->timestamp = 0;
    }
  }

  return this;
}

void _(Destruct)()
{
  if (this) {
    DELETE (this->key);
    DELETE (this->value);
  }
}

CacheRecord *STATIC (FromValues)(const char *filename, const char *packagename, long timestamp)
{
  CacheRecord *record = NEW (CacheRecord)(NULL);

  record->key       = NEW (String)(filename);
  record->value     = NEW (String)(packagename);
  record->timestamp = timestamp;

  return record;
}

String *_(ToString)()
{
  return String_Format("%-32O %-31O %-15ld", this->key, this->value, this->timestamp);
}

int _(Comparer)(CacheRecord *other)
{
  return String_Compare(this->key, other->key);
}

int _(KeyComparer)(String *key)
{
  return String_Compare(this->key, key);
}



#undef TYPENAME
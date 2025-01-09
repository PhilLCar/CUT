#include <cacherecord.h>

#define TYPENAME CacheRecord

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

CacheRecord *_(Construct)(String *value, long timestamp)
{
  if (this) {
    this->value     = value;
    this->timestamp = timestamp;
  }

  return this;
}

void _(Destruct)()
{
  DELETE (this->value);
}

#undef TYPENAME
#ifndef CACHERECORD_H
#define CACHERECORD_H

// CUT
#include <diagnostic.h>
#include <oop.h>
#include <str.h>
#include <map.h>
#include <collection.str.h>

#define TYPENAME CacheRecord

OBJECT (String *line) NOBASE
  String *key;
  String *value;
  long    timestamp;
END_OBJECT(NULL);

CacheRecord *STATIC (FromValues)(const char *filename, const char *packagename, long lastmod);

String *_(ToString)()                   VIRTUAL (ToString);
int     _(Comparer)(CacheRecord *other) VIRTUAL (Comparer);
int     _(KeyComparer)(String *key)     VIRTUAL (KeyComparer);

#undef TYPENAME

#endif
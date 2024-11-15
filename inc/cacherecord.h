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
  String *file;
  String *package;
  long    lastmod;
END_OBJECT(NULL);

CacheRecord *STATIC (FromValues)(const char *filename, const char *packagename, long lastmod);

String      *_(ToString)();

#undef TYPENAME

#endif
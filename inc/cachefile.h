#ifndef CACHEFILE_H
#define CACHEFILE_H

// CUT
#include <diagnostic.h>
#include <cacherecord.h>
#include <filestream.h>
#include <charstream.h>

#define TYPENAME CacheFile

OBJECT (const char *filename, AccessModes mode) INHERIT (ObjectArray)
  const char  *filename;
  AccessModes  mode;
END_OBJECT(".cache", ACCESS_READ);

long statfile(const char *filename);

#undef TYPENAME

#endif
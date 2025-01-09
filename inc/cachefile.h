#ifndef CACHEFILE_H
#define CACHEFILE_H

// CUT
#include <diagnostic.h>
#include <cacherecord.h>
#include <filestream.h>
#include <charstream.h>
#include <mapset.h>
#include <collection.str.h>

#define TYPENAME CacheFile

OBJECT (const char *filename, AccessModes mode) INHERIT (MapSet)
  const char  *filename;
  AccessModes  mode;
END_OBJECT(".cache", ACCESS_READ);

#undef TYPENAME

#endif
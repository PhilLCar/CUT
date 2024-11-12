#ifndef CACHEFILE_H
#define CACHEFILE_H

// CUT
#include <diagnostic.h>
#include <cacherecord.h>
#include <filestream.h>

#define TYPENAME CacheFile

OBJECT (const char *filename, FileAccessModes mode) INHERIT (ObjectArray)
  const char      *filename;
  FileAccessModes  mode;
END_OBJECT(".cache", FILEACCESS_READ);


#undef TYPENAME

#endif
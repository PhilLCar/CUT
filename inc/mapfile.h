#ifndef MAPFILE_H
#define MAPFILE_H

// CUT
#include <oop.h>
#include <map.h>
#include <str.h>
#include <filestream.h>
#include <string.charstream.h>

#define TYPENAME MapFile

OBJECT (const char *filename, FileAccessModes mode) INHERIT(Map)
  const char      *filename;
  FileAccessModes  mode;
END_OBJECT("", 1);

#undef TYPENAME
#endif
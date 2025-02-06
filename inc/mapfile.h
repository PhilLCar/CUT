#ifndef MAPFILE_H
#define MAPFILE_H

// CUT
#include <diagnostic.h>
#include <oop.h>
#include <map.h>
#include <str.h>
#include <filestream.h>
#include <charstream.h>
#include <set.h>

#define TYPENAME MapFile

OBJECT (const char *filename, AccessModes mode) INHERIT (Map)
  const char  *filename;
  AccessModes  mode;
END_OBJECT("", ACCESS_READ);

#undef TYPENAME
#endif
# The following variables need to be set prior to including library.mk

NAME = cut

# Versioning
include ver/version.mk

# Bootstrap
include res/bootstrap.mk

# If the project is separated among multiple sub-folders
PROJECT_ROOTS =

# Additionnal libraries (ex: -pthread, -lmath, etc)
ADD_LIBRARIES = 

# Additionnal flags for the compiler
ADD_CFLAGS = 

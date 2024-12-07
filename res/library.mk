# This is a template for every C/C++ library that conforms to [this] standard
# More information: github.com/PhilLCar (philippe-caron@hotmail.com)

# NOTE: that since all paths are included, files within a project should have
# different names, even if they are not in the same subfolder.

# (!) The user should define NAME, FLAGS, LIBRARIES and PROJECT_ROOTS
# according to their needs.

.DEFAULT_GOAL = library
INCLUDE_FROM  = headers

# Default to C
ifeq "$(LANG)" "C++"
	NAME   := $(NAME)$(MAJOR)++
else
	NAME   := $(NAME)$(MAJOR)
endif

#include common commands
include $(CUT_HOME)CUT/res/common.mk

# This is a template for every C/C++ library that conforms to [this] standard
# More information: github.com/PhilLCar (philippe-caron@hotmail.com)

# NOTE: that since all paths are included, files should have different names
# even if they are not in the same subfolder.

# (!) The user should define NAME, MAJOR, CFLAGS, LIBRARIES and DIRECTORIES
# according to their needs.

# General parameters
CC     = gcc
CCXX   = g++
CFLAGS = $(ADD_CFLAGS) -Wall \
-DVERSION_MAJOR=$(MAJOR) \
-DVERSION_MINOR=$(MINOR) \
-DVERSION_REVISION=$(REVISION) \
-DVERSION_BUILD=$(BUILD) \
-DBUILD_DATE='"$(shell date +"%Y-%m-%d %H:%M:%S")"'

# Default to C
ifeq "$(LANG)" "C++"
	CMP     = $(CCXX)
	EXT_SRC = .cpp
	EXT_HDR = .hpp
	NAME   := $(NAME)$(MAJOR)++
else
	CMP     = $(CC)
	EXT_SRC = .c
	EXT_HDR = .h
	NAME   := $(NAME)$(MAJOR)
endif

############################################################################################
# The goal of the following function is to flatten the "obj" folder
# This lookup function is taken from:
# https://stackoverflow.com/questions/9674711/makefile-find-a-position-of-word-in-a-variable
_pos = $(if $(findstring $1,$2),$(call _pos,$1,$(wordlist 2,$(words $2),$2),x $3),$3)
pos  = $(words $(call _pos,$1,$2))

lookup = $(word $(call pos,$1,$2),$3)
############################################################################################

# CUT Bootstrap
BOOTSTRAP = $(CUT_HOME)CUT/bin/bootstrap

# Compilation parameters
DIRECTORIES = $(PROJECT_ROOTS) .
INCLUDES    = $(patsubst %, -I%/inc, $(DIRECTORIES))
SOURCES     = $(foreach path, $(DIRECTORIES), $(wildcard $(path)/src/*$(EXT_SRC)))
OBJECTS     = $(patsubst %, obj/%.o, $(notdir $(SOURCES)))
LIBRARIES   = $(ADD_LIBRARIES)
#$(shell \
cd $(CUT_HOME)CUT && \
make cache >/dev/null && \
cd - >/dev/null&& \
$(CUT_HOME)CUT/bin/bootstrap --depends $(patsubst %, %/inc/, $(DIRECTORIES)))

# Add the debug flag if variable is defined
ifdef DEBUG
	CFLAGS += -g
endif

# Output directories might not exist if freshly pulled from git, or after clean
obj:
	mkdir obj

bin:
	mkdir bin

lib:
	mkdir lib

.cut:
	mkdir .cut

.cut/depends.list: .cut
	cd $(CUT_HOME)CUT && make cache
	$(BOOTSTRAP) --depends ./ > $@

#1 include script recursive
#2 dependency file

# Rules for language-specific object files
%$(EXT_SRC).o: $(call lookup, $@, $(OBJECTS), $(SOURCES)) obj
	$(CMP) $(CFLAGS) $(INCLUDES) $(shell $(BOOTSTRAP) --include $(notdir $<)) -c $< -o $@ $(ADD_LIBRARIES)

lib/lib$(NAME).a: .cut/depends.list $(OBJECTS) lib
	ar rcs $@ obj/*$(EXT_SRC).o

bin/$(NAME).test: lib/lib$(NAME).a bin
	$(CMP) $(CFLAGS) $(LIBRARIES) $(INCLUDES) -Llib -l$(NAME) tst/main$(EXT_SRC) -o bin/$(NAME).test

# Global rules
library: lib/lib$(NAME).a

test: bin/$(NAME).test
	./bin/$(NAME).test
	

clean:
	rm -rf obj
	rm -rf lib
	rm -rf bin
	rm -rf .cut

tmp:
	echo $(shell $(BOOTSTRAP) --include map.c)
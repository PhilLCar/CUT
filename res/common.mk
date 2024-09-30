# This is a template for every C/C++ object that conforms to [this] standard
# More information: github.com/PhilLCar (philippe-caron@hotmail.com)

# NOTE: that since all paths are included, files within a project should have
# different names, even if they are not in the same subfolder.

# (!) The user should define NAME, FLAGS, LIBRARIES and PROJECT_ROOTS
# according to their needs.

# General parameters
CC     = gcc
CCXX   = g++
CFLAGS = $(FLAGS) -Wall \
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
else
	CMP     = $(CC)
	EXT_SRC = .c
	EXT_HDR = .h
endif

# CUT Bootstrap
BOOTSTRAP = $(CUT_HOME)CUT/bin/bootstrap
DEPENDS   = depends.list

# Compilation parameters
DIRECTORIES = $(PROJECT_ROOTS) .
INCLUDES    = $(patsubst %, -I%/inc, $(DIRECTORIES))
SOURCES     = $(foreach path, $(DIRECTORIES), $(wildcard $(path)/src/*$(EXT_SRC)))
HEADERS     = $(foreach path, $(DIRECTORIES), $(wildcard $(path)/inc/*$(EXT_HDR)))
OBJECTS     = $(patsubst %, obj/%.o, $(notdir $(SOURCES)))

# Add the debug flag if variable is defined
ifdef DEBUG
	CFLAGS += -g
endif

# Find which source is associated with the object
FINDSRC = $(foreach src,$(2),$(if $(findstring /$(1),$(src)),$(src),))
############################################################################################

# Output directories might not exist if freshly pulled from git, or after clean
obj:
	mkdir obj

bin:
	mkdir bin

.cut:
	mkdir .cut

# File dependencies
.cut/$(DEPENDS): $(HEADERS) | .cut
	cd $(CUT_HOME)CUT && make build-cache
	$(BOOTSTRAP) --depends $(INCLUDE_FROM) ./ > $@

.SECONDEXPANSION:
obj/%.o: $$(call FINDSRC,$$(*F),$$(SOURCES)) | obj .cut/$(DEPENDS) 
	$(CMP) $(CFLAGS) $(LIBRARIES) $(INCLUDES) $(shell $(BOOTSTRAP) --include $(INCLUDE_FROM) $(notdir $<)) -c $< -o $@

depends:
	rm -rf .cut
	make .cut/$(DEPENDS)
	
clean-base:
	rm -rf obj
	rm -rf bin
	rm -rf .cut

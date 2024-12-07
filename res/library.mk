# This is a template for every C/C++ library that conforms to [this] standard
# More information: github.com/PhilLCar (philippe-caron@hotmail.com)

# NOTE: that since all paths are included, files within a project should have
# different names, even if they are not in the same subfolder.

# (!) The user should define NAME, FLAGS, LIBRARIES and PROJECT_ROOTS
# according to their needs.

.DEFAULT_GOAL = library
INCLUDE_FROM  = headers

#include common commands
include $(CUT_HOME)CUT/res/common.mk

# Default to C
ifeq "$(LANG)" "C++"
	NAME   := $(NAME)$(MAJOR)++
else
	NAME   := $(NAME)$(MAJOR)
endif

lib:
	mkdir lib

lib/lib$(NAME).a: $(OBJECTS) | lib
	ar crs $@ $?

# Global rules
library: lib/lib$(NAME).a

bin/$(NAME).test: lib/lib$(NAME).a | bin
	$(eval LIBS:= $(patsubst %, %/lib, $(shell $(BOOTSTRAP) --library)))
	$(eval LINK:= $(patsubst %, -L%, $(LIBS)))
	$(eval INC:=  $(patsubst %, -I%/inc, $(shell $(BOOTSTRAP) --library)))
	$(eval FILE:= $(filter-out -l:, $(foreach path, $(LIBS), -l:$(notdir $(wildcard $(path)/*)))))

	$(CMP) -g tst/main$(EXT_SRC) $(CFLAGS) $(LIBRARIES) $(INCLUDES) $(INC) -Llib -l$(NAME) $(LINK) $(FILE) -o bin/$(NAME).test 

reset:
	rm -f bin/$(NAME).test

test: reset bin/$(NAME).test
	./bin/$(NAME).test
	
clean: clean-base

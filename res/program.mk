# This is a template for every C/C++ program that conforms to [this] standard
# More information: github.com/PhilLCar (philippe-caron@hotmail.com)

# NOTE: that since all paths are included, files within a project should have
# different names, even if they are not in the same subfolder.

# (!) The user should define NAME, FLAGS, LIBRARIES and PROJECT_ROOTS
# according to their needs.

.DEFAULT_GOAL := program
INCLUDE_FROM   = sources

#include common commands
include $(CUT_HOME)CUT/res/common.mk

bin/$(NAME): obj/main.o | lib/lib$(NAME).a bin
	$(eval LIBS:= $(patsubst %, %/lib, $(shell $(BOOTSTRAP) --library)))
	$(eval LINK:= $(patsubst %, -L%, $(LIBS)))
	$(eval INC:=  $(patsubst %, -I%/inc, $(shell $(BOOTSTRAP) --library)))
	$(eval FILE:= $(filter-out -l:, $(foreach path, $(LIBS), -l:$(notdir $(wildcard $(path)/*)))))

	$(CMP) -g obj/main.o $(CFLAGS) $(LIBRARIES) $(INCLUDES) $(INC) $(LINK) $(FILE) $(FILE) $(LIBRARIES) -o bin/$(NAME) 

bin/%: obj/%.o | lib/lib$(NAME).a bin
	$(eval LIBS:= $(patsubst %, %/lib, $(shell $(BOOTSTRAP) --library)))
	$(eval LINK:= $(patsubst %, -L%, $(LIBS)))
	$(eval INC:=  $(patsubst %, -I%/inc, $(shell $(BOOTSTRAP) --library)))
	$(eval FILE:= $(filter-out -l:, $(foreach path, $(LIBS), -l:$(notdir $(wildcard $(path)/*)))))

	$(CMP) -g $< $(CFLAGS) $(LIBRARIES) $(INCLUDES) $(INC) $(LINK) $(FILE) $(FILE) $(LIBRARIES) -o $@

program: bin/$(NAME)

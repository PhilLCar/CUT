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


bin/$(NAME): obj/main.o | bin
	$(eval LIBS:= $(patsubst %, %/lib, $(shell $(BOOTSTRAP) --library)))
	$(eval LINK:= $(patsubst %, -L%, $(LIBS)))
	$(eval INC:=  $(patsubst %, -I%/inc, $(shell $(BOOTSTRAP) --library)))
	$(eval FILE:= $(filter-out -l:, $(foreach path, $(LIBS), -l:$(notdir $(wildcard $(path)/*)))))

	$(CMP) -g obj/main.o $(CFLAGS) $(LIBRARIES) $(INCLUDES) $(INC) $(LINK) $(FILE) $(FILE) -o bin/$(NAME) 

bin/$(NAME).test: bin
	$(eval LIBS:= $(patsubst %, %/lib, $(shell $(BOOTSTRAP) --library)))
	$(eval LINK:= $(patsubst %, -L%, $(LIBS)))
	$(eval INC:=  $(patsubst %, -I%/inc, $(shell $(BOOTSTRAP) --library)))
	$(eval FILE:= $(filter-out -l:, $(foreach path, $(LIBS), -l:$(notdir $(wildcard $(path)/*)))))

	$(CMP) -g tst/main$(EXT_SRC) $(CFLAGS) $(LIBRARIES) $(INCLUDES) $(INC) $(LINK) $(FILE) -o bin/$(NAME).test 

reset:
	rm -f bin/$(NAME).test

test: reset bin/$(NAME).test
	./bin/$(NAME).test

program: bin/$(NAME)
	
clean: clean-base
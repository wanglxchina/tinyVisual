#**********************************************************************************
# step 1:Compile related parameters, according to the need to modify 
#**********************************************************************************
#Target file name 
PROGRAM = tinyvisual

#Target file path
PROPATH = ../bin

#The source file directory 
SRCDIR = ../source ../util

#excluded files from SRCDIR when compile
EXCLUDESRC = 

#Attached header file directory
INCLUDEDIR = -I ./ -I ../util -I ../include

#Compiler and its parameters
CC = g++
CMYFLAGS = -Wall -rdynamic -std=c++11   
#CMYFLAGS += -U TEST-MACRO_1  undefine macro
#CMYFLAGS += -D TEST-MACRO_1  define macro 

#Linker and its parameters
#LD = ld
LDFLAGS = -lpthread -L ../lib/x86 -lx264 -ldl

#Parameters to generate dependency file 
DEPENDFLAG = -MM

#**********************************************************************************
# step 2:Automatic compile and link according to the relevant parameters.generally do not need to modify these.
#**********************************************************************************
ALLSRCS = $(foreach dir,$(SRCDIR),$(wildcard $(dir)/*.cpp))
SRCS = $(filter-out $(EXCLUDESRC),$(ALLSRCS))
OBJS = $(SRCS:%.cpp=%.o)
INLUDEDEPENDS = $(SRCS:%.cpp=%.d)
CFLAGS := $(CMYFLAGS) $(INCLUDEDIR)
DIREXSIT = $(shell if ! test -d $(PROPATH); then mkdir $(PROPATH);fi;)
$(PROGRAM):$(OBJS)
	@echo $(DIREXSIT)
	@$(CC) $(filter %.o,$^) $(LDFLAGS) -o $@
	@cp $@ $(PROPATH)/
	@rm $@ 
	@echo 
	@echo compile $(PROPATH)/$(PROGRAM) success!
	@echo
%.o:%.cpp
	@$(CC) -c -o $@ $(CFLAGS) $<
	@echo "$(CC)" $<
$(INLUDEDEPENDS):%.d:%.cpp
	@$(CC) $(DEPENDFLAG) $(CFLAGS)  $< | \
	sed "s?\\(.*\\):?$(basename $<).o $(basename $<).d :?g" \
	> $@ || $(RM) $@
include $(INCLUDEDEPANDS)

#**********************************************************************************
# step 3:Additional tools that will not be automatically executed.
#**********************************************************************************
.PHONY : clean
clean:
	@echo "start clean:"
	@echo $(OBJS)
	@echo $(INCLUDEDEPANDS)
	@echo $(PROPATH)/$(PROGRAM)
	@$(Q)-rm -f $(OBJS) $(INCLUDEDEPANDS) $(PROPATH)/$(PROGRAM) 
	@echo "Clean finish."

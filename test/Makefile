#---------
#
# CppUTest Examples Makefile
#
#----------

#Set this to @ to keep the makefile quiet
ifndef SILENCE
	SILENCE = @
endif

#--- Inputs ----#
COMPONENT_NAME = lwjson
CPPUTEST_HOME = ~/work/cpputest
CPPUTEST_CFLAGS += -std=c99
CPPUTEST_PEDANTIC_ERRORS = Y
CPPUTEST_WARNINGFLAGS = -Wall

CPPUTEST_USE_EXTENSIONS = Y
CPPUTEST_USE_GCOV = Y
CPP_PLATFORM = Gcc

SRC_DIRS = ../src

TEST_SRC_DIRS =\
	./\
	./mocks

INCLUDE_DIRS =\
  .\
  ../inc\
  $(CPPUTEST_HOME)/include

include $(CPPUTEST_HOME)/build/MakefileWorker.mk


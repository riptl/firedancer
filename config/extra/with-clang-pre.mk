# Default Clang executables
ifeq ($(CROSS),)
CC=clang-20
CXX=clang++-20
LD=clang++-20
endif

lib_LTLIBRARIES= libTecKitJni.la
libTecKitJni_la_SOURCES = ../../Engine.cpp TecKitJni.cpp

# set the include path found by configure
INCLUDES= $(all_includes) '-I../../Public-headers' -I ../..

# the library version and shared libs so there are no run time undefined symbols
libTecKitJni_la_LDFLAGS= -version-info 1 -lz 
libTecKitJni_la_CC=g++

bin_PROGRAMS= testTecKit
testTecKit_SOURCES= teckitjniTest.cpp 

# set the library search path
testTecKit_LDFLAGS =-lTecKitJni -lz 



CC = gcc
CXX = g++
CFLAGS = -Werror -Wall -pedantic -std=c11 -fpic
CXXFLAGS =  -Werror -Wall -pedantic -std=c++11 -fpic

result = libserialport.so

srcdir = src
incdir = inc
objdir = obj
bindir = bin

VPATH = $(srcdir) $(incdir)

cobjects = 
cxxobjects = serialport.o

.PHONY : all clean

all: $(result)

clean: 
#rm -vf $(objdir)/$(cobjects)
	rm -vf $(objdir)/$(cxxobjects)
	rm -vf $(bindir)/$(result) 

$(result): $(cobjects) $(cxxobjects)
	$(CC) -shared -Wl,-soname,$(result) -o $(bindir)/$@ $(objdir)/$^
	
$(cobjects): %.o: %.c %.h
	$(CC) -I $(incdir) -c $(CFLAGS) -o $(objdir)/$@ $<
	
$(cxxobjects): %.o: %.cpp %.h
	$(CXX) -I $(incdir) -c $(CXXFLAGS) -o $(objdir)/$@ $<
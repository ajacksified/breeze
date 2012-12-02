# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# 'install' program location 
INSTALL ?= install

# Location where the package is installed by 'make install' 
prefix ?= /usr/local

# Destination root (/ is used if empty) 
DESTDIR ?= 

# C++ compiler 
CXX = g++

# Standard flags for C++ 
CXXFLAGS ?= 

# Standard preprocessor flags (common for CC and CXX) 
CPPFLAGS ?= 

# Standard linker flags 
LDFLAGS ?= 



# -------------------------------------------------------------------------
# Do not modify the rest of this file!
# -------------------------------------------------------------------------

### Variables: ###

CPPDEPS = -MT$@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP
BREEZE_CXXFLAGS = -Ideps/libpropeller/include -Ideps/lua/src -O2 -D_THREAD_SAFE \
	$(CPPFLAGS) $(CXXFLAGS)
BREEZE_OBJECTS =  \
	obj/breeze_breeze.o \
	obj/breeze_main.o \
	obj/breeze_trace.o

### Conditionally set variables: ###



all: obj
obj:
	@mkdir -p obj

### Targets: ###

all: obj/breeze

install: install_breeze

uninstall: uninstall_breeze

clean: 
	rm -f obj/*.o
	rm -f obj/*.d
	rm -f obj/breeze
	-(cd deps/libpropeller && $(MAKE) clean)
	-(cd deps/lua && $(MAKE) clean)

libs: 
	cd deps/libpropeller && make && cd ../lua && make

obj/breeze: libs $(BREEZE_OBJECTS) 
	$(CXX) -o $@ $(BREEZE_OBJECTS) -Ldeps/libpropeller/obj -Ldeps/lua/src $(LDFLAGS)  -lpropeller -llua

install_breeze: obj/breeze
	$(INSTALL) -d $(DESTDIR)$(prefix)/bin
	install -c obj/breeze $(DESTDIR)$(prefix)/bin
	install -c deps/libpropeller/obj/libpropeller.so $(DESTDIR)$(prefix)/lib
	mkdir -p $(DESTDIR)$(prefix)/lib/breeze
	cp -r lua/*  $(DESTDIR)$(prefix)/lib/breeze/	

uninstall_breeze: 
	rm -f $(DESTDIR)$(prefix)/bin/breeze

obj/breeze_breeze.o: src/breeze.cpp
	$(CXX) -c -o $@ $(BREEZE_CXXFLAGS) $(CPPDEPS) $<

obj/breeze_main.o: src/main.cpp
	$(CXX) -c -o $@ $(BREEZE_CXXFLAGS) $(CPPDEPS) $<

obj/breeze_trace.o: src/trace.cpp
	$(CXX) -c -o $@ $(BREEZE_CXXFLAGS) $(CPPDEPS) $<

.PHONY: all install uninstall clean install_breeze uninstall_breeze


# Dependencies tracking:
-include obj/*.d

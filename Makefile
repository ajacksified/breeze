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

PLATFORM_LDFLAGS ?= 

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
PLATFORM_LDFLAGS = -lrt
endif



# -------------------------------------------------------------------------
# Do not modify the rest of this file!
# -------------------------------------------------------------------------

### Variables: ###

CPPDEPS = -MT$@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP
BREEZE_CXXFLAGS = -Ideps/libpropeller/include -Ideps/libpropeller/src -Ideps/libpropeller/deps/libevent/include -Ideps/lua/src -Ideps/cjson -O2 -D_THREAD_SAFE -pthread \
	$(CPPFLAGS) $(CXXFLAGS)
BREEZE_OBJECTS =  \
	obj/breeze_breeze.o \
	obj/breeze_metrics.o \
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
	-(cd deps/cjson && $(MAKE) clean)

libs: 
	cd deps/libpropeller && make && cd ../lua && make && cd ../cjson && make

obj/breeze: libs $(BREEZE_OBJECTS) 
	$(CXX) -o $@ $(BREEZE_OBJECTS)  -Ldeps/libpropeller/obj -Ldeps/libpropeller/deps/libevent/.libs -Ldeps/lua/src -Ldeps/cjson $(LDFLAGS)  -pthread -lpropeller -llua -lcjson  -levent -levent_pthreads $(PLATFORM_LDFLAGS)

install_breeze: obj/breeze
	$(INSTALL) -d $(DESTDIR)$(prefix)/bin
	install -c obj/breeze $(DESTDIR)$(prefix)/bin
	mkdir -p $(DESTDIR)$(prefix)/lib/breeze
	cp -r lua/*  $(DESTDIR)$(prefix)/lib/breeze/	
	chmod -R a+r $(DESTDIR)$(prefix)/lib/breeze/
	cd deps/lua && make install
	cp -r deps/lua/lua/*  $(DESTDIR)$(prefix)/lib/lua/5.2/
	chmod -R a+r $(DESTDIR)$(prefix)/lib/lua/5.2/
	

uninstall_breeze: 
	rm -f $(DESTDIR)$(prefix)/bin/breeze

obj/breeze_breeze.o: src/Breeze.cpp
	$(CXX) -c -o $@ $(BREEZE_CXXFLAGS) $(CPPDEPS) $<
	
obj/breeze_metrics.o: src/Metrics.cpp
	$(CXX) -c -o $@ $(BREEZE_CXXFLAGS) $(CPPDEPS) $<	

obj/breeze_main.o: src/main.cpp
	$(CXX) -c -o $@ $(BREEZE_CXXFLAGS) $(CPPDEPS) $<

obj/breeze_trace.o: src/trace.cpp
	$(CXX) -c -o $@ $(BREEZE_CXXFLAGS) $(CPPDEPS) $<

.PHONY: all install uninstall clean install_breeze uninstall_breeze


# Dependencies tracking:
-include obj/*.d

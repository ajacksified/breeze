# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# 'install' program location 
INSTALL ?= install

# Location where the package is installed by 'make install' 
prefix ?= /usr/local

# Destination root (/ is used if empty) 
DESTDIR ?= 

# Compiler flags to link shared library 
#Location
LINK_DLL_FLAGS ?= -shared

# C++ compiler 
CXX = g++

# Standard flags for C++ 
CXXFLAGS ?= 

# Standard preprocessor flags (common for CC and CXX) 
CPPFLAGS ?= 

# Standard linker flags 
LDFLAGS ?= 

AR = ar rcu
RANLIB= ranlib


# -------------------------------------------------------------------------
# Do not modify the rest of this file!
# -------------------------------------------------------------------------

### Variables: ###

CPPDEPS = -MT$@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP
PROPELLER_CXXFLAGS = -Iinclude -Ideps/libevent -Ideps/libevent/include -O2 -D_THREAD_SAFE -pthread $(CPPFLAGS) $(CXXFLAGS)
PROPELLER_OBJECTS =  \
	obj/propeller_propeller.o \
	obj/propeller_HttpProtocol.o \
	obj/propeller_Connection.o \
	obj/propeller_Server.o \
	obj/propeller_system.o \
	obj/propeller_trace.o \
	obj/propeller_event.o
PROPELLER_HEADERS =  \
	../include/propeller.h

### Conditionally set variables: ###

all: obj
obj:
	@mkdir -p obj

### Targets: ###

all: obj/libpropeller.a

install: install_propeller install_propeller_headers

uninstall: uninstall_propeller uninstall_propeller_headers

clean: 
	rm -f obj/*.o
	rm -f obj/*.d
	rm -f obj/libpropeller.so
	rm -f obj/libpropeller.so
	-(cd test && $(MAKE) clean)
	-(cd deps/libevent && $(MAKE) clean)


libevent: 
	cd deps/libevent && make

obj/libpropeller.a: libevent $(PROPELLER_OBJECTS) 
	$(AR) $@ $(PROPELLER_OBJECTS)
	$(RANLIB) $@
	
install_propeller: obj/libpropeller.so
	$(INSTALL) -d $(DESTDIR)$(prefix)/lib
	$(INSTALL) -m 644 obj/libpropeller.so $(DESTDIR)$(prefix)/lib
	install -c obj/libpropeller.so $(DESTDIR)$(prefix)/lib
	cd deps/libevent && make install

uninstall_propeller: 
	rm -f $(DESTDIR)$(prefix)/lib/libpropeller.so
	rm -f $(DESTDIR)$(prefix)/lib/libpropeller.so

install_propeller_headers: 
	$(INSTALL) -d $(DESTDIR)$(prefix)/include
	for f in $(PROPELLER_HEADERS); do \
	if test ! -d $(DESTDIR)$(prefix)/include/`dirname $$f` ; then \
	$(INSTALL) -d $(DESTDIR)$(prefix)/include/`dirname $$f`; \
	fi; \
	$(INSTALL) -m 644 src/$$f $(DESTDIR)$(prefix)/include/$$f; \
	done

uninstall_propeller_headers: 
	for f in $(PROPELLER_HEADERS); do \
	rm -f $(DESTDIR)$(prefix)/include/$$f; \
	done

test: 
	(cd test && $(MAKE) all)

obj/propeller_propeller.o: src/propeller.cpp
	$(CXX) -c -o $@ $(PROPELLER_CXXFLAGS) $(CPPDEPS) $<

obj/propeller_HttpProtocol.o: src/HttpProtocol.cpp
	$(CXX) -c -o $@ $(PROPELLER_CXXFLAGS) $(CPPDEPS) $<

obj/propeller_Connection.o: src/Connection.cpp
	$(CXX) -c -o $@ $(PROPELLER_CXXFLAGS) $(CPPDEPS) $<

obj/propeller_Server.o: src/Server.cpp
	$(CXX) -c -o $@ $(PROPELLER_CXXFLAGS) $(CPPDEPS) $<

obj/propeller_system.o: src/system.cpp
	$(CXX) -c -o $@ $(PROPELLER_CXXFLAGS) $(CPPDEPS) $<

obj/propeller_trace.o: src/trace.cpp
	$(CXX) -c -o $@ $(PROPELLER_CXXFLAGS) $(CPPDEPS) $<

obj/propeller_event.o: src/event.cpp
	$(CXX) -c -o $@ $(PROPELLER_CXXFLAGS) $(CPPDEPS) $<

.PHONY: all install uninstall clean install_propeller uninstall_propeller \
	install_propeller_headers uninstall_propeller_headers test


# Dependencies tracking:
-include obj/*.d

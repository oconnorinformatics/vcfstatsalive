CFLAGS=-g -std=c++14 -DHTSLIB
INCLUDES=-Ilib/vcflib/src -Ilib/vcflib -Ilib/jansson-2.6/src -Ilib/htslibpp -I$(HTSLIB)/include
LDADDS=-lbz2 -lcurl -lcrypto -lz -llzma -lstdc++ -pthread
LIBS=$(HTSLIB)/lib/libhts.a

SOURCES=main.cpp \
		AbstractStatCollector.cpp \
		BasicStatsCollector.cpp 
PROGRAM=vcfstatsalive
PCH_SOURCE=vcfStatsAliveCommon.hpp
PCH=$(PCH_SOURCE).gch

PCH_FLAGS=-include $(PCH_SOURCE)

OBJECTS=$(SOURCES:.cpp=.o)

JANSSON=lib/jansson-2.6/src/.libs/libjansson.a
VCFLIB=lib/vcflib/libvcf.a
DISORDER=lib/vcflib/smithwaterman/disorder.c

SCHEMA ?= DEBUG

all: $(PROGRAM)

.PHONY: all

checkvar:
	@if [ "x$(HTSLIB)" = "x" ]; then echo "HTSLIB need to be defined"; exit 1; fi

$(PROGRAM): checkvar $(PCH) $(OBJECTS) $(VCFLIB) $(JANSSON)
	$(CXX) $(CFLAGS) -D$(SCHEMA) -o $@ $(OBJECTS) $(LIBS) $(VCFLIB) $(JANSSON) $(DISORDER) $(LDADDS)

.cpp.o:
	$(CXX) $(CFLAGS) -D$(SCHEMA) $(INCLUDES) $(PCH_FLAGS) -c $< -o $@

$(PCH): 
	$(CXX) $(CFLAGS) -D$(SCHEMA) $(INCLUDES) -x c++-header $(PCH_SOURCE) -Winvalid-pch -o $@

.PHONY: $(PCH)

$(JANSSON):
	@if [ ! -d lib/jansson-2.6 ]; then cd lib; curl -o - http://www.digip.org/jansson/releases/jansson-2.6.tar.gz | tar -xzf - ; fi
	@cd lib/jansson-2.6; ./configure --disable-shared --enable-static; make; cd ../..

$(VCFLIB):
	make -C lib/vcflib libvcf.a

clean:
	rm -rf $(OBJECTS) $(PROGRAM) $(PCH) *.dSYM

clean-dep:
	make -C lib/vcflib clean
	make -C lib/jansson-2.6 clean

.PHONY: clean

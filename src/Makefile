UNAME		:= $(shell uname -s)
SRCS		:= $(wildcard *.c)

# CC = gcc
# CFLAGS += -g

INCLUDES	+= -I.
CFLAGS		+= -pipe -Wall -D_BSD_SOURCE -fPIC $(INCLUDES)
LDFLAGS		+= -lm

ifdef PROFILE
	CFLAGS += -pg -fprofile-generate -ftest-coverage
	LDFLAGS += -pg -lgcov -coverage
endif

ifdef NDEBUG
	CFLAGS += -DNDEBUG -O3
else
	CFLAGS += -g3 -fstack-protector-all
endif

# better stripping
CFLAGS += -fdata-sections
ifndef PROFILE
	CFLAGS += -ffunction-sections
endif
ifeq ($(UNAME),Darwin)
    LDFLAGS += -Wl
else
    LDFLAGS += -Wl,--gc-sections
endif

all: libminibloom.so libminibloom.a minibloomcat minibloomglue

libminibloom.so: minibloom.o minibloomfile.o murmur.o
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) -shared $^ -o $@
ifdef NDEBUG
	$(CROSS_COMPILE)strip $@
endif

libminibloom.a: minibloom.o minibloomfile.o murmur.o
	$(CROSS_COMPILE_AR)$(AR) $(ARFLAGS) $@ $^

minibloomcat: minibloom.o minibloomfile.o minibloomcat.o murmur.o
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) $^ -o $@

minibloomglue: minibloom.o minibloomfile.o minibloomglue.o murmur.o
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f libminibloom.so libminibloom.a *.o *.so *.gcno minibloomcat minibloomglue

.PHONY: all clean

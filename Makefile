UNAME	:= $(shell uname -s)

INCLUDES	+= -I. -I./src
CFLAGS		+= -pipe -Wall -D_BSD_SOURCE -fPIC $(INCLUDES)
LDFLAGS		+= -L./src -lm

ifeq ($(UNAME),Darwin)
    LDFLAGS += -Wl
    PKG_CONFIG_LIBS = `pkg-config --libs lua5.2`
    PKG_CONFIG_CFLAGS = `pkg-config --cflags lua5.2`
else
    LDFLAGS += -Wl,--gc-sections
    PKG_CONFIG_LIBS = `pkg-config --libs lua5.1`
    PKG_CONFIG_CFLAGS = `pkg-config --cflags lua5.1`
endif
LDFLAGS		+= $(PKG_CONFIG_LIBS)
CFLAGS		+= $(PKG_CONFIG_CFLAGS)

all: minibloom.so

src/libminibloom.a:
	$(MAKE) -C src

minibloom.so: minibloom.o src/libminibloom.a
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) -shared $^ -o $@ $(PKG_CONFIG_LIBS) src/libminibloom.a

minibloom.o: minibloom.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(INCLUDES) -fPIC -c -o $@ $< $(PKG_CONFIG)

clean:
	$(MAKE) -C src clean
	rm -rf  *.o *.so

.PHONY: all clean

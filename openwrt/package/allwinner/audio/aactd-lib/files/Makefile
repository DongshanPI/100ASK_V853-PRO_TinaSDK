TARGET = libaactd.so

SRCS = $(wildcard *.c)

override CFLAGS +=
override INCLUDES += -I./include
override LDFLAGS +=

all: $(TARGET) install

$(TARGET): $(SRCS)
	$(CC) $^ $(CFLAGS) $(INCLUDES) $(LDFLAGS) -fPIC -shared -o $@

install:
	mkdir -p $(INSTALL_PREFIX)/usr/lib
	cp $(TARGET) $(INSTALL_PREFIX)/usr/lib
	mkdir -p $(INSTALL_PREFIX)/usr/include
	cp -r ./include/* $(INSTALL_PREFIX)/usr/include

clean:
	rm -rf $(TARGET)

.PHONY: all clean install

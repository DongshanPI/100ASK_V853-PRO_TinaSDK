TARGET = aactd

SRCS = $(wildcard *.c)

override INCLUDES +=
override LDFLAGS += -laactd -lpthread

all: $(TARGET) install

$(TARGET): $(SRCS)
	$(CC) $^ $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@

install:
	mkdir -p $(INSTALL_PREFIX)/usr/bin
	cp $(TARGET) $(INSTALL_PREFIX)/usr/bin

clean:
	rm -rf $(TARGET)

.PHONY: all clean install

TARGET = aactd_test_client

SRCS = client.c

override CFLAGS +=
override INCLUDES +=
override LDFLAGS += -laactd

all: $(TARGET) install

$(TARGET): $(SRCS)
	$(CC) $^ $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@

install:
	mkdir -p $(INSTALL_PREFIX)/usr/bin
	cp $(TARGET) $(INSTALL_PREFIX)/usr/bin

clean:
	rm -rf $(TARGET)

.PHONY: all clean install

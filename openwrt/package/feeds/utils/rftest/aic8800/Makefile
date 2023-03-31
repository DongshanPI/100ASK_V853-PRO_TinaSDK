TARGET = aicrf_test

all: $(TARGET)
$(TARGET): aicrf_test.o
	@mkdir -p $(CONFIG_PREFIX)
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -I$(INCLUDES) $(LDFLAGS) $^ -o $@
	@cp $@ $(CONFIG_PREFIX)/aicrf_test


clean:
	$(RM) $(TARGET)

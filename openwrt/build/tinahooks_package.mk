
define TinaHooks/Elect/RetFilePath
$(dir $(shell $(if $(strip $(1)), \
  ( [ -e "machinfo/$(LICHEE_IC)/$(LICHEE_BOARD)/$(strip $(1))" ] && \
    echo "machinfo/$(LICHEE_IC)/$(LICHEE_BOARD)/$(strip $(1))" || \
    ( [ -e "machinfo/$(LICHEE_IC)/common/$(strip $(1))" ] && \
      echo "machinfo/$(LICHEE_IC)/common/$(strip $(1))" || \
      ( [ -e "machinfo/common/$(strip $(1))" ] && \
        echo "machinfo/common/$(strip $(1))" || echo "" ))),$(echo ""))))
endef

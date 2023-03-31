


# $(1)  Operate CMD
# $(2)  src file
# $(3)  dst file
define DownloadMethod/file/operate
	[ ! -f "$(2)" ] && echo "Error $(2) not exist." || \
	( [ -f "$(3)" ] && cmp $(2) $(3) > /dev/null || \
	(rm -f $(3) && $(1) $(2) $(3) ))
endef

define DownloadMethod/linkfile
	$(call wrap_mirror,$(1),$(2), \
		$(call DownloadMethod/file/operate,\
			ln -s,$(strip $(patsubst .%,$(shell pwd)%,$(URL))/$(if $(strip $(URL_FILE)),$(URL_FILE),$(FILE))),\
			$(DL_DIR)/$(FILE)))
endef

define DownloadMethod/copyfile
	$(call wrap_mirror,$(1),$(2), \
		$(call DownloadMethod/file/operate,\
			cp,$(strip $(patsubst .%,$(shell pwd)%,$(URL))/$(if $(strip $(URL_FILE)),$(URL_FILE),$(FILE))),\
			$(DL_DIR)/$(FILE)))
endef




include $(LICHEE_TOP_DIR)/openwrt/build/tinahooks_commonfunc.mk

define TinaHooks/PackagePre/BuildSoftlink
	echo "" ;\
	echo "Check Vendor Package..." && \
	[ -d "$(LICHEE_TOP_DIR)/openwrt/package" -a -e "$(TOPDIR)/package" ] &&  \
	( $(call TinaHooks/ForceCreateSoftLink, \
		$(LICHEE_TOP_DIR)/openwrt/package, $(TOPDIR)/package/subpackage) )\
	|| echo "Error: link $(LICHEE_TOP_DIR)/openwrt/package"; \
	echo "end";
endef



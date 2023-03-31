
# delete old link, force create a softlink
# $1  TARGET-NAME
# $2  LINK-NAME
define TinaHooks/ForceCreateSoftLink
echo "CreateSoftLink $(2) link to $(1)" ; \
[ ! -e $(2) ] && ln -sf $(1) $(2) || \
    ([ ! -L $(2) ] \
      && (echo "Warnning: $(2) is not softlink, recreate it" \
          && (rm -rf $(2) && ln -sf $(1) $(2)) )\
      || ( [ x"$(strip $(shell readlink -f $(2)))" = x"$(strip $(1))" ] \
          || (echo "Warnning: $(2) is incorrect, recreate it" \
              && (rm -rf $(2) && ln -sf $(1) $(2)) )))
endef



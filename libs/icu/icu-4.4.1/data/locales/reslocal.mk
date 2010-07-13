# reslocal.mk - eliminate locale data which xetex won't use anyhow

# Aliases which do not have a corresponding xx.xml file (see icu-config.xml & build.xml)
GENRB_SYNTHETIC_ALIAS = 


# All aliases (to not be included under 'installed'), but not including root.
GENRB_ALIAS_SOURCE = $(GENRB_SYNTHETIC_ALIAS) 


# Ordinary resources
GENRB_SOURCE = # en.txt


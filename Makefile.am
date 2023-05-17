####### kdevelop will overwrite this part!!! (begin)##########

SUBDIRS = src 

EXTRA_DIST = AUTHORS COPYING INSTALL README TODO CHANGELOG 

install-data-local:
	$(mkinstalldirs) 
	$(INSTALL_DATA) AUTHORS AUTHORS
	$(mkinstalldirs) 
	$(INSTALL_DATA) COPYING COPYING
	$(mkinstalldirs) 
	$(INSTALL_DATA) INSTALL INSTALL
	$(mkinstalldirs) 
	$(INSTALL_DATA) README README
	$(mkinstalldirs) 
	$(INSTALL_DATA) TODO TODO
	$(mkinstalldirs) 
	$(INSTALL_DATA) CHANGELOG CHANGELOG

uninstall-local:
	-rm -f AUTHORS
	-rm -f COPYING
	-rm -f INSTALL
	-rm -f README
	-rm -f TODO
	-rm -f CHANGELOG

####### kdevelop will overwrite this part!!! (end)############
AUTOMAKE_OPTIONS = foreign

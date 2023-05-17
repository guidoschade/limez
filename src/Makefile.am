####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = limez
limez_SOURCES = lightdir.cc ldapstorage.cc generator.cc manager.cc tools.cc main.cc receiver.cc transmitter.cc timer.cc task.cc storage.cc stat.cc signalhandler.cc semaphore.cc scheduler.cc resolver.cc rawdata.cc message.cc mailinglist.cc log.cc line.cc keyvalue.cc filestorage.cc dbstorage.cc datablock.cc database.cc connection.cc config.cc address.cc 
limez_LDADD   = 

SUBDIRS = include 

EXTRA_DIST = address.cc address_def.h address_inl.h config.cc config_def.h connection.cc connection_def.h database.cc database_def.h datablock.cc datablock_def.h dbstorage.cc dbstorage_def.h filestorage.cc filestorage_def.h keyvalue.cc keyvalue_def.h line.cc line_def.h list_def.h list_inl.h listelement_def.h listelement_inl.h log.cc log_def.h log_inl.h mailinglist.cc mailinglist_def.h message.cc message_def.h message_inl.h rawdata.cc rawdata_def.h resolver.cc resolver_def.h scheduler.cc scheduler_def.h semaphore.cc semaphore_def.h sender_def.h signalhandler.cc signalhandler_def.h stat.cc stat_def.h storage.cc storage_def.h task.cc task_def.h timer.cc timer_def.h keyvalue_inl.h datablock_inl.h connection_inl.h scheduler_inl.h limez limez.config transmitter.cc receiver.cc receiver_def.h transmitter_def.h main.cc tools.cc manager.cc manager_def.h generator_def.h generator.cc ldapstorage_def.h ldapstorage.cc task_inl.h lightdir.cc lightdir_def.h 

install-data-local:
	$(mkinstalldirs) 
	$(INSTALL_DATA) limez.config limez.config

install-exec-local:
	$(mkinstalldirs) 
	$(INSTALL_SCRIPT) limez limez

uninstall-local:
	-rm -f limez
	-rm -f limez.config

####### kdevelop will overwrite this part!!! (end)############
bin_PROGRAMS = limez

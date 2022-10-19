
MP_PKGS = PKG/MP/libsrc/* PKG/MP/src/*

SUBDIRS := $(MP_PKGS) tools/* ngap_asn comm_include liblocal eap5g sctpc ngapp nwucp dbm

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKEFLAGS)

all: 
	@for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir all; \
	done
new: 
	@for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir new; \
	done
install: 
	@for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir install; \
	done
clean: 
	@for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir clean; \
	done

.PHONY: subdirs $(SUBDIRS)
.PHONY: all new install clean

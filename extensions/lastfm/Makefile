XPIFILES = $(shell find chrome chrome.manifest components defaults install.rdf -type f)
VERSION=$(shell grep em:version install.rdf|sed -e 's/<[^>]*>//g'|sed -e 's/ //g')
XPI = "lastfm-$(VERSION).xpi"
$(XPI): $(XPIFILES)
	rm -f $(XPI)
	zip $(XPI) $(XPIFILES)

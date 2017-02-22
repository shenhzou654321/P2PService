.PHONY: all everything copybin debug clean

all: everything copybin

everything:
	$(MAKE) $(T) --directory=common
	$(MAKE) $(T) --directory=stuncore
	$(MAKE) $(T) --directory=networkutils
	$(MAKE) $(T) --directory=p2plib
	$(MAKE) $(T) --directory=relaynode
	$(MAKE) $(T) --directory=test
	$(MAKE) $(T) --directory=clientdemo

copybin: everything
	rm -rf bin
	mkdir bin
	rm -f ./bin/libp2plib.so ./bin/p2plibtest ./bin/relaynode ./bin/clientdemo
	cp p2plib/libp2plib.so ./bin
	cp test/p2plibtest ./bin
	cp relaynode/relaynode ./bin
	cp clientdemo/clientdemo ./bin


debug: T := debug
debug: all

profile: T := profile
profile: all

clean:	T := clean
clean: everything
	rm -f ./bin/p2plibtest ./bin/libp2plib.so ./bin/relaynode ./bin/clientdemo




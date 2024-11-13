default: client

install:
	@git clone https://github.com/Jacob-C-Smith/socket.git
	@cp socket/include/socket/socket.h .
	@cp socket/socket.c .
	@if [ -d socket ]; then rm -rf socket; fi
	@$(MAKE) client
	@if [ -d /usr/local/bin ]; then sudo cp datab /usr/local/bin/.; fi
	@export PATH=$PATH:/usr/local/bin

uninstall: clean
	@if [ -e /usr/local/bin/datab ]; then sudo rm /usr/local/bin/datab; fi

client:
	@gcc datab.c -o datab -lssh2 -L/bin/src -I/usr/local/include

clean:
	@if [ -e datab ]; then rm datab; fi
	@if [ -d socket ]; then rm -rf socket; fi

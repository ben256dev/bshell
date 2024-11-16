NEED_TARGETS = $(shell ./sh/check -t)

default: client

.PHONY: install
install:
	@make --no-print-directory install-target

.PHONY: install-target
install-target: $(NEED_TARGETS)

.PHONY: install-argon2
install-argon2:
	@git clone https://github.com/P-H-C/phc-winner-argon2.git
	@cd phc-winner-argon2 && gcc -fPIC -c src/argon2.c src/core.c src/blake2/blake2b.c src/thread.c src/encoding.c -Iinclude/ && gcc -shared -o libargon2.so argon2.o core.o blake2b.o thread.o encoding.o
	@sudo cp phc-winner-argon2/libargon2.so /usr/lib/
	@sudo cp phc-winner-argon2/include/argon2.h /usr/include/
	@rm -rf phc-winner-argon2

.PHONY: install-libssh
install-libssh2:

.PHONY: uninstall-argon2
uninstall-argon2:
	@ if [ -f /usr/lib/libargon2.so ]; then \
		sudo rm /usr/lib/libargon2.so; \
	fi
	@ if [ -f /usr/include/argon2.h ]; then \
		sudo rm /usr/include/argon2.h; \
	fi

.PHONY: install-datab
install-datab:
	@$(MAKE) client
	@if [ -d /usr/local/bin ]; then \
		sudo cp datab /usr/local/bin/.; \
	fi
	@export PATH=$PATH:/usr/local/bin

.PHONY: check
check:
	@./sh/check -s -q

.PHONY: uninstall
uninstall: clean uninstall-argon2
	@if [ -e /usr/local/bin/datab ]; then sudo rm /usr/local/bin/datab; fi

.PHONY: client
client:
	@gcc datab.c -o datab -lssh2 -L/bin/src -I/usr/local/include

.PHONY: clean
clean:
	@if [ -e datab ]; then rm datab; fi

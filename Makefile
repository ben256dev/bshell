NEED_TARGETS = $(shell ./sh/check -t)

.PHONY: default
default: client

.PHONY: reinstall
reinstall: uninstall install

.PHONY: install
install:
	@make --no-print-directory install-target
	@./sh/check -i7

.PHONY: install-target
install-target: $(NEED_TARGETS)

.PHONY: install-argon2
install-argon2:
	@git clone https://github.com/P-H-C/phc-winner-argon2.git
	@cd phc-winner-argon2 && gcc -fPIC -c src/argon2.c src/core.c src/blake2/blake2b.c src/thread.c src/encoding.c -Iinclude/ && gcc -shared -o libargon2.so argon2.o core.o blake2b.o thread.o encoding.o
	@sudo cp phc-winner-argon2/libargon2.so /usr/lib/
	@sudo cp phc-winner-argon2/include/argon2.h /usr/include/
	@rm -rf phc-winner-argon2
	@./sh/check -i4

.PHONY: install-libssh2
install-libssh2:
	@git clone https://github.com/libssh2/libssh2.git
	@cd libssh2 && cmake -B bld -DBUILD_SHARED_LIBS=ON && cmake --build bld
	@sudo cp libssh2/bld/src/libssh2.so /usr/lib/
	@sudo cp libssh2/include/libssh2.h /usr/include/
	@rm -rf libssh2
	@./sh/check -i2

.PHONY: install-datab
install-datab:
	@$(MAKE) client
	@if [ -d /usr/local/bin ]; then \
		sudo cp datab /usr/local/bin/.; \
	fi
	@export PATH=$PATH:/usr/local/bin
	@./sh/check -i1

.PHONY: uninstall-argon2
uninstall-argon2:
	@if [ -f /usr/lib/libargon2.so ]; then \
		sudo rm /usr/lib/libargon2.so; \
	fi
	@if [ -f /usr/include/argon2.h ]; then \
		sudo rm /usr/include/argon2.h; \
	fi

.PHONY: uninstall-libssh2
uninstall-libssh2:
	@if [ -f /usr/lib/libssh2.so ]; then \
		sudo rm /usr/lib/libssh2.so; \
	fi
	@if [ -f /usr/include/libssh2.h ]; then \
		sudo rm /usr/include/libssh2.h; \
	fi

.PHONY: uninstall-datab
uninstall-datab:
	@if [ -e /usr/local/bin/datab ]; then sudo rm /usr/local/bin/datab; fi

.PHONY: check
check:
	@./sh/check -s -q

.PHONY: uninstall
uninstall: clean uninstall-argon2 uninstall-libssh2 uninstall-datab

.PHONY: client
client:
	@gcc datab.c -o datab -lssh2 -L/bin/src -I/usr/local/include

.PHONY: clean
clean:
	@if [ -e datab ]; then rm datab; fi
	@if [ -d phc-winner-argon2 ]; then rm -rf phc-winner-argon2; fi
	@if [ -d libssh2 ]; then rm -rf libssh2; fi

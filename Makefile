PREFIX := /usr
BUILD_DIR := build/release
MANIFEST := $(PREFIX)/share/karousel-fx/install_manifest.txt

.PHONY: all install uninstall clean

all:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(PREFIX)
	cmake --build $(BUILD_DIR)

install:
	@test -f $(BUILD_DIR)/CMakeCache.txt || { echo "Run make before make install" >&2; exit 1; }
	cmake --install $(BUILD_DIR)
	install -D --mode 644 $(BUILD_DIR)/install_manifest.txt $(MANIFEST)

uninstall:
	@test -f $(MANIFEST) || { echo "karousel-fx is not installed" >&2; exit 1; }
	@set -e; \
	owned() { \
		if command -v pacman > /dev/null; then pacman -Qoq "$$1" > /dev/null 2>&1; \
		elif command -v dnf > /dev/null || command -v zypper > /dev/null; then rpm -qf "$$1" > /dev/null 2>&1; \
		elif command -v apt-get > /dev/null; then dpkg -S "$$1" > /dev/null 2>&1; \
		fi; \
	}; \
	removable() { \
		case $$1 in $(PREFIX)/*/*) [ -d "$$1" ] && [ -z "$$(ls -A "$$1")" ] && ! owned "$$1" ;; *) false ;; esac; \
	}; \
	dirs=$$(xargs --delimiter '\n' dirname -- $(MANIFEST) < $(MANIFEST) | sort -ru); \
	xargs --delimiter '\n' rm --force --verbose -- $(MANIFEST) < $(MANIFEST); \
	for dir in $$dirs; do \
		while removable "$$dir"; do \
			rmdir --verbose "$$dir"; \
			dir=$$(dirname "$$dir"); \
		done; \
	done

clean:
	rm -rf $(BUILD_DIR)

# GBC Poker — Texas Hold'em for Game Boy Color
#
# make test  — native unit tests (host gcc)
# make rom   — build build/gbc-poker.gbc with GBDK-2020
# make run   — open the ROM in Emulicious/SameBoy/OpenEmu (whichever is installed)

GBDK    ?= tools/gbdk
LCC      = $(GBDK)/bin/lcc
CC       = cc

# CGB-only, MBC5+RAM+Battery, 4 banks (64 KB)
LCCFLAGS = -Wm-yC -Wm-yt0x1B -Wm-ya1 -Wl-yo4 -Wm-yn"GBCPOKER"

CORE_SRC = src/rand.c src/deck.c src/eval.c src/game.c src/ai.c
ROM_SRC  = $(CORE_SRC) src/carddata.c src/sound.c src/ui.c src/text.c src/save.c src/main.c

.PHONY: all rom test run clean toolchain

all: test rom

# Fetch GBDK-2020 into tools/ (override URL for your platform; default macOS arm64).
GBDK_VERSION ?= 4.5.0
GBDK_URL ?= https://github.com/gbdk-2020/gbdk-2020/releases/download/$(GBDK_VERSION)/gbdk-macos-arm64.tar.gz
toolchain:
	mkdir -p tools
	curl -sL "$(GBDK_URL)" -o tools/gbdk.tar.gz
	tar -xzf tools/gbdk.tar.gz -C tools
	rm -f tools/gbdk.tar.gz
	@echo "GBDK ready at $(GBDK)"

build:
	mkdir -p build

test: build
	$(CC) -O2 -Wall -Wextra -Isrc -o build/test_eval $(CORE_SRC) tests/test_eval.c
	./build/test_eval

rom: build
	$(LCC) $(LCCFLAGS) -Isrc -o build/gbc-poker.gbc $(ROM_SRC)

run: rom
	@if command -v emulicious >/dev/null 2>&1; then emulicious build/gbc-poker.gbc; \
	elif [ -d "/Applications/SameBoy.app" ]; then open -a SameBoy build/gbc-poker.gbc; \
	elif [ -d "/Applications/OpenEmu.app" ]; then open -a OpenEmu build/gbc-poker.gbc; \
	else echo "No emulator found: install Emulicious or SameBoy, or copy build/gbc-poker.gbc to your R36S (roms/gbc)"; fi

clean:
	rm -rf build

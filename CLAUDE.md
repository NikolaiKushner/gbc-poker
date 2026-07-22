# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

```sh
make toolchain   # fetch GBDK-2020 into tools/gbdk (default macOS arm64; override GBDK_URL for other platforms)
make test        # build + run native unit tests with host cc (no emulator/GBDK needed)
make rom         # build build/gbc-poker.gbc via GBDK-2020 lcc
make run         # build rom, then open it in Emulicious / SameBoy / OpenEmu (whichever is installed)
make clean
```

There is no single-test runner: `tests/test_eval.c` is one native binary that runs every check (hand categories, tie-breaks, a 1M-hand evaluator cross-check, side-pot cases, and multi-thousand-hand fuzz runs). Edit that file to narrow what runs.

`make toolchain` must succeed before `make rom`/`make run` — GBDK is platform-specific and not committed. `make test` never needs GBDK.

## Architecture

The project's central design decision: **all game logic is pure, GBDK-free C** so the exact same sources compile both into the ROM and into a native host binary for fast, emulator-free testing.

- **Core (portable, GBDK-free):** `rand`, `deck`, `eval`, `game`, `ai`. These form `CORE_SRC` in the Makefile and are the *only* files linked into the native test build. Keep them free of GBDK headers, `malloc`, floats, and recursion — the GBC has none of those cheaply, and the tests link them on the host.
- **Platform layer (ROM-only):** `ui` (CGB background renderer), `sound` (APU), `save` (SRAM), `main` (menu + tournament loop), plus generated tile/font data. These may use GBDK.

### Data model

- A card is a single byte: `suit<<4 | rank` (`cards.h`, `RANK`/`SUIT`/`MAKE_CARD`). Rank is 2..14, ace high.
- `eval7()` (`eval.c`) takes 7 cards and returns a **32-bit comparable strength**: `(category<<20) | k1<<16 | ... | k5`. Larger = stronger; equal value = exact chop. This packing is what lets showdown and side-pot logic compare hands with plain integer `>`/`==`. Uses a rank histogram — no per-hand allocation.
- `game.c` is the betting state machine. Driver loop: while `!game_betting_done()`, read `g->to_act`, ask that seat (UI or `ai_decide`), call `game_apply_action()`; then `game_advance_street()` deals the next street or signals showdown. `game_showdown()` builds real side pots from each player's `total_bet` and awards into `won[]`, handling odd-chip splits. See `game.h` for the full flag/street/action constants.
- `ai.c`: three personas (`AI_PROFILES`, indexed by `Player.persona`) parameterized by aggression/looseness/bluff_freq, with light opponent modelling via `ai_note_fold_to_raise`. Scoring helpers (`ai_preflop_score`, `ai_postflop_score`) are exposed for tests.
- `save.c`: two independent SRAM blobs — persistent `SaveData` stats and a resumable `SavedGame` tournament snapshot taken at between-hands boundaries.

### Generated assets

`src/carddata.*`, `src/portraits.*`, and `src/fontmap.c` are **generated, not hand-edited**. Regenerate with the Python scripts in `assets/`:

```sh
python3 assets/gen_cards.py       # -> src/carddata.{c,h}, src/fontmap.c
python3 assets/gen_portraits.py   # -> src/portraits.{c,h}
```

Tile/portrait data lives in bank 1; `fontmap.c` is deliberately kept in the home bank so text draws work from any bank.

### ROM configuration

`LCCFLAGS` in the Makefile fix the cartridge as **CGB-only, MBC5 + RAM + battery, 4 banks (64 KB)**, title `GBCPOKER`. Adding code/data may require bumping the bank count (`-Wl-yo`).

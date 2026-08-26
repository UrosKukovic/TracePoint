# CLAUDE.md

Engineering discipline for AI-assisted work on TracePoint's firmware rework. See `REWORK.md` for the full plan and ticket groups, and `BUGS.md` for the running bug log.

## Hard constraints

1. Never add a dependency without asking first and stating why the standard library, ESP-IDF/Arduino core, or an existing dep can't do it.
2. Never touch files outside the current ticket's scope. No opportunistic refactoring, no "I also cleaned up X."
3. Never add a feature that wasn't asked for. If you think it's needed, say so in one line and wait.
4. Never rewrite working code just because it could have been written differently.
5. No file over ~300 lines. If it's heading there, stop and propose the split.

## Required behavior every session

1. Plan before code: files touched, approach, assumptions — no code until confirmed.
2. State assumptions explicitly; ask rather than silently pick when something's ambiguous.
3. One ticket at a time, from `REWORK.md`'s ticket groups, in order.
4. Each ticket starts from a clean working tree — if it isn't clean, stop and ask before proceeding.
5. Report what's incomplete or faked at the end of a ticket.

## Bug logging

The moment a fix is confirmed working, log it in `BUGS.md` before anything else, including the commit message. Every entry needs: Date, Ticket, Bug, Solution, Explanation (why it happened, why the fix works). Check for an existing entry before adding a new one — never log the same bug twice.

## Stop-the-line

Stop and reassess if: you can't explain what a file does without opening it, two files do overlapping things and it's unclear which is authoritative, the same bug gets fixed twice, or a ticket is running 3x longer than expected.

## Personal workflow

This project also uses a local, gitignored `CLAUDE.local.md` for the maintainer's personal learning workflow — a topic-by-topic, understanding-gated teaching process. It is not part of this repository's tracked history.

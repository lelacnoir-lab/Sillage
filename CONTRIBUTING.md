# Contributing to Sillage

Sillage is an eight-track groovebox for the M-VAVE FM-1. Use this repository for Sillage bugs and suggestions. See [project credits](LICENSING.md) for the upstream projects.

| What | Where |
|---|---|
| A bug you can reproduce | [Issues](https://github.com/lelacnoir-lab/Sillage/issues/new/choose): one issue per bug |
| A feature idea, a question | [Issues](https://github.com/lelacnoir-lab/Sillage/issues/new): say in the title which it is |

## Reporting a bug

Please include the Sillage version (hold HOME > ABOUT), your computer's OS and browser if the editor or installer
is involved, the steps to reproduce it, what you expected and what happened. A photo or a short video helps.

## Pull requests

Pull requests are welcome, including AI-assisted ones, with the same bar for all:

- keep it small and focused on one thing;
- make sure you understand the change and can explain it;
- follow [DESIGN.md](DESIGN.md): it says how every screen and control behaves; change it first if your change
  changes a rule;
- test it on a real FM-1;
- run `tests/run_tests.sh` (see [BUILDING.md](BUILDING.md)): every host test passes;
- new files carry the `SPDX-License-Identifier: GPL-3.0-only` header; preserve upstream copyright and attribution notices;
- bring in only code whose licence allows it (GPL-3.0-compatible, credited in [LICENSING.md](LICENSING.md)).

By contributing you agree that your contribution is licensed under GPL-3.0-only.

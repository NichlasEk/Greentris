# Greentris

Greentris is a green-themed homebrew SNES take on Tetris, built with PVSnesLib.

Created by Nichlas, with AI-assisted polish, iteration, and technical refinement.
The goal is simple: make Tetris feel crisp, stylish, and fun to play.

## Features

- Green-on-green visual identity with multiple shades
- Dedicated title screen
- Responsive sprite-based active piece rendering
- Score and high score tracking
- Level-based speed increase
- SNES ROM build and release script included

## Build

```sh
cd /home/nichlas/Greentris
. ./env.sh
make
```

## Release Build

```sh
cd /home/nichlas/Greentris
./build-release.sh
```

The release ROM is written to:

```sh
release/Greentris.sfc
```

## Tech

- Language: C
- Platform: Super Nintendo / SNES
- Library: PVSnesLib

## Notes

This project is intended to grow into a genuinely fun version of Tetris to play, not just a technical demo.

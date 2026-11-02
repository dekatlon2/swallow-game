# Swallow

A terminal-based arcade game written in C. Made for the Introduction to Programming course, December 2025.

## Gameplay

You control a swallow flying across the sky and collecting stars while avoiding hazards:

- **Hunters** appear from the sides of the screen, lock onto your position and charge at you. After a few seconds they stop, re-target and charge again.
- **Fog** covers part of the screen and grows larger as the game goes on.
- **Wind** changes how hard it is to fly in each direction.

## How to run

```bash
gcc swallow.c -o swallow -lncurses
./swallow
```

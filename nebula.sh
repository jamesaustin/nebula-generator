#!/usr/bin/env sh
make all
time python3 nebula.py nebula.json assets/progress
time ./simulate assets/progress/000.nebula.txt assets/octave/octave.png assets/final/000.nebula.png

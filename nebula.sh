#!/usr/bin/env sh
make all && \
    time \
    python3 nebula.py nebula.json --verbose --path assets/final --noise assets/octave/octave.png \
    | ./simulate assets/octave/octave.png assets/final/000.nebula.sim.png \
    && open .

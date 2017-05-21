#!/usr/bin/env sh
time python3 nebula.py nebula.json \
	--verbose \
	--path assets/final \
	--progress assets/progress/pngs \
	--noise assets/octave/octave.png 
open .

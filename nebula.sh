#!/usr/bin/env sh
time python3 nebula.py nebula.json \
	--verbose \
	--path assets/final \
	--noise assets/octave/octave.png \
	&& open .

#--progress assets/progress/pngs \

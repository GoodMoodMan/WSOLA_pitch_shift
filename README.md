# WSOLA_pitch_shift
Pitch shifting project based on the WSOLA algorithm.

DONE:
 - WSOLA algorithm implemented successfuly (not real rime)

Output wav file will be a sped up/down version maintaining the same Pitch. 

Currently the program accepts wav files recorded using mono sound (1 channel) with a standard Sample Rate of 44.1 kHz.
Using 'tinywav' open source code made by Martin Roth for reading and writing wav files.

The next steps include:
 - implementing a fast (real time) resampling method
 - rebuilding the algorithm to work in a real time environment
 - implementing the algorithm in a driver framework for real time sound input and output

Usage:

make

./main "filename.wav" 'shift_percent' 

for example:
./main "test.wav" 90

further examples can be found in the example folder



## Tremolo + Bandpass
Building a new effect pedal

The idea is to apply a tremolo effect to only a selected frequency band, notated with both high and low frequency parameters available to the musican via potentiometers. The current prototype adjusts the center frequency and q factors with the potentiometers on the daisy pod.

Project modes, selectable via the rotary encoder:

# Blue led mode
Tremolo + Bandpass Mode. Pot 1 sets the center frequency of the bandpass, Pot 2 sets the q. 

# Pink led mode
Tremolo Mode. Used to dial in the tremolo rate with Pot 1 and the tremolo depth with Pot 2. Settings persist in Blue Mode.

# Green led mode
SVF mode. Used to evaluate the svf filter. Not used in the Tremolo + Bandpass currently.


Theory of the Single Order All Pass filter is here: 

http://synthnotes.ucsd.edu/wp4/index.php/2019/11/09/second-order-allpass-filter/

http://synthnotes.ucsd.edu/wp4/index.php/2019/11/10/bandpass-and-bandreject-derived-from-allpass-filter/

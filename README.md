# CondorRudderPedal_ESP8622

### What it is used for:
This is an embedded C++ project using PlatformIO. I decided to build my own rudder pedals for the glider flight simulator [condor](https://www.condorsoaring.com/).
The mechanical core is an old fitness stepper with the pedals rearranged in a more vertical manner. To easy joyce to read the pedal position would have been 2 potentiometers. However, adding them mechanically 
would not have been an easy task. As I was on the look for a fun embedded project anyway, I decided to build my own self-made [sine-cosine-encoder](https://www.motioncontroltips.com/what-is-a-sine-encoder-aka-sine-cosine-encoder/) - mechanically comprised of a 3D printed toothed rack and 2 self-built IR light barriers. It is an ongoing project. 

![Alt Text](/docs/pics/pedals.jpg?raw=true "pedals") ![Alt Text](/docs/pics/encoder_mechanic.jpg?raw=true "encoder_mechanic") 
![Alt Text](/docs/pics/Opamp_Multiplexer.jpg?raw=true "Opamp_Multiplexer") ![Alt Text](/docs/pics/NodeMCU.jpg?raw=true "NodeMCU") 

### Functions:
- each pedal has its encoder.
- from the pedal positions a rudder position is calculated and sent via RS232 to an arduino UNO.
- the arduino works as a [HID](https://en.wikipedia.org/wiki/Human_interface_device) and just transfers the data to the PC where the simulator (Condor or any other flight simulator) is running. 
The arduino program is based on [UnoJoy](https://github.com/AlanChatham/UnoJoy)) and is NOT part of this repository.

### How it's done: 
Each encoder works roughly like this:
- the 2 light barriers are fixed and the toothed rack is moving with the pedal, creating a signal very close to a sinus.
- the second light barrier is mounted at a distance to create an approx. 90° shift between the two signals, further called channel A and B.
- from channel A and B a "summary-" and a "difference-channel" is calculated.
- tooth counting: each time the "difference-channel" crosses 0, the two signals A and B are crossing. That's where the teeth were counted (actually each halfteeth). 
- to get a higher resolution, interpolation is done between the half teeth using the "summary-channel".
- the basic principle is already working. The current problem is that my toothed rail is printed quite irregularly. To see how much of the irregularity could be filtered numerically I tried several approaches.
 This is still an ongoing process. Therefore the current version of the repository probably contains a lot of functions that won't be used in the future. It also contains a lot of branches which I used to try new approaches. For the moment I keep them all to be able to go back if needed.

### Plan: 
If it works well, I will probably spin off some arduino libraries of the project: maybe one for sine-cosine-encoding which would then use another one containing all the timer and filter functions I created.

# CondorRudderPedal_ESP8622

### What it is used for:
This is an embedded C++ project using PlatformIO. I decided to build my own rudder pedals for the glider flight simulator [condor](https://www.condorsoaring.com/).
The mechanical core is an old fitness stepper with the pedals rearanged in a more vetical manner. To easy joyce to read the pedal position would have been 2 potentiometers. However, adding them mechanically 
would not have been an easy task. As I was on the look for a fun embedded project anyway, I decided to build my own selfmade [sine-cosine-encoder](https://www.motioncontroltips.com/what-is-a-sine-encoder-aka-sine-cosine-encoder/) - mechanically comprised of a 3D printed toothed rack and 2 selfbuild IR light barriers. It is an ongoing project. 

### Functions:
- each pedal has its own encoder.
- from the pedal positions a rudder postion is calculated and sent via RS232 to an arduino UNO.
- the arduiuno works as a [HID](https://en.wikipedia.org/wiki/Human_interface_device) and just transfairs the data to the PC where the simulator (Condor or any other flight simulator) is running. 
The arduino programm is based on [UnoJoy]([https://github.com/kegarlv/ArduinoJoy](https://github.com/AlanChatham/UnoJoy)) and is NOT part of this repository.



### How it's done: 
Each encoder works roughly like this:
- the 2 light barriers are fix and the toothed rack are moving with the pedal, creating a signal very close to a sinus.
- the second light barrier is mounted in a distance to create a approx. 90° shift between the two signals, further called channel A and B.
- from channel A and B a "summery-" and a "difference-channel" is calcualated.
- tooth counting: each time the difference-channel crosses 0, the two signals A and B are crossing. Thats where the teeth were counted (actually a halfteeth). 
- to get a higher resolution, an interpolation is done between the halfteeth using the summary-channel.
- the basic principle is already working. The current problem is that my toothed rail is printed quite irregular. In order to see how much of the irregularity could be filtered numerically I tried several aproaches.
 This is still an ongoing process. Therefore the current version of the repository probably contains a lot of functions which won't be used in the future. It also contains a lot of branches which I used to try new appoaches. For the moment i keep them all to be able to go back if needed.

### Future plan: 
If it works well I will probably spin of some arduino libraries of the project: maybe one for sine-cosine-encoding which would then use another one containing all the timer and filter functions I created.

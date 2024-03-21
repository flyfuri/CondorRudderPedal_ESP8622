# CondorRudderPedal_ESP8622

### What it is used for:
This is an embedded C++ project using PlatformIO. I decided to build my own rudder pedals for the glider flight simulator [condor](https://www.condorsoaring.com/).
The mechanical core is an old fitness stepper with the pedals rearranged more vertically. The easy choice to read the pedal position would have been 2 potentiometers. However, adding them mechanically would not have been an easy task. As I was on the lookout for a fun embedded project anyway, I decided to build my own self-made [sine-cosine-encoder](https://www.motioncontroltips.com/what-is-a-sine-encoder-aka-sine-cosine-encoder/) - mechanically comprised of a 3D-printed toothed rack and 2 self-built IR light barriers. Important note if someone wants to build a similar stile encoder: make sure that the printing material for the light barrier parts and the toothed rack are not transparent to IR light - like some plastics are!

![Alt Text](/docs/pics/pedals.jpg?raw=true "pedals") 

### Functions:
It roughly works like this:
- Each encoder has two fix light barriers and a toothed rack moving with the pedal, creating two signals very close to a sine.
- The two light barriers of each encoder are mounted at a distance to create an approx. 90° shift between the two signals.
- Each pedal has its encoder (see [DirtySignal_Sin_Cos_Encoder](https://github.com/flyfuri/DirtySignal_Sin_Cos_Encoder)).
- From the pedal positions, a rudder position is calculated and sent via RS232 to an Arduino UNO.
- The Arduino works as a [HID](https://en.wikipedia.org/wiki/Human_interface_device) and just transfers the data to the PC where the simulator (Condor or any other flight simulator) is running. 
  The Arduino program is based on [UnoJoy](https://github.com/AlanChatham/UnoJoy) and is NOT part of this repository.
  
![Alt Text](/docs/pics/encoder_mechanic.jpg?raw=true "encoder_mechanic") 
![Alt Text](/docs/pics/Opamp_Multiplexer.jpg?raw=true "Opamp_Multiplexer") ![Alt Text](/docs/pics/NodeMCU.jpg?raw=true "NodeMCU") 

### External light disturbance compensation:
Any external light source with a high enough IR component (ex. sunlight) can disturb the measurement of the light barriers. For compensation, the IR LEDs of all four light barriers can be shut off via a transistor, allowing a measurement of the disturbance, which can then be subtracted from the regular measurement.

### Libraries used: 
The library [DirtySignal_Sin_Cos_Encoder](https://github.com/flyfuri/DirtySignal_Sin_Cos_Encoder) is used to condition and encode the signals. Another library [TimerAndFilter](https://github.com/flyfuri/TimerAndFilter) contains an easy to use timer class and 2 multi-filter classes. Both libraries were originally developed for this project.

### Hardware:
[<img src="/docs/pics/schematic_small_pic.JPG">](/docs/schematics/CondorRudderPedal_ESP8622.pdf)\
Click the picture to get to the larger pdf.

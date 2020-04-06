## AnyScatter: Eliminating Technology Dependency in Ambient Backscatter Systems
- Author: Taekyung Kim
- Contact: tkkim92@korea.ac.kr
- Description:  This is a technology independent real-time ambient backscatter receiver for GNU Radio.

### Dependencies
- [libzmq](https://github.com/zeromq/libzmq)

### Installation

```
git clone git://github.com/tkkim92/gr-AnyScatter.git
cd gr-AnyScatter
mkdir build
cd build
cmake ..
make install
```

### Usage 

- AnyScatter achieves ambient backscatter transmission over all single stream wireless signals including Wi-Fi, Bluetooth, and even a white noise.

- Connect the REF OUT and PPS TRIG OUT ports of the first USRP X310 (```addr0=192.168.40.2```) to the REF IN and PPS TRIG IN ports of the second USRP X310 (```addr1=192.168.41.2```) for accurate device clock synchronization. Then, run ```examples/usrp_rx.py```.

- You can make a backscatter device with the Raspberry Pi and an RF switch (e.g., [HMC194AMS8](https://www.analog.com/media/en/technical-documentation/data-sheets/hmc194a.pdf)). Leave one port unconnected and connect a 50 Ω termination (OOK) or a short circuit termination (BPSK) to the another port. ```examples/raspberry.py``` is an example code for this implementation method.

### References
AnyScatter: Eliminating Technology Dependency in Ambient Backscatter Systems<br>
Taekyung Kim and Wonjun Lee<br>
IEEE INFOCOM 2020

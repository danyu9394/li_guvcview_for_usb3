# li_guvcview_for_usb3

__Modified based on guveview version2.0.5.__
- Support viewing most LI-USB3 bayer sensors 
- Capture raw frames 
- Registers read/write

##Install guvcview
###dependencies:
make sure the following libraries have installed
```
#/bin/sh
sudo apt-get install intltool
sudo apt-get install autotools-dev
sudo apt-get install libsdl1.2-dev
sudo apt-get install libgtk-3-dev
sudo apt-get install portaudio19-dev
sudo apt-get install libpng12-dev
sudo apt-get install libavcodec-dev
sudo apt-get install libavutil-dev
sudo apt-get install libv4l-dev
sudo apt-get install libudev-dev
sudo apt-get install libusb-1.0-0-dev
sudo apt-get install libpulse-dev
sudo apt-get install libsdl2-dev
sudo apt-get install libsfml-dev
sudo apt-get install qtbase5-dev
sudo apt-get install libgsl0-dev
```

###build configuration:
After running the following commands, guvcview executable will be appeared under __/guvcview__
```
#/bin/sh
chmod +x bootstrap.h
./bootstrap.h
./configure
make
sudo make install
```

##Run application:
First identify your camera device using:
```
lsusb
ls /dev/video*
```
Run your camera device by changing /dev/video#
```
./guvcview/guvcview -d /dev/video0  
```

#Add more sensors for debayering
Inside __frame_decode.c__, under __case V4L2_PIX_FMT_YUYV__, add the pixel order accordingly for color sensor.
Use mono2yu12 for monochrome sensor.

##Test platform:
__4.15.0-32-generic #35~16.04.1-Ubuntu__
__4.15.0-20-generic #21~Ubuntu 18.04.1 LTS__

_Author: Danyu LI   Date: 2018/Aug_

<<<<<<< HEAD
GTK UVC VIEWER (guvcview)
*************************

Basic Configuration
===================
Dependencies:
-------------

Guvcview depends on the following:
 - intltool,
 - autotools, 
 - libsdl2 and/or sfml, 
 - libgtk-3 or libqt5, 
 - portaudio19, 
 - libpng, 
 - libavcodec, 
 - libavutil, 
 - libv4l, 
 - libudev,
 - libusb-1.0,
 - libpulse (optional)
 - libgsl0 (optional)

On most distributions you can just install the development 
packages:
 intltool, autotools-dev, libsdl2-dev, libsfml-dev, libgtk-3-dev or qtbase5-dev, 
 portaudio19-dev, libpng12-dev, libavcodec-dev, libavutil-dev,
 libv4l-dev, libudev-dev, libusb-1.0-0-dev, libpulse-dev, libgsl0-dev

Build configuration:
--------------------
(./bootstrap.sh; ./configure)

The configure script is generated from configure.ac by autoconf,
the helper script ./bootstrap.sh can be used for this, it will also
run the generated configure with the command line options passed.
After configuration a simple 'make && make install' will build and
install guvcview and all the associated data files.

guvcview will build with Gtk3 support by default, if you want to use 
the Qt5 interface instead, just run ./configure --disable-gtk3 --enable-qt5
you can use SDL2 (enabled by default) and/or SFML (disabled by default) 
as the rendering engine, both engines can be enabled during configure 
so that you can choose between the two with a command line option.
 

Data Files:
------------
(language files; image files; gnome menu entry)

guvcview data files are stored by default to /usr/local/share
setting a different prefix (--prefix=BASEDIR) during configuration
will change the installation path to BASEDIR/share.

Built files, src/guvcview and data/gnome.desktop, are dependent 
on this path, so if a new prefix is set a make clean is required 
before issuing the make command. 

After running the configure script the normal, make && make install 
should build and install all the necessary files.    
    
 
guvcview bin:
-------------
(guvcview)

The binarie file installs to the standart location,
/usr/local/bin, to change the install path, configure
must be executed with --prefix=DIR set, this will cause
the bin file to be installed in DIR/bin, make sure 
DIR/bin is set in your PATH variable, or the gnome 
menu entry will fail.

guvcview libraries:
-------------------
(libgviewv4l2core, libgviewrender, libgviewaudio, libgviewencoder)

The core functionality of guvcview is now split into 4 libraries
these will install to ${prefix}/lib and development headers to
${prefix}/include/guvcview-2/libname. 
pkg-config should be use to determine the compile flags.


guvcview.desktop:
-----------------

(data/guvcview.desktop)

The desktop file (gnome menu entry) is built from the
data/guvcview.desktop.in definition and is dependent on the 
configure --prefix setting, any changes to this, must 
be done in data/guvcview.desktop.in.

configuration files:
--------------------
(~/.config/guvcview2/video0)

The configuration file is saved into the $HOME dir when 
exiting guvcview. If a video device with index > 0,
e.g: /dev/video1 is used then the file stored will be
named ~/.config/guvcview2/video1

Executing guvcview
================== 

For instructions on the command line args 
execute "guvcview --help".
=======
# li_guvcview_for_usb3
guvcview is a simple interface for capturing and viewing video from v4l2 devices, with a special emphasis for the linux uvc driver. This is Leopard Imaging modified version for guvcivew.
_Modified based on guveview version2.0.5._
- Support viewing most LI-USB3 bayer sensors 
- Capture raw frames 
- Registers read/write

## Install guvcview
### Dependencies:
Make sure the following libraries have installed
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

### Build Configuration:
After running the following commands, guvcview executable will be appeared under __/guvcview__
```
#/bin/sh
chmod +x bootstrap.h
./bootstrap.h
./configure
make
sudo make install
```

## Run Application:
First identify your camera device using:
```
lsusb
ls /dev/video*
```
Run your camera device by changing /dev/video#
```
./guvcview/guvcview -d /dev/video0  
```

## Add more sensors for debayering

Inside __frame_decode.c__, under __case V4L2_PIX_FMT_YUYV__, add the pixel order accordingly for color sensor.
Use mono2yu12 for monochrome sensor.

## Test platform:
__4.15.0-32-generic #35~16.04.1-Ubuntu__
__4.15.0-20-generic #21~Ubuntu 18.04.1 LTS__

_Author: Danyu LI   Date: 2018/Aug_
>>>>>>> cc1f646933ec6717ccb1513e26c91e1e6a6dc915

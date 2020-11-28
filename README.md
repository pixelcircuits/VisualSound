# VisualSound (LED Cube)
Bluetooth Speaker Visualizer powered by Raspberry Pi (LED Cube Version)

## Software Setup
Please follow these steps to install and setup the Visualizer on a Raspberry Pi 3 (other Raspberry Pi versions not yet supported).

### 1. Install and Setup Raspbian Lite
a) The first thing to do is start with a fresh SD card with [Raspbian Lite](https://www.raspberrypi.org/downloads/raspbian/) loaded. Refer to [this tutorial](https://www.raspberrypi.org/documentation/installation/installing-images/README.md) for getting RaspberryPi OS up and running (last known working version is 2020-05-27)

b) Boot up the Raspberry Pi and from the command line run the raspi configuration tool and set the following options
```
sudo raspi-config
	Network Options -> Wi-Fi (setup)
```
c) Make sure the package manager is up to date
```
sudo apt-get update
sudo apt-get dist-upgrade -y
```
d) Install the following dependencies
```
sudo apt-get install -y git 
sudo apt-get install -y bluealsa libasound2-dev 
sudo apt-get install -y dbus libdbus-1-dev libdbus-glib-1-2 libdbus-glib-1-dev
sudo apt-get install -y scons
```

### 2. Configure Bluetooth
a) Modify the bluealsa service by running the nano text editor (press ctrl+x to exit and make sure to save)
```
sudo mkdir /etc/systemd/system/bluealsa.service.d
sudo nano /etc/systemd/system/bluealsa.service.d/override.conf
   [Service]
   ExecStart=
   ExecStart=/usr/bin/bluealsa -i hci0 -p a2dp-sink
   ExecStartPre=/bin/sleep 1
```
b) Modify bluetooth config
```
sudo nano /etc/bluetooth/main.conf
   [General]
   Class = 0x200414
   DiscoverableTimeout = 0

   [Policy]
   AutoEnable=true
```
c) Set a "pretty" bluetooth name
```
sudo hostnamectl set-hostname --pretty "VisualSound"
```

### 3. Disable the onboard audio
a) Both the LED Panel and LED Strip library need exclusive access to the PWM module, so the onboard audio needs to be disabled
```
sudo nano /boot/config.txt
   dtparam=audio=off
```

### 4. Install the LED Panel and LED Strip library
a) Run the following commands to install the library used for SPI
```
sudo apt-get install wiringpi
```

### 5. Build and Install VisualSound
a) Run the following commands to download the source code and build the VisualSound project
```
cd /home/pi
git clone https://github.com/pixelcircuits/VisualSound.git
cd VisualSound
mkdir build
make
```
b) To have the application run on boot add the following lines before the exit line to rc.local
```
sudo nano /etc/rc.local
   cd /home/pi/VisualSound
   sudo ./VisualSound
```

### 6. (Optional) Disable Power Key Function
a) Some USB rf controllers have a power button that the system forces shutdown from. To disable this behavior on the system level, add the following config
```
sudo nano /etc/systemd/logind.conf 
   HandlePowerKey=ignore
```

### 7. (Optional) Use a dedicated USB Bluetooth dongle
a) The onboard bluetooth doesn't have the best reception. To use a USB bluetooth dongle instead, add the following config
```
sudo nano /boot/config.txt
   dtoverlay=pi3-disable-bt
```

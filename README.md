# __caw__ audio processing program

__caw__ is an interactive, real-time audio processing environment based on [libcw](https://github.com/currawong-project/libcw).
The environment implements a data flow processing model which is specified with a JSON like configuration language.

The best introduction to the language is [__caw__ by Example](https://github.com/currawong-project/caw/blob/master/examples/examples.md)
This tutorial steps through the basic language constructs and theory of operation.

# Installation

## Prerequisites:

Fedora
```
sudo dnf install autoconf autoconf-archive automake libtool gcc-c++ gdb fftw-devel alsa-lib-devel libwebsockets-devel libubsan  
```

Ubuntu
```
sudo apt install autoconf libtool fftw-dev libwebsockets-dev libatlas-base-dev libasound2-dev libubsan1 
```

Note that libcw will take advantage of the [Intel Math Kernel Library](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html)
if it is enabled via the configure `enable-mkl` switch.

## Build

Get the project code
```
cd ~/src
git clone https://github.com/currawong-project/caw.git
cd caw/src
git clone https://github.com/currawong-project/libcw.git
cd libcw
```

Build
```
cd ~/src/caw
rm -rf build
cmake  -B build/debug -DCMAKE_INSTALL_PREFIX=build/debug/install --preset debug # (or --preset release)
cmake --build build/debug --preset debug
cmake --install build/debug 
```



## Command Line

```
       caw ui        <program_cfg_fname> {<program_label>} : Run with a GUI.
       caw exec      <program_cfg_fname> <program_label>   : Run without a GUI.
       caw hw_report <program_cfg_fname>                   : Print the hardware details and exit.
       caw test      <test_cfg_fname> (<module_label> | all) (<test_label> | all) (compare | echo | gen_report )* {args ...}
       caw test_stub ...
```

Test Example Command line
```
caw test     ~/src/cwtest/src/cwtest/cfg/test/main.cfg /time all echo
```


## Prevent Wireplumber from holding an audio device:
```
wpctl status                         # get the device id
wpctl set-profile <device-id> off    # disable the device
```


# Install rtpmidid


sudo dnf install avahi nss-mdns
sudo systemctl enable --now avahi-daemon

Install the rtpmidid service
```
dnf install rtpmidid-fedora-43-x86_64-26.01-1.fc43.rpm 
```

Create a user named rtpmidid and assign them to the group audio.
This rtpmidid daemon executes as this user
```
sudo useradd -r -s /usr/sbin/nologin -G audio rtpmidid 
```


0. Verify that the rtpmidi deamon is activated:
`systemctl status rtpmidid`

1. Be sure that the xioxm is on the same sub-net as the computers it will communicate with
This may involve giving it a static IP via Auracle-X.  (e.g. 192.168.8.146)

Hints:
- Disable WiFi
- Verify the expecte IP address is being used via `ip addr`

2. Open the firewall
```
sudo firewall-cmd --add-port=5004-5011/udp  # use  --permanent to open the ports permanently
```
3. Use Auracle-X to verify that "mixXM FFC-01" is setup as a 'responder' 

// SKIP THIS STEP IT DOES NOT SEEM TO BE NECESSARY:
//Connect the Fedora workstation that will send/receive data from/to DIN 1 on the xioxm.
// `rtpmidid-cli connect name="mioXM FFC-01" hostname=192.168.8.146 port=5004`

4. `aconnect -l` should list `rtpmidid` as a device and `mioXM FFC-01` as an input and output port.

5. Create a caw `midi_in` or `midi_out` object to send/receive to/from DIN 1.

`m_in: { class: midi_in, args:{ print_fl:true, dev_label:"rtpmidid",    port_label:"mioXM FFC-01" }, ui:{create_fl:true} },`


6. Note that on the xioXM DIN 1 connected to `mioXM FFC-01` by default.  




./rtpmidi-cli.py 

To use RTP-MIDI, you must open UDP ports 5004 and 5005. 
Each additional virtual MIDI session or connection requires the next pair of consecutive sequential ports (such as 5006 and 5007, 5008 and 5009, and so on)
```
# open up 2 base channels (5004,05) and (5 additional connections)
sudo firewall-cmd --add-port=5004-5015/udp # To make permanent include: --permanent
```

See /etc/rtpmidi/default.ini for the default rtpmidid setup


Setup three loop back ports:
```
# 1. Disable Wifi.

# 2. Enable Static IP

sudo systemctl stop rtpmidid
sudo modprobe -r snd-seq-dummy        # unload the current 
sudo modprobe snd-seq-dummy ports=3   # create three loop-back port
aconnect -1                           # look at the current setup - the MIDI throughs should be available
sudo systemctl start rtpmidid
aconnect -1                           # look at the current setup - the RTP MIDI interface should be available
```

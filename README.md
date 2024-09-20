# __caw__ audio processing program

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


## Build

Construct the project
```
cd ~/src
git clone http://gitea.larke.org/kevin/caw.git
cd caw/src
git clone http://gitea.larke.org/kevin/libcw.git
cd libcw
git switch poly
```

Debug Build
```
cd caw/build/linux/debug
./build.sh # Generates and runs caw/configure
make
make install # installs into caw/build/linux/debug/bin
```

Release Build
```
cd caw/build/linux/release
./build.sh  # generates and runs caw/configure
make
make install # installs into caw/build/linux/release/bin
```



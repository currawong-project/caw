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

# __caw__ audio processing program

__caw__ is an experimental, interactive, real-time audio processing environment
configured with a declarative language for describing data flow programs.
It is based on the [libcw](https://gitea.currawongproject.org/cml/libcw).


The best introduction to the configuration language is [__caw__ by Example](https://gitea.currawongproject.org/cml/caw/src/branch/master/examples/examples.md) This tutorial steps through the basic language constructs and theory of operation.

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

Get the project code
```
cd ~/src
git clone http://gitea.currawongproject.org/cml/caw.git
cd caw/src
git clone http://gitea.currawongproject.org/cml/libcw.git
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

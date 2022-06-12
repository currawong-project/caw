Template App


# GDB Setup:

    set env LD_LIBRARY_PATH /home/kevin/sdk/libwebsockets/build/out/lib
    r ~/src/cwtest/src/cwtest/cfg/main.cfg mtx


# Valgrind setup

    export LD_LIBRARY_PATH=~/sdk/libwebsockets/build/out/lib
    valgrind --leak-check=yes --log-file=vg0.txt ./cwtest  ~/src/cwtest/src/cwtest/cfg/main.cfg mtx


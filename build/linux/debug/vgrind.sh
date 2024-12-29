# export LD_LIBRARY_PATH=~/sdk/libwebsockets/build/out/lib
# valgrind --leak-check=yes --log-file=vg0.txt --track-origins=yes bin/caw ui ~/src/caw/src/caw/ui/ui_caw.cfg $1
# valgrind --leak-check=yes --log-file=vg0.txt --track-origins=yes bin/caw test ~/src/cwtest/src/cwtest/cfg/main.cfg /flow_value all echo
valgrind --leak-check=yes --log-file=vg0.txt --track-origins=yes bin/caw ui ~/src/caw/src/caw/perf/perf_caw.cfg


# run server
../../scripts/nsys_profile.sh ./pdc_server.exe

# run test
OMP_NUM_THREADS=8 nsys profile --sample=none --cpuctxsw=none --stats=false --backtrace=none --trace=nvtx --gpuctxsw=false mpirun -np 2 ./vpicio_compress_obj; ../../scripts/nsys_profile.sh ./close_server


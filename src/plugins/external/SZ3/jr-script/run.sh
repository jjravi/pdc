#!/bin/bash

# echo "Bash version ${BASH_VERSION}..."

# nvidia-smi --query-gpu=timestamp,name,pci.bus_id,driver_version,pstate,pcie.link.gen.max,pcie.link.gen.current,temperature.gpu,utilization.gpu,utilization.memory,memory.total,memory.free,memory.used --format=csv -lms 10 > results-file6.csv

# for i in 1 {2..10..2}
for i in 1 {2..22..2}
do
  echo "running $i times"
  timeout 10 ./cpu_mon > cpu_stats$i.csv &
  sleep 1
  mpirun -np $i ./sz3 -f -M PSNR -S 80 -z -i /dev/shm/temperature.f32 -3 512 512 512  > app_stats$i.txt
  # mpirun -np $i ./sz3 -f -M PSNR -S 80 -z -i /dev/shm/QRAINf48.log10.bin.f32 -3 100 500 500 > app_stats$i.txt
  # mpirun -np $i ./sz3 -f -M PSNR -S 80 -z -i /dev/shm/QCLOUDf48.log10.bin.f32 -3 100 500 500  > app_stats$i.txt
  # mpirun -np $i ./sz3 -f -M PSNR -S 80 -z -i /dev/shm/QVAPORf48.bin.f32 -3 100 500 500  > app_stats$i.txt
  # mpirun -np $i ./sz3 -f -M PSNR -S 80 -z -i /dev/shm/einspline_115_69_69_288.f32 -4 115 69 69 288 > app_stats$i.txt
  while pgrep -lf "sz3" > /dev/null; do
    sleep 1
  done
  while pgrep -lf "cpu_mon" > /dev/null; do
    sleep 1
  done
done

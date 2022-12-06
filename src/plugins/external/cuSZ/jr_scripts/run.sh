#!/bin/bash

# echo "Bash version ${BASH_VERSION}..."

# nvidia-smi --query-gpu=timestamp,name,pci.bus_id,driver_version,pstate,pcie.link.gen.max,pcie.link.gen.current,temperature.gpu,utilization.gpu,utilization.memory,memory.total,memory.free,memory.used --format=csv -lms 10 > results-file6.csv

for i in 1 {18..22..2}
do
  echo "running $i times"
  timeout 10 nvidia-smi --query-gpu=timestamp,name,temperature.gpu,utilization.gpu,utilization.memory --format=csv -lms 10 > gpu_stats$i.csv &
  sleep 1
  mpirun -np $i ./cusz -m abs -t f32 -e 4e-6 -z -i /dev/shm/QVAPORf48.bin.f32 -l 100x500x500 --report time > app_stats$i.txt
  while pgrep -lf "cusz" > /dev/null; do
    sleep 1
  done
  while pgrep -lf "nvidia-smi" > /dev/null; do
    sleep 1
  done
done

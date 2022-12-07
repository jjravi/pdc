#!/bin/bash

# echo "Bash version ${BASH_VERSION}..."

# nvidia-smi --query-gpu=timestamp,name,pci.bus_id,driver_version,pstate,pcie.link.gen.max,pcie.link.gen.current,temperature.gpu,utilization.gpu,utilization.memory,memory.total,memory.free,memory.used --format=csv -lms 10 > results-file6.csv

# for i in 1 {2..22..2}
for i in 1 {2..10..2}
do
  echo "running $i times"
  timeout 10 nvidia-smi -i 0 --query-gpu=timestamp,name,temperature.gpu,utilization.gpu,utilization.memory --format=csv -lms 10 > gpu_stats$i.csv &
  sleep 1
  # CUDA_VISIBLE_DEVICES=0 mpirun -np $i ./cusz -t f32 -m r2r -e 6e-3 -z -i /dev/shm/temperature.f32 -l 512x512x512 --report time > app_stats$i.txt
  # CUDA_VISIBLE_DEVICES=0 mpirun -np $i ./cusz -t f32 -m r2r -e 6e-3 -z -i /dev/shm/QRAINf48.log10.bin.f32 -l 100x500x500 --report time > app_stats$i.txt
  # CUDA_VISIBLE_DEVICES=0 mpirun -np $i ./cusz -t f32 -m r2r -e 1e0 -z -i /dev/shm/QCLOUDf48.log10.bin.f32 -l 100x500x500 --report time > app_stats$i.txt
  # CUDA_VISIBLE_DEVICES=0 mpirun -np $i ./cusz -m abs -t f32 -e 4e-6 -z -i /dev/shm/QVAPORf48.bin.f32 -l 100x500x500 --report time > app_stats$i.txt
  CUDA_VISIBLE_DEVICES=0 mpirun -np $i ./cusz -m abs -t f32 -e 2e-1 -z -i /dev/shm/einspline_115_69_69_288.f32 -l 115x69x69x288 --report time > app_stats$i.txt
  while pgrep -lf "cusz" > /dev/null; do
    sleep 1
  done
  while pgrep -lf "nvidia-smi" > /dev/null; do
    sleep 1
  done
done

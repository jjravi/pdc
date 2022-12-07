#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

static unsigned long long *p_array;

static unsigned long long cpu_last_idle;
static unsigned long long cpu_last_total;
float cpu_util_g = 0;
static FILE *fp;

// Description:  Returns a float value of CPU usage percentage from given array[4] = {USER_PROC, NICE_PROC, SYSTEM_PROC, IDLE_PROC}
static float get_cpu_percentage(unsigned long long *a1, unsigned long long *a2)
{
  //   echo "CPU usage at $cpu_usage%" 
  unsigned long long delta_idle = a1[3] - a2[3];
  unsigned long long delta_used = (a1[0] - a2[0]) + (a1[1] - a2[1]) + (a1[2] - a2[2]);
  return ((float)delta_used / (float)(delta_used + delta_idle)) * 100;
}

void sample_cpu_util()
{
  // formula
  //   cpu_now=($(head -n1 /proc/stat)) 
  //   cpu_total=SUM(cpu_now)
  //   cpu_delta=cpu_total - cpu_last_total
  //   cpu_idle=cpu_now[4]- cpu_last_idle
  //   cpu_used=cpu_delta - cpu_idle
  //   cpu_usage=100 * cpu_used / cpu_delta
  //   
  //   # Keep this as last for our next read 
  //   cpu_last_idle=cpu_now[4]
  //   cpu_last_total=cpu_total 

  fp = fopen("/proc/stat", "r");
  fscanf(fp, "%*s %llu %llu %llu %llu ", p_array+0, p_array+1, p_array+2, p_array+3);

  unsigned long long cpu_total = p_array[0] + p_array[1] + p_array[2] + p_array[3];
  unsigned long long cpu_delta = cpu_total - cpu_last_total;
  unsigned long long cpu_idle = p_array[3] - cpu_last_idle;
  unsigned long long cpu_used = cpu_delta - cpu_idle;
  cpu_last_idle = p_array[3];
  cpu_last_total = cpu_total;
  cpu_util_g = (float)cpu_used / cpu_delta;
  // printf("CPU: %.2f\n", 100 * ((float)cpu_used/cpu_delta));
  fclose(fp);
}

void cpu_monitor_start()
{
  p_array = (unsigned long long *)malloc(4*sizeof(unsigned long long));
}

void cpu_monitor_stop()
{
  free(p_array);
}

int main()
{
  cpu_monitor_start();
  int count = 0;
  while(1)
  {
    sample_cpu_util();
    count++;
    usleep(1000); // 1ms

    if (count == 10)
    {
      printf("cpu_util=%.2f%\n", cpu_util_g);
      count = 0;
    }
  }

  cpu_monitor_stop();
  return 0;
}

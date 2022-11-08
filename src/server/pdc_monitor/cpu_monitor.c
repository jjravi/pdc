#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

// #define PROCSTAT "/proc/stat"
// 
// static int num_of_cpus;
// 
// // Description:  Reads /proc/stat file and returns an array[cpu_num][process_type] of process count
// static unsigned long long **read_cpu()
// {
//   FILE *fp;
//   unsigned long long **array;
//   char buffer[1024];
//   int i, j;
//   unsigned long long ignore[6];
// 
//   array = (unsigned long long **)malloc((num_of_cpus + 1) * sizeof(unsigned long long));
// 
//   fp = fopen(PROCSTAT, "r");
// 
//   for(i = 0; i<num_of_cpus + 1; i++)
//   {
//     array[i] = (unsigned long long *)malloc(4 * sizeof(unsigned long long));
//     fscanf(fp, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu ",
//       &array[i][0], &array[i][1], &array[i][2], &array[i][3],
//       &ignore[0], &ignore[1], &ignore[2], &ignore[3], &ignore[4], &ignore[5]);
//   }
//   fclose(fp);
//   return array;
// }
// 
// // Description:  Returns a float value of CPU usage percentage from given array[4] = {USER_PROC, NICE_PROC, SYSTEM_PROC, IDLE_PROC}
// static float get_cpu_percentage(unsigned long long *a1, unsigned long long *a2)
// {
//   return
//     (
//       (float)((a1[0] - a2[0]) + (a1[1] - a2[1]) + (a1[2] - a2[2])) /
//       (float)((a1[0] - a2[0]) + (a1[1] - a2[1]) + (a1[2] - a2[2]) + (a1[3] - a2[3]))
//     ) * 100;
// }
// 
// void print_cpu_usage()
// {
//   float *percentage;
// 
//   printf("CPU");
//   for(int i = 1; i <= num_of_cpus; i++)
//   {
//     printf("\tCPU-%d", i);
//   }
//   printf("\n");
// 
//   // while(1)
//   {
//     unsigned long long **p_cpu_array = read_cpu();
//     sleep(1);
//     unsigned long long **n_cpu_array = read_cpu();
// 
//     for(int i = 0; i<=num_of_cpus; i++)
//     {
//       printf("%.2f\t", get_cpu_percentage(n_cpu_array[i], p_cpu_array[i]));
//     }
//     printf("\n");
// 
//     for(int i = 0; i < num_of_cpus; i++)
//     {
//       free(p_cpu_array[i]);
//       free(n_cpu_array[i]);
//     }
//   }
//   return EXIT_SUCCESS;
// }
// 
// static int get_number_of_cpu_cores()
// {
//   FILE *fp = fopen(PROCSTAT, "r");
// 
//   int n = 0;
//   char line[100];
// 
//   while(!feof(fp))
//   {
//     fgets(line, 100, fp);
//     if(line[0] == 'c' && line[1] == 'p' && line[2] == 'u') n++;
//   }
//   fclose(fp);
// 
//   return n - 1;
// }
// 
// void cpu_monitor_start()
// {
//   num_of_cpus = get_number_of_cpu_cores();
// }
// 
// void cpu_monitor_stop()
// {
//   
// }

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


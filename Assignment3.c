#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

//EACH RQ HAS
// in pointer, out pointer, # processes in queue, size of buffer
// each consumer reads from RQ0 until it is empty, then moves to next queues

//STEPS
// create all 20 processes from input file
// sort them all into RQx based on priority
// sort each RQ by priority
// execute each process based on sched_policy

//ROUND ROBIN
// if EXEC_TIME - TIME_SLICE > 0, insert process back into queue with updated remaining_time

time_t t;

enum sched_policy {
  RR,
  FIFO,
  NORMAL
};

struct data {
  pid_t pid;
  int static_priority;
  int dynamic_priority;
  int remain_time;
  int time_slice;
  int accu_time_slice;
  int last_cpu;
  enum sched_policy policy;
};

struct rq {
  struct data queue[20];
  int in;
  int out;
  int count;
};

struct local_thread_queue {
  struct rq rq[3];
};

//global var declarations
pthread_t consumer_threads[4];
void *producer_thread_function(void *arg);
void *consumer_thread_function(void *arg);
struct data processes[20];
struct local_thread_queue local_thread_queue[4];

int min(int x, int y) {
  return x > y ? y : x;
}

int max(int x, int y) {
  return x > y ? x : y;
}

void swap(struct data* process_one, struct data* process_two)
{
    struct data temp = *process_one;
    *process_one = *process_two;
    *process_two = temp;
}

// A function to implement bubble sort
void sort(struct data arr[], int n)
{
    int i, j;
    for (i = 0; i < n - 1; i++)

        // Last i elements are already in place
        for (j = 0; j < n - i - 1; j++)
            if (arr[j].static_priority > arr[j + 1].static_priority)
                swap(&arr[j], &arr[j + 1]);
}

int main() {
    FILE* file = fopen("input.txt", "r");
    srand((unsigned) time(&t));
    if (file == NULL) {
      printf("no such file.");
      exit(EXIT_FAILURE);
    }

    struct timeval start, end;
    int res;
    pthread_t consumer_threads[4];

    int max_priority;
    int min_priority;
    struct sched_param scheduling_value;

    //initializing all queues
    struct rq one_rq0 = { .in = 0, .out =0, .count =0};
    struct rq one_rq1 = {.in = 0, .out =0, .count =0};
    struct rq one_rq2 = {.in = 0, .out =0, .count =0};

    struct rq two_rq0 = { .in = 0, .out =0, .count =0};
    struct rq two_rq1 = { .in = 0, .out =0, .count =0};
    struct rq two_rq2 = { .in = 0, .out =0, .count =0};

    struct rq three_rq0 = { .in = 0, .out =0, .count =0};
    struct rq three_rq1 = { .in = 0, .out =0, .count =0};
    struct rq three_rq2 = { .in = 0, .out =0, .count =0};

    struct rq four_rq0 = { .in = 0, .out =0, .count =0};
    struct rq four_rq1 = { .in = 0, .out =0, .count =0};
    struct rq four_rq2 = { .in = 0, .out =0, .count =0};

    struct local_thread_queue l0 = { .rq[0] = one_rq0, .rq[1] = one_rq1, .rq[2] = one_rq2};
    struct local_thread_queue l1 = { .rq[0] = two_rq0, .rq[1] = two_rq1, .rq[2] = two_rq2};
    struct local_thread_queue l2 = { .rq[0] =three_rq0, .rq[1] = three_rq1, .rq[2] = three_rq2};
    struct local_thread_queue l3 = { .rq[0] =four_rq0, .rq[1] = four_rq1, .rq[2] = four_rq2};

    local_thread_queue[0] = l0;
    local_thread_queue[1] = l1;
    local_thread_queue[2] = l2;
    local_thread_queue[3] = l3;

    char buff[100];
    char type[10];
    int priority;
    int exec_time;
    int pid;
    size_t size;
    char c[3];
    int num_processes = 0;
    void *thread_result;

    //initialising # of processes in each queue to be 0
    //struct local_thread_process_count process_count_by_queue[4];
    //getting number of lines in file/number of processes
    int ch = 0;
    while(!feof(file)) {
      ch = fgetc(file);

      if(ch == '\n') {
        num_processes++;
      }
    }

    //rewinding file pointer is necessary to read from file again
    rewind(file);

    int processes_per_thread = num_processes / 4;
    printf("Each CPU recieves: %d\n", processes_per_thread);

    int count = 0;

    //reading from file until EOF and instantiating new "processes"
    while (count < num_processes && fscanf(file, "%s %d %d %d", type, &priority, &exec_time, &pid) != EOF) {
      struct data new_process;
      if (strcmp(type, "FIFO") == 0) {
        new_process.policy = FIFO;
      } else if (strcmp(type, "RR") == 0) {
        new_process.policy = RR;
      } else {
        new_process.policy = NORMAL;
      }

      new_process.static_priority = priority;
      new_process.remain_time = exec_time;
      new_process.pid = pid;

      if (priority < 120) {
        new_process.time_slice = (140 - priority) * 20;
      } else {
        new_process.time_slice = (140 - priority) * 5;
      }

      //seperating into priority queues
      if (priority >= 0 && priority < 100) {

        //count / processes_per_thread = current thread that should be populated (even distribution)
        local_thread_queue[count / processes_per_thread].rq[0].queue[local_thread_queue[count / processes_per_thread].rq[0].count] = new_process;
        local_thread_queue[count / processes_per_thread].rq[0].count++;

      } else if (priority >= 100 && priority < 130) {

        local_thread_queue[count / processes_per_thread].rq[1].queue[local_thread_queue[count / processes_per_thread].rq[1].count] = new_process;
        local_thread_queue[count / processes_per_thread].rq[1].count++;

      } else if (priority >= 130 && priority < 140) {

        local_thread_queue[count / processes_per_thread].rq[2].queue[local_thread_queue[count / processes_per_thread].rq[2].count] = new_process;
        local_thread_queue[count / processes_per_thread].rq[2].count++;

      }
      count++;
    }

    //sort each queue by priority lowest to highest value (highest priority = lowest value)
    for (int i = 0; i < 4; i++) {
      sort(local_thread_queue[i].rq[0].queue, local_thread_queue[i].rq[0].count);
      sort(local_thread_queue[i].rq[1].queue, local_thread_queue[i].rq[1].count);
      sort(local_thread_queue[i].rq[2].queue, local_thread_queue[i].rq[2].count);
    }

    for (int k = 0; k < 4; k++) {
      printf("---------CPU %d---------\n", k);
      for (int j = 0; j < 3; j++) {
        printf("    RQ %d\n", j);
        for (int i = 0; i < local_thread_queue[k].rq[j].count; i++) {
          printf("    PID %d Assigned CPU: %d in Queue: rq%d\n", local_thread_queue[k].rq[j].queue[i].pid, k, j);
        }
        printf("\n");
      }
  }

    for (int i = 0; i < 4; i++) {
      local_thread_queue[i].rq[0].in = local_thread_queue[i].rq[0].count + 1;
      local_thread_queue[i].rq[1].in = local_thread_queue[i].rq[1].count + 1;
      local_thread_queue[i].rq[2].in = local_thread_queue[i].rq[2].count + 1;
    }

    //consumer thread creation to simulate CPU
    for(int consumer = 0; consumer < 4; consumer++) {
        res = pthread_create(&(consumer_threads[consumer]), NULL, consumer_thread_function, (void *) &consumer);
        if (res != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
        sleep(1);
    }

    exit(EXIT_SUCCESS);
}

void fifo_scheduling(struct local_thread_queue *current, int queue, int task, int *processes_finished) {
  printf("FIFO SCHEDULING: rq%d task %d. RUNTIME: %d \n\n", queue, task, current->rq[queue].queue[task].remain_time);
  usleep(current->rq[queue].queue[task].remain_time);
  current->rq[queue].queue[task].remain_time = 0;
  //fifo always runs to completion
  *processes_finished += 1;
}

void rr_scheduling(struct local_thread_queue *current, int queue, int task, int *processes_finished) {
  printf("ROUND ROBIN SCHEDULING: rq%d task %d. TIME_SLICE: %d ", queue, task, current->rq[queue].queue[task].time_slice);
  usleep(current->rq[queue].queue[task].remain_time);

  //need to check if time_slice is more than remaining execution time
  current->rq[queue].queue[task].remain_time = current->rq[queue].queue[task].remain_time < current->rq[queue].queue[task].time_slice ? 0
    : current->rq[queue].queue[task].remain_time - current->rq[queue].queue[task].time_slice;
  printf("REMAINING TIME %d \n\n", current->rq[queue].queue[task].remain_time);

  //if process is finished, mark as done
  if (current->rq[queue].queue[task].remain_time == 0) {
    *processes_finished += 1;
  }
}

void normal_scheduling(struct local_thread_queue *current, int queue, int task, int *processes_finished) {
  int random = rand() % 20;
  int exec_time_for_iteration = (random + 1) * 10;

  //determining bonus
  int bonus = exec_time_for_iteration < current->rq[queue].queue[task].time_slice ? 100 : 5;

  //determining new priority
  int new_priority = max(110, min(current->rq[queue].queue[task].static_priority - bonus + 5, 139));

  //uncomment to see sorting work
  //new_priority = 10 - task;

  printf("NORMAL SCHEDULING: rq%d task %d. EXEC_TIME ALLOWED FOR THIS ITERATION: %d \n", queue, task, exec_time_for_iteration);
  printf("time_slice: %d\n", current->rq[queue].queue[task].time_slice);
  printf("OLD PRIORITY: %d NEW PRIORITY: %d BONUS: %d ", current->rq[queue].queue[task].static_priority, new_priority, bonus);
  if (bonus == 10) {
    printf("(bonus of net -5 applied due to exec_time generated < time_slice alloted) \n");
  }

  usleep(exec_time_for_iteration);
  current->rq[queue].queue[task].static_priority = new_priority;
  //need to check if time_slice is more than remaining execution time
  current->rq[queue].queue[task].remain_time = current->rq[queue].queue[task].remain_time
    < exec_time_for_iteration ? 0 : current->rq[queue].queue[task].remain_time - exec_time_for_iteration;

  //if process is finished, mark as done
  printf("Remaining time: %d\n", current->rq[queue].queue[task].remain_time);
  if (current->rq[queue].queue[task].remain_time == 0) {
    *processes_finished += 1;
  }

  //assign new time slice
  if (new_priority < 120) {
    current->rq[queue].queue[task].time_slice = (140 - new_priority) * 20;
  } else {
    current->rq[queue].queue[task].time_slice = (140 - new_priority) * 5;
  }
  printf("New time_slice: %d\n", current->rq[queue].queue[task].time_slice);

  printf("PID before sort %d\n", current->rq[queue].queue[task].pid);
  sort(current->rq[queue].queue, current->rq[queue].count);
  printf("PID after sort %d\n\n", current->rq[queue].queue[task].pid);
}

void *consumer_thread_function(void *arg) {
    int i = *(int *)arg;
    printf("-------------------\n");
    printf("Current CPU: %d\n", i);

    struct local_thread_queue current_cpu = local_thread_queue[i];
    struct data current_task;
    int processes_finished = 0;

    //begin scheduling
    //each priority queue one by one
    for (int j = 0; j < 3; j++) {
      printf("Number of tasks in CPU %d in rq[%d] %d\n",i,j, current_cpu.rq[j].count);
      processes_finished = 0;
      //make sure all tasks in queue finish before moving on to the next queue
      while (processes_finished <= current_cpu.rq[j].count && current_cpu.rq[j].count != 0) {
        //move through the queue one by one and determine how to execute task
        while (current_cpu.rq[j].out < current_cpu.rq[j].count) {
          current_task = current_cpu.rq[j].queue[current_cpu.rq[j].out];

          //don't run the tasks if it's over
          if (current_task.remain_time > 0) {

          //determine which scheduling process to run
          switch (current_task.policy) {
            case RR:
              printf("CPU %d PID %d Scheduling Type: Round Robin\n", i, current_task.pid);
              rr_scheduling(&current_cpu, j, current_cpu.rq[j].out,  &processes_finished);
              break;
            case FIFO:
              printf("CPU %d PID: %d FIFO\n", i, current_task.pid);
              fifo_scheduling(&current_cpu, j, current_cpu.rq[j].out, &processes_finished);
              break;
            case NORMAL:
              printf("CPU %d PID: %d NORMAL\n", i, current_task.pid);
              normal_scheduling(&current_cpu, j, current_cpu.rq[j].out, &processes_finished);
              break;
            }
          }
          //increment out pointer
          current_cpu.rq[j].out++;
        }
        //after each pass through the queue, reset the pointer to 0 to run them again if necessary
        current_cpu.rq[j].out = 0;
      }
    }
    return 0;
}

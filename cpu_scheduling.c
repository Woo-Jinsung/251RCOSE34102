#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define PROCESS_COUNT 3
#define IO_COUNT 2
#define ARRIVAL_TIME 2
#define BURST_TIME 20
#define IO_BURST_TIME 3
#define TIME_QUANTUM 5
#define MAX_QUEUE_SIZE 100


typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority; // the smaller the number, the higher the priority

    int io_count;
    int io_request_time[IO_COUNT];
    int io_burst_time[IO_COUNT];
    int io_index;

    int ready_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;

    bool in_ready;
    bool in_waiting;
    bool in_running;
    bool completed;
} Process;

typedef struct {
    Process* queue[MAX_QUEUE_SIZE];
    int front;
    int rear;
} Queue;

void randomization(int* arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void generate_sorted_io_request_times(int* target, int burst_time, int io_count) {
    int pool[burst_time - 1];
    for (int i = 0; i < burst_time - 1; i++) {
        pool[i] = i + 1; // 1 to burst_time-1
    }

    randomization(pool, burst_time - 1); // randomize the IO request times

    for (int i = 0; i < io_count; i++) {
        target[i] = pool[i]; // Random IO request time without replacement(1 to burst_time)
    }

    // insertion sort
    for (int i = 1; i < io_count; i++) {
        int key = target[i];
        int j = i - 1;
        while (j >= 0 && target[j] > key) {
            target[j + 1] = target[j];
            j--;
        }
        target[j + 1] = key;
    }
}

Process* Create_Process(int n) {
    Process* p = malloc(n * sizeof(Process));
    srand(time(NULL));

    int arrival_times[ARRIVAL_TIME + 1];
    for (int i = 0; i < ARRIVAL_TIME + 1; i++) {
        arrival_times[i] = i;
    }
    randomization(arrival_times, ARRIVAL_TIME + 1);

    int priority[n];
    for (int i = 0; i < n; i++) {
        priority[i] = i + 1;
    }
    randomization(priority, n);

    for(int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].arrival_time = arrival_times[i]; // Random arrival time without replacement(0 to 20)
        p[i].burst_time = rand() % BURST_TIME + 1; // 1 to 20
        p[i].remaining_time = p[i].burst_time;
        p[i].priority = priority[i]; // Random priority without replacement(1 to n)

        int max_io_count = p[i].burst_time - 1;
        if(max_io_count > IO_COUNT) {
            max_io_count = IO_COUNT;
        }
        p[i].io_count = rand() % (max_io_count + 1); // 0 to max_io_count
        generate_sorted_io_request_times(p[i].io_request_time, p[i].burst_time, p[i].io_count);
        for (int j = 0; j < p[i].io_count; j++) {
            p[i].io_burst_time[j] = rand() % IO_BURST_TIME + 1; // 1 to 5
        }
        p[i].io_index = 0;
        p[i].ready_time = p[i].arrival_time;
        p[i].finish_time = 0;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;

        p[i].in_ready = false;
        p[i].in_waiting = false;
        p[i].in_running = false;
        p[i].completed = false;
    }

    return p;
}

Process* Create_Process_Manually(){
    Process* p = malloc(sizeof(Process));
    printf("\nEnter Process ID: ");
    scanf("%d", &p->pid);
    printf("Enter Arrival Time: ");
    scanf("%d", &p->arrival_time);
    printf("Enter Burst Time: ");
    scanf("%d", &p->burst_time);
    p->remaining_time = p->burst_time;
    printf("Enter Priority (1 is highest): ");
    scanf("%d", &p->priority);

    printf("Enter number of I/O requests (0 to %d): ", p->burst_time - 1);
    scanf("%d", &p->io_count);
    
    for (int i = 0; i < p->io_count; i++) {
        printf("Enter I/O request time %d: ", i + 1);
        scanf("%d", &p->io_request_time[i]);
        printf("Enter I/O burst time %d: ", i + 1);
        scanf("%d", &p->io_burst_time[i]);
    }

    for (int i = 0; i < p->io_count - 1; i++) {
        for (int j = 0; j < p->io_count - 1 - i; j++) {
            if (p->io_request_time[j] > p->io_request_time[j + 1]) {
                int temp_request = p->io_request_time[j];
                p->io_request_time[j] = p->io_request_time[j + 1];
                p->io_request_time[j + 1] = temp_request;

                int temp_burst = p->io_burst_time[j];
                p->io_burst_time[j] = p->io_burst_time[j + 1];
                p->io_burst_time[j + 1] = temp_burst;
            }
        }
    }
    
    p->io_index = 0;
    p->ready_time = p->arrival_time;
    p->finish_time = -1;
    p->waiting_time = 0;
    p->turnaround_time = 0;

    p->in_ready = false;
    p->in_waiting = false;
    p->in_running = false;
    p->completed = false;

    return p;
}

void init_queue(Queue* q) {
    q->front = 0;
    q->rear = 0;
}

int is_empty(Queue* q) {
    return q->front == q->rear;
}

void enqueue(Queue* q, Process* p) {
    if ((q->rear + 1) % MAX_QUEUE_SIZE == q->front) {
        printf("Queue is full\n");
        return;
    }
    q->queue[q->rear] = p;
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
}

Process* dequeue(Queue* q) {
    if (is_empty(q)) {
        printf("Queue is empty\n");
        return NULL;
    }
    Process* p = q->queue[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    return p;
}

void sort_ready_queue_by_remaining_time(Queue* q) {
    int size = (q->rear - q->front + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
    if (size <= 1) {
        return; // No need to sort if the queue has 0 or 1 element
    }

    Process* temp[size];
    int index = 0;
    while (!is_empty(q)) {
        temp[index++] = dequeue(q);
    }

    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            if (temp[i]->remaining_time > temp[j]->remaining_time) {
                Process* swap = temp[i];
                temp[i] = temp[j];
                temp[j] = swap;
            }
        }
    }

    for (int i = 0; i < size; i++) {
        enqueue(q, temp[i]);
    }
    
}

void sort_ready_queue_by_priority(Queue* q) {
    int size = (q->rear - q->front + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
    if (size <= 1) {
        return; // No need to sort if the queue has 0 or 1 element
    }

    Process* temp[size];
    int index = 0;
    while (!is_empty(q)) {
        temp[index++] = dequeue(q);
    }

    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            if (temp[i]->priority > temp[j]->priority) {
                Process* swap = temp[i];
                temp[i] = temp[j];
                temp[j] = swap;
            }
        }
    }

    for (int i = 0; i < size; i++) {
        enqueue(q, temp[i]);
    }
}

void GanttChart(int gantt[], int size) {
    printf("Gantt Chart:\n");
    printf("Time\tProcess ID\n");
    for (int i = 0; i < size; i++) {
        if (gantt[i] != -1) {
            printf("%d\t|  %d  |\n", i, gantt[i]);
        } else {
            printf("%d\t|  -  |\n", i);
        }
    }
    printf("%d\t|  -  |\n", size);
    printf("----------------------------------------------------------------\n");
}

void Evaluation(Process* p, int n) {
    float total_waiting_time = 0;
    float total_turnaround_time = 0;

    for (int i = 0; i < n; i++) {
        total_waiting_time += p[i].waiting_time;
        total_turnaround_time += p[i].turnaround_time;
    }

    printf("\nProcess ID\tWaiting Time\tTurnaround Time\n");

    for (int i = 0; i < n; i++) {
        printf("%d\t\t%d\t\t%d\n", p[i].pid, p[i].waiting_time, p[i].turnaround_time);
    }
    printf("Average Waiting Time: %.2f\n", total_waiting_time / n);
    printf("Average Turnaround Time: %.2f\n\n", total_turnaround_time / n);
}

void FCFS(Process* p, int n) {

    printf("FCFS\n\n");
    Queue ready_queue, waiting_queue;
    init_queue(&ready_queue);
    init_queue(&waiting_queue);

    int time = 0;
    int completed = 0;
    Process* current_process = NULL;
    Process* io_process = NULL;
    int io_remaining_time = 0;
    int gantt_pid[PROCESS_COUNT * BURST_TIME];

    while (completed < n) {

        for (int i = 0; i < n; i++) {
            if (p[i].ready_time <= time && !p[i].completed && !p[i].in_ready && !p[i].in_waiting && !p[i].in_running) {
                enqueue(&ready_queue, &p[i]);
                p[i].in_ready = true;
            }
        }

        if (current_process == NULL && !is_empty(&ready_queue)) {
            current_process = dequeue(&ready_queue);
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = true;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        for (int i = 0; i < n; i++) {
            if (p[i].in_ready) {
                p[i].waiting_time++;
            }
        }

        gantt_pid[time] = current_process ? current_process->pid : -1;

        if (current_process != NULL) {
            int elapsed_time = current_process->burst_time - current_process->remaining_time;
            if (current_process->io_index < current_process->io_count && elapsed_time + 1 == current_process->io_request_time[current_process->io_index]) {
                printf("Process %d I/O request at time %d\n", current_process->pid, time + 1);
                current_process->remaining_time--;
                current_process->in_ready = false;
                current_process->in_waiting = true;
                current_process->in_running = false;
                
                enqueue(&waiting_queue, current_process);
                if (!is_empty(&ready_queue)) {
                    current_process = dequeue(&ready_queue);
                    current_process->in_ready = false;
                    current_process->in_waiting = false;
                    current_process->in_running = true;
                    printf("Process %d started at time %d\n", current_process->pid, time + 1);
                } else {
                    current_process = NULL;
                }
            }
             else {
                current_process->remaining_time--;
                if (current_process->remaining_time == 0) {
                    current_process->finish_time = time + 1;
                    current_process->turnaround_time = current_process->finish_time - current_process->arrival_time;
                    current_process->completed = true;
                    printf("Process %d completed at time %d\n", current_process->pid, time + 1);
                    completed++;
                    if (!is_empty(&ready_queue)) {
                        current_process = dequeue(&ready_queue);
                        current_process->in_ready = false;
                        current_process->in_waiting = false;
                        current_process->in_running = true;
                        printf("Process %d started at time %d\n", current_process->pid, time + 1);
                    } else {
                        current_process = NULL;
                    }
                }
            }         
        }

        time++;

        if (io_process == NULL && !is_empty(&waiting_queue)) {
            io_process = dequeue(&waiting_queue);
            io_remaining_time = io_process->io_burst_time[io_process->io_index];
        }

        if (io_process != NULL) {
            io_remaining_time--;
            if (io_remaining_time == 0) {
                io_process->io_index++;
                io_process->in_ready = false;
                io_process->in_waiting = false;
                io_process->in_running = false;
                io_process->ready_time = time + 1;
                io_process = NULL;
            }
        }
    }

    int end_time = time;

    printf("\nFCFS Evaluation:");
    Evaluation(p, n);

    GanttChart(gantt_pid, end_time);
}


void Nonpreemptive_SJF(Process* p, int n) {

    printf("Nonpreemptive SJF\n\n");
    Queue ready_queue, waiting_queue;
    init_queue(&ready_queue);
    init_queue(&waiting_queue);

    int time = 0;
    int completed = 0;
    Process* current_process = NULL;
    Process* io_process = NULL;
    int io_remaining_time = 0;
    int gantt_pid[PROCESS_COUNT * BURST_TIME];

    while (completed < n) {

        for (int i = 0; i < n; i++) {
            if (p[i].ready_time <= time && !p[i].completed && !p[i].in_ready && !p[i].in_waiting && !p[i].in_running) {
                enqueue(&ready_queue, &p[i]);
                p[i].in_ready = true;
            }
        }

        sort_ready_queue_by_remaining_time(&ready_queue);

        if (current_process == NULL && !is_empty(&ready_queue)) {
            current_process = dequeue(&ready_queue);
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = true;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        for (int i = 0; i < n; i++) {
            if (p[i].in_ready) {
                p[i].waiting_time++;
            }
        }

        gantt_pid[time] = current_process ? current_process->pid : -1;

        if (current_process != NULL) {
            int elapsed_time = current_process->burst_time - current_process->remaining_time;
            if (current_process->io_index < current_process->io_count && elapsed_time + 1 == current_process->io_request_time[current_process->io_index]) {
                printf("Process %d I/O request at time %d\n", current_process->pid, time + 1);
                current_process->remaining_time--;
                current_process->in_ready = false;
                current_process->in_waiting = true;
                current_process->in_running = false;
                enqueue(&waiting_queue, current_process);
                if (!is_empty(&ready_queue)) {
                    current_process = dequeue(&ready_queue);
                    current_process->in_ready = false;
                    current_process->in_waiting = false;
                    current_process->in_running = true;
                    printf("Process %d started at time %d\n", current_process->pid, time + 1);
                } else {
                    current_process = NULL;
                }
            } else {
                current_process->remaining_time--;
                if (current_process->remaining_time == 0) {
                    current_process->finish_time = time + 1;
                    current_process->turnaround_time = current_process->finish_time - current_process->arrival_time;
                    current_process->completed = true;
                    printf("Process %d completed at time %d\n", current_process->pid, time + 1);
                    completed++;
                    if (!is_empty(&ready_queue)) {
                        current_process = dequeue(&ready_queue);
                        current_process->in_ready = false;
                        current_process->in_waiting = false;
                        current_process->in_running = true;
                        printf("Process %d started at time %d\n", current_process->pid, time + 1);
                    } else {
                        current_process = NULL;
                    }
                }
            }
        }

        time++;

        if (io_process == NULL && !is_empty(&waiting_queue)) {
            io_process = dequeue(&waiting_queue);
            io_remaining_time = io_process->io_burst_time[io_process->io_index];
        }

        if (io_process != NULL) {
            io_remaining_time--;
            if (io_remaining_time == 0) {
                io_process->io_index++;
                io_process->in_ready = false;
                io_process->in_waiting = false;
                io_process->in_running = false;
                io_process->ready_time = time + 1;
                io_process = NULL;
            }
        }

    }

    int end_time = time;

    printf("\nNonpreemptive SJF Evaluation:");
    Evaluation(p, n);

    GanttChart(gantt_pid, end_time);
}


void Nonpreemptive_Priority(Process* p, int n) {

    printf("Nonpreemptive Priority\n\n");
    Queue ready_queue, waiting_queue;
    init_queue(&ready_queue);
    init_queue(&waiting_queue);

    int time = 0;
    int completed = 0;
    Process* current_process = NULL;
    Process* io_process = NULL;
    int io_remaining_time = 0;
    int gantt_pid[PROCESS_COUNT * BURST_TIME];

    while (completed < n) {

        for (int i = 0; i < n; i++) {
            if (p[i].ready_time <= time && !p[i].completed && !p[i].in_ready && !p[i].in_waiting && !p[i].in_running) {
                enqueue(&ready_queue, &p[i]);
                p[i].in_ready = true;
            }
        }

        sort_ready_queue_by_priority(&ready_queue);

        if (current_process == NULL && !is_empty(&ready_queue)) {
            current_process = dequeue(&ready_queue);
            current_process->in_running = true;
            current_process->in_ready = false;
            current_process->in_waiting = false;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        for (int i = 0; i < n; i++) {
            if (p[i].in_ready) {
                p[i].waiting_time++;
            }
        }

        gantt_pid[time] = current_process ? current_process->pid : -1;

        if (current_process != NULL) {
            int elapsed_time = current_process->burst_time - current_process->remaining_time;
            if (current_process->io_index < current_process->io_count && elapsed_time + 1 == current_process->io_request_time[current_process->io_index]) {
                printf("Process %d I/O request at time %d\n", current_process->pid, time + 1);
                current_process->remaining_time--;
                current_process->in_running = false;
                current_process->in_waiting = true;
                current_process->in_ready = false;
                enqueue(&waiting_queue, current_process);
                if (!is_empty(&ready_queue)) {
                    current_process = dequeue(&ready_queue);
                    current_process->in_running = true;
                    current_process->in_waiting = false;
                    current_process->in_ready = false;
                    printf("Process %d started at time %d\n", current_process->pid, time + 1);
                } else {
                    current_process = NULL;
                }
            } else {
                current_process->remaining_time--;
                if (current_process->remaining_time == 0) {
                    current_process->finish_time = time + 1;
                    current_process->turnaround_time = current_process->finish_time - current_process->arrival_time;
                    current_process->completed = true;
                    printf("Process %d completed at time %d\n", current_process->pid, time + 1);
                    completed++;
                    if (!is_empty(&ready_queue)) {
                        current_process = dequeue(&ready_queue);
                        current_process->in_running = true;
                        current_process->in_waiting = false;
                        current_process->in_ready = false;
                        printf("Process %d started at time %d\n", current_process->pid, time + 1);
                    } else {
                        current_process = NULL;
                    }
                }
            }
        }

        time++;

        if (io_process == NULL && !is_empty(&waiting_queue)) {
            io_process = dequeue(&waiting_queue);
            io_remaining_time = io_process->io_burst_time[io_process->io_index];
        }

        if (io_process != NULL) {
            io_remaining_time--;
            if (io_remaining_time == 0) {
                io_process->io_index++;
                io_process->in_ready = false;
                io_process->in_waiting = false;
                io_process->in_running = false;
                io_process->ready_time = time + 1;
                io_process = NULL;
            }
        }

    }

    int end_time = time;

    printf("\nNonpreemptive Priority Evaluation:");
    Evaluation(p, n);

    GanttChart(gantt_pid, end_time);
}


void RR(Process* p, int n) {

    printf("Round Robin (RR)\n\n");
    Queue ready_queue, waiting_queue;
    init_queue(&ready_queue);
    init_queue(&waiting_queue);

    int time = 0;
    int completed = 0;
    Process* current_process = NULL;
    Process* io_process = NULL;
    int io_remaining_time = 0;
    int time_distance = TIME_QUANTUM;
    int gantt_pid[PROCESS_COUNT * BURST_TIME];

    while (completed < n) {

        for (int i = 0; i < n; i++) {
            if (p[i].ready_time <= time && !p[i].completed && !p[i].in_ready && !p[i].in_waiting && !p[i].in_running) {
                enqueue(&ready_queue, &p[i]);
                p[i].in_ready = true;
            }
        }

        if (current_process == NULL && !is_empty(&ready_queue)) {
            current_process = dequeue(&ready_queue);
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = true;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        for (int i = 0; i < n; i++) {
            if (p[i].in_ready) {
                p[i].waiting_time++;
            }
        }

        gantt_pid[time] = current_process ? current_process->pid : -1;

        if (current_process != NULL) {
            int elapsed_time = current_process->burst_time - current_process->remaining_time;
            if (current_process->io_index < current_process->io_count && elapsed_time + 1 == current_process->io_request_time[current_process->io_index]) {
                printf("Process %d I/O request at time %d\n", current_process->pid, time + 1);
                time_distance = TIME_QUANTUM;
                current_process->remaining_time--;
                current_process->in_ready = false;
                current_process->in_waiting = true;
                current_process->in_running = false;
                enqueue(&waiting_queue, current_process);
                if (!is_empty(&ready_queue)) {
                    current_process = dequeue(&ready_queue);
                    current_process->in_ready = false;
                    current_process->in_waiting = false;
                    current_process->in_running = true;
                    printf("Process %d started at time %d\n", current_process->pid, time + 1);
                } else {
                    current_process = NULL;
                }
            } else {
                current_process->remaining_time--;
                time_distance--;
                if (current_process->remaining_time == 0) {
                    time_distance = TIME_QUANTUM;
                    current_process->finish_time = time + 1;
                    current_process->turnaround_time = current_process->finish_time - current_process->arrival_time;
                    current_process->completed = true;
                    printf("Process %d completed at time %d\n", current_process->pid, time + 1);
                    completed++;
                    if (!is_empty(&ready_queue)) {
                        current_process = dequeue(&ready_queue);
                        current_process->in_ready = false;
                        current_process->in_waiting = false;
                        current_process->in_running = true;
                        printf("Process %d started at time %d\n", current_process->pid, time + 1);
                    } else {
                        current_process = NULL;
                    }
                }
                if (time_distance == 0) {
                    current_process->in_ready = false;
                    current_process->in_waiting = false;
                    current_process->in_running = false;
                    time_distance = TIME_QUANTUM;
                    printf("Process %d preempted at time %d\n", current_process->pid, time + 1);
                    if (!is_empty(&ready_queue)) {
                        current_process = dequeue(&ready_queue);
                        current_process->in_ready = false;
                        current_process->in_waiting = false;
                        current_process->in_running = true;
                        printf("Process %d started at time %d\n", current_process->pid, time + 1);
                    } else {
                        current_process = NULL;
                    }
                }
            }
        }

        time++;

        if (io_process == NULL && !is_empty(&waiting_queue)) {
            io_process = dequeue(&waiting_queue);
            io_remaining_time = io_process->io_burst_time[io_process->io_index];
        }

        if (io_process != NULL) {
            io_remaining_time--;
            if (io_remaining_time == 0) {
                io_process->io_index++;
                io_process->in_ready = false;
                io_process->in_waiting = false;
                io_process->in_running = false;
                io_process->ready_time = time + 1;
                io_process = NULL;
            }
        }

    }

    int end_time = time;

    printf("\nRound Robin Evaluation:");
    Evaluation(p, n);

    GanttChart(gantt_pid, end_time);
}


void Preemptive_SJF(Process* p, int n) {

    printf("Preemptive SJF\n\n");
    Queue ready_queue, waiting_queue;
    init_queue(&ready_queue);
    init_queue(&waiting_queue);

    int time = 0;
    int completed = 0;
    Process* current_process = NULL;
    Process* io_process = NULL;
    int io_remaining_time = 0;
    int gantt_pid[PROCESS_COUNT * BURST_TIME];

    while (completed < n) {

        for (int i = 0; i < n; i++) {
            if (p[i].ready_time <= time && !p[i].completed && !p[i].in_ready && !p[i].in_waiting && !p[i].in_running) {
                enqueue(&ready_queue, &p[i]);
                p[i].in_ready = true;
            }
        }

        sort_ready_queue_by_remaining_time(&ready_queue);

        if (current_process == NULL && !is_empty(&ready_queue)) {
            current_process = dequeue(&ready_queue);
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = true;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        for (int i = 0; i < n; i++) {
            if (p[i].in_ready) {
                p[i].waiting_time++;
            }
        }

        if (!is_empty(&ready_queue) && current_process != NULL && current_process->remaining_time > ready_queue.queue[ready_queue.front]->remaining_time) {
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = false;
            printf("Process %d preempted at time %d\n", current_process->pid, time);
            current_process = dequeue(&ready_queue);
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = true;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        gantt_pid[time] = current_process ? current_process->pid : -1;

        if (current_process != NULL) {
            int elapsed_time = current_process->burst_time - current_process->remaining_time;
            if (current_process->io_index < current_process->io_count && elapsed_time + 1 == current_process->io_request_time[current_process->io_index]) {
                printf("Process %d I/O request at time %d\n", current_process->pid, time + 1);
                current_process->remaining_time--;
                current_process->in_ready = false;
                current_process->in_waiting = true;
                current_process->in_running = false;
                enqueue(&waiting_queue, current_process);
                if (!is_empty(&ready_queue)) {
                    current_process = dequeue(&ready_queue);
                    current_process->in_ready = false;
                    current_process->in_waiting = false;
                    current_process->in_running = true;
                    printf("Process %d started at time %d\n", current_process->pid, time + 1);
                } else {
                    current_process = NULL;
                }
            } else {
                current_process->remaining_time--;
                if (current_process->remaining_time == 0) {
                    current_process->finish_time = time + 1;
                    current_process->turnaround_time = current_process->finish_time - current_process->arrival_time;
                    current_process->completed = true;
                    printf("Process %d completed at time %d\n", current_process->pid, time + 1);
                    completed++;
                    if (!is_empty(&ready_queue)) {
                        current_process = dequeue(&ready_queue);
                        current_process->in_ready = false;
                        current_process->in_waiting = false;
                        current_process->in_running = true;
                        printf("Process %d started at time %d\n", current_process->pid, time + 1);
                    } else {
                        current_process = NULL;
                    }
                }
            }
        }

        time++;

        if (io_process == NULL && !is_empty(&waiting_queue)) {
            io_process = dequeue(&waiting_queue);
            io_remaining_time = io_process->io_burst_time[io_process->io_index];
        }

        if (io_process != NULL) {
            io_remaining_time--;
            if (io_remaining_time == 0) {
                io_process->io_index++;
                io_process->in_ready = false;
                io_process->in_waiting = false;
                io_process->in_running = false;
                io_process->ready_time = time + 1;
                io_process = NULL;
            }
        }

        for (int i = 0; i < n; i++) {
            if (p[i].in_ready) {
                p[i].waiting_time++;
            }
        }

    }

    int end_time = time;

    printf("\nPreemptive SJF Evaluation:");
    Evaluation(p, n);

    GanttChart(gantt_pid, end_time);
}


void Preemptive_Priority(Process* p, int n) {

    printf("Preemptive Priority\n\n");
    Queue ready_queue, waiting_queue;
    init_queue(&ready_queue);
    init_queue(&waiting_queue);

    int time = 0;
    int completed = 0;
    Process* current_process = NULL;
    Process* io_process = NULL;
    int io_remaining_time = 0;
    int gantt_pid[PROCESS_COUNT * BURST_TIME];

    while (completed < n) {

        for (int i = 0; i < n; i++) {
            if (p[i].ready_time <= time && !p[i].completed && !p[i].in_ready && !p[i].in_waiting && !p[i].in_running) {
                enqueue(&ready_queue, &p[i]);
                p[i].in_ready = true;
            }
        }

        sort_ready_queue_by_priority(&ready_queue);

        if (current_process == NULL && !is_empty(&ready_queue)) {
            current_process = dequeue(&ready_queue);
            current_process->in_running = true;
            current_process->in_ready = false;
            current_process->in_waiting = false;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        if (!is_empty(&ready_queue) && current_process != NULL && current_process->priority > ready_queue.queue[ready_queue.front]->priority) {
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = false;
            printf("Process %d preempted at time %d\n", current_process->pid, time);
            current_process = dequeue(&ready_queue);
            current_process->in_ready = false;
            current_process->in_waiting = false;
            current_process->in_running = true;
            printf("Process %d started at time %d\n", current_process->pid, time);
        }

        for (int i = 0; i < n; i++) {
            if (p[i].in_ready) {
                p[i].waiting_time++;
            }
        }

        gantt_pid[time] = current_process ? current_process->pid : -1;

        if (current_process != NULL) {
            int elapsed_time = current_process->burst_time - current_process->remaining_time;
            if (current_process->io_index < current_process->io_count && elapsed_time + 1 == current_process->io_request_time[current_process->io_index]) {
                printf("Process %d I/O request at time %d\n", current_process->pid, time + 1);
                current_process->remaining_time--;
                current_process->in_running = false;
                current_process->in_waiting = true;
                current_process->in_ready = false;
                enqueue(&waiting_queue, current_process);
                if (!is_empty(&ready_queue)) {
                    current_process = dequeue(&ready_queue);
                    current_process->in_running = true;
                    current_process->in_waiting = false;
                    current_process->in_ready = false;
                    printf("Process %d started at time %d\n", current_process->pid, time + 1);
                } else {
                    current_process = NULL;
                }
            } else {
                current_process->remaining_time--;
                if (current_process->remaining_time == 0) {
                    current_process->finish_time = time + 1;
                    current_process->turnaround_time = current_process->finish_time - current_process->arrival_time;
                    current_process->completed = true;
                    printf("Process %d completed at time %d\n", current_process->pid, time + 1);
                    completed++;
                    if (!is_empty(&ready_queue)) {
                        current_process = dequeue(&ready_queue);
                        current_process->in_running = true;
                        current_process->in_waiting = false;
                        current_process->in_ready = false;
                        printf("Process %d started at time %d\n", current_process->pid, time + 1);
                    } else {
                        current_process = NULL;
                    }
                }
            }
        }

        time++;

        if (io_process == NULL && !is_empty(&waiting_queue)) {
            io_process = dequeue(&waiting_queue);
            io_remaining_time = io_process->io_burst_time[io_process->io_index];
        }

        if (io_process != NULL) {
            io_remaining_time--;
            if (io_remaining_time == 0) {
                io_process->io_index++;
                io_process->in_ready = false;
                io_process->in_waiting = false;
                io_process->in_running = false;
                io_process->ready_time = time + 1;
                io_process = NULL;
            }
        }

    }

    int end_time = time;

    printf("\nPreemptive Priority Evaluation:");
    Evaluation(p, n);

    GanttChart(gantt_pid, end_time);
}


int main() {

    int choice;
    printf("Automatic Process Generation (1) or Manual Process Creation (2): ");
    scanf_s("%d", &choice);
    printf("\n");
    if (choice == 1) {
        int n = PROCESS_COUNT;
        Process* p = Create_Process(n);
        Process* p1 = malloc(n * sizeof(Process));
        Process* p2 = malloc(n * sizeof(Process));
        Process* p3 = malloc(n * sizeof(Process));
        Process* p4 = malloc(n * sizeof(Process));
        Process* p5 = malloc(n * sizeof(Process));
        Process* p6 = malloc(n * sizeof(Process));
        for (int i = 0; i < n; i++) {
            p1[i] = p[i];
            p2[i] = p[i];
            p3[i] = p[i];
            p4[i] = p[i];
            p5[i] = p[i];
            p6[i] = p[i];
        }

        printf("Process ID\tArrival Time\tBurst Time\tI/O Count\tPriority\n");
        for (int i = 0; i < n; i++) {
            printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\n", p[i].pid, p[i].arrival_time, p[i].burst_time, p[i].io_count, p[i].priority);
        }
        printf("I/O Request Times and Burst Times:\n");
        for (int i = 0; i < n; i++) {
            printf("Process %d: ", p[i].pid);
            for (int j = 0; j < p[i].io_count; j++) {
                printf("(%d, %d) ", p[i].io_request_time[j], p[i].io_burst_time[j]);
            }
            printf("\n");
        }
        printf("----------------------------------------------------------------\n");
        printf("----------------------------------------------------------------\n");

        // Run CPU scheduling algorithm
        FCFS(p1, n);
        Nonpreemptive_SJF(p2, n);
        Nonpreemptive_Priority(p3, n);
        RR(p4, n);
        Preemptive_SJF(p5, n);
        Preemptive_Priority(p6, n);

        free(p);
        free(p1);
        free(p2);
        free(p3);
        free(p4);
        free(p5);
        free(p6);

    } else if (choice == 2) {
        int n;
        printf("Enter number of processes: ");
        scanf_s("%d", &n);
        Process* p = malloc(n * sizeof(Process));
        int index = 0;
        while (true) {
            p[index] = *Create_Process_Manually();
            index++;
            if (index >= n) {
                break;
            }
        }
        Process* p1 = malloc(n * sizeof(Process));
        Process* p2 = malloc(n * sizeof(Process));
        Process* p3 = malloc(n * sizeof(Process));
        Process* p4 = malloc(n * sizeof(Process));
        Process* p5 = malloc(n * sizeof(Process));
        Process* p6 = malloc(n * sizeof(Process));
        for (int i = 0; i < n; i++) {
            p1[i] = p[i];
            p2[i] = p[i];
            p3[i] = p[i];
            p4[i] = p[i];
            p5[i] = p[i];
            p6[i] = p[i];
        }

        printf("Process ID\tArrival Time\tBurst Time\tI/O Count\tPriority\n");
        for (int i = 0; i < n; i++) {
            printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\n", p[i].pid, p[i].arrival_time, p[i].burst_time, p[i].io_count, p[i].priority);
        }
        printf("I/O Request Times and Burst Times:\n");
        for (int i = 0; i < n; i++) {
            printf("Process %d: ", p[i].pid);
            for (int j = 0; j < p[i].io_count; j++) {
                printf("(%d, %d) ", p[i].io_request_time[j], p[i].io_burst_time[j]);
            }
            printf("\n");
        }
        printf("----------------------------------------------------------------\n");
        printf("----------------------------------------------------------------\n");

        // Run CPU scheduling algorithm
        FCFS(p1, n);
        Nonpreemptive_SJF(p2, n);
        Nonpreemptive_Priority(p3, n);
        RR(p4, n);
        Preemptive_SJF(p5, n);
        Preemptive_Priority(p6, n);

        free(p);
        free(p1);
        free(p2);
        free(p3);
        free(p4);
        free(p5);
        free(p6);
    }
}

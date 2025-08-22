#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAXP 100
#define MAXG 10000
/*Features:
1. Implement the following CPU scheduling algorithms: First-Come, First-Served (FCFS), Shortest
Job First (SJF), Round Robin (RR), and Priority Scheduling.
2. Design a user-friendly menu-based interface for the software, allowing the user to select the
desired scheduling algorithm and input parameters.
3. Generate a Gantt chart to visually represent the scheduling sequence and the allocated CPU time
for each process.
4. Calculate and display the average waiting time for the selected scheduling algorithm.
5. Include error handling and input validation to ensure the program handles invalid inputs
gracefully.
6. Implement a comparison module to determine the best CPU scheduling algorithm based on a
given set of processes and their burst times.
    */
typedef struct
{
    int pid;
    int arrival;
    int burst;
    int priority;
    int remaining;
    int start_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int started;
} Process;

typedef struct
{
    int pid;
    int start;
    int end;
} GanttEvent;

typedef struct
{
    GanttEvent events[MAXG];
    int count;
} GanttChart;

typedef struct
{
    double avg_wait;
    double avg_turn;
} Averages;


static int read_int_safe(const char *prompt, int min_allowed, int max_allowed)
{
    int x;
    char line[256];
    while (1)
    {
        if (prompt)
            printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin))
        {
            clearerr(stdin);
            continue;
        }
        char *endptr;
        long val = strtol(line, &endptr, 10);
        if (endptr == line || (*endptr && *endptr != '\n'))
        {
            printf("Invalid input. Enter an integer.\n");
            continue;
        }
        if (val < min_allowed || val > max_allowed)
        {
            printf("Out of range (%d to %d). Try again.\n", min_allowed, max_allowed);
            continue;
        }
        x = (int)val;
        break;
    }
    return x;
}

static void reset_stats(Process p[], int n)
{
    for (int i = 0; i < n; i++)
    {
        p[i].remaining = p[i].burst;
        p[i].start_time = -1;
        p[i].completion_time = -1;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;
        p[i].started = 0;
    }
}

static void copy_processes(Process dst[], const Process src[], int n)
{
    for (int i = 0; i < n; i++)
        dst[i] = src[i];
}

static int all_done(Process p[], int n)
{
    for (int i = 0; i < n; i++)
        if (p[i].remaining > 0)
            return 0;
    return 1;
}

static int next_arrival_time(Process p[], int n, int current_time)
{
    int min_arr = INT_MAX;
    for (int i = 0; i < n; i++)
    {
        if (p[i].remaining > 0 && p[i].arrival > current_time)
        {
            if (p[i].arrival < min_arr)
                min_arr = p[i].arrival;
        }
    }
    return (min_arr == INT_MAX) ? current_time : min_arr;
}

static void push_event(GanttChart *g, int pid, int start, int end)
{
    if (g->count == 0)
    {
        g->events[g->count++] = (GanttEvent){pid, start, end};
        return;
    }
    // Merge consecutive same-PID event
    GanttEvent *last = &g->events[g->count - 1];
    if (last->pid == pid && last->end == start)
    {
        last->end = end;
    }
    else
    {
        if (g->count < MAXG)
            g->events[g->count++] = (GanttEvent){pid, start, end};
    }
}

static void print_gantt(const GanttChart *g)
{
    printf("\nGantt Chart:\n");
    // Top bar
    for (int i = 0; i < g->count; i++)
    {
        int span = g->events[i].end - g->events[i].start;
        if (span < 1)
            span = 1;
        printf("+");
        for (int j = 0; j < span; j++)
            printf("-");
    }
    printf("+\n");

    // Process row
    for (int i = 0; i < g->count; i++)
    {
        int span = g->events[i].end - g->events[i].start;
        if (span < 1)
            span = 1;
        printf("|");
        int pad = span - 1;
        if (pad < 0)
            pad = 0;
        printf("P%d", g->events[i].pid);
        for (int j = 0; j < pad; j++)
            printf(" ");
    }
    printf("|\n");

    // Bottom bar
    for (int i = 0; i < g->count; i++)
    {
        int span = g->events[i].end - g->events[i].start;
        if (span < 1)
            span = 1;
        printf("+");
        for (int j = 0; j < span; j++)
            printf("-");
    }
    printf("+\n");

    // Time markers
    int last_time = -1;
    for (int i = 0; i < g->count; i++)
    {
        int start = g->events[i].start;
        if (i == 0 || start != last_time)
        {
            printf("%d", start);
        }
        else
        {
            printf(" ");
        }
        int span = g->events[i].end - g->events[i].start;
        if (span < 1)
            span = 1;
        for (int j = 0; j < span; j++)
            printf(" ");
        last_time = g->events[i].end;
    }
    printf("%d\n\n", g->events[g->count - 1].end);
}

static Averages compute_and_print_table(Process p[], int n)
{
    double sum_w = 0, sum_t = 0;
    printf("PID\tAT\tBT\tPR\tST\tCT\tTAT\tWT\n");
    for (int i = 0; i < n; i++)
    {
        p[i].turnaround_time = p[i].completion_time - p[i].arrival;
        p[i].waiting_time = p[i].turnaround_time - p[i].burst;
        sum_w += p[i].waiting_time;
        sum_t += p[i].turnaround_time;
        printf("P%-2d\t%-2d\t%-2d\t%-2d\t%-2d\t%-2d\t%-2d\t%-2d\n",
               p[i].pid, p[i].arrival, p[i].burst, p[i].priority,
               p[i].start_time, p[i].completion_time,
               p[i].turnaround_time, p[i].waiting_time);
    }
    Averages avg = {sum_w / n, sum_t / n};
    printf("\nAverage Waiting Time   : %.2f\n", avg.avg_wait);
    printf("Average Turnaround Time: %.2f\n", avg.avg_turn);
    return avg;
}

// FCFS part
static Averages fcfs(Process p[], int n, GanttChart *g)
{
    reset_stats(p, n);
    g->count = 0;
    // Sort by arrival, then PID
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (p[j].arrival < p[i].arrival ||
                (p[j].arrival == p[i].arrival && p[j].pid < p[i].pid))
            {
                Process tmp = p[i];
                p[i] = p[j];
                p[j] = tmp;
            }
        }
    }
    int time = 0;
    for (int i = 0; i < n; i++)
    {
        if (time < p[i].arrival)
            time = p[i].arrival;
        p[i].start_time = time;
        time += p[i].burst;
        p[i].remaining = 0;
        p[i].completion_time = time;
        push_event(g, p[i].pid, p[i].start_time, p[i].completion_time);
    }
    printf("\n=== FCFS ===\n");
    print_gantt(g);
    return compute_and_print_table(p, n);
}

// sjf for non preemptive

static Averages sjf_np(Process p[], int n, GanttChart *g)
{
    reset_stats(p, n);
    g->count = 0;
    int time = 0, completed = 0;
    // Start at earliest arrival if CPU idle initially
    int earliest = INT_MAX;
    for (int i = 0; i < n; i++)
        if (p[i].arrival < earliest)
            earliest = p[i].arrival;
    if (earliest != INT_MAX)
        time = earliest;

    while (completed < n)
    {
        // Pick available with shortest burst
        int idx = -1, best_bt = INT_MAX, best_pid = INT_MAX;
        for (int i = 0; i < n; i++)
        {
            if (p[i].remaining > 0 && p[i].arrival <= time)
            {
                if (p[i].burst < best_bt || (p[i].burst == best_bt && p[i].pid < best_pid))
                {
                    best_bt = p[i].burst;
                    best_pid = p[i].pid;
                    idx = i;
                }
            }
        }
        if (idx == -1)
        {
            // Jump to next arrival
            int next_t = next_arrival_time(p, n, time);
            time = next_t;
            continue;
        }
        p[idx].start_time = time;
        time += p[idx].burst;
        p[idx].remaining = 0;
        p[idx].completion_time = time;
        push_event(g, p[idx].pid, p[idx].start_time, p[idx].completion_time);
        completed++;
    }
    printf("\n=== SJF (Non-Preemptive) ===\n");
    print_gantt(g);
    return compute_and_print_table(p, n);
}

// priority non preemptive

static Averages priority_np(Process p[], int n, GanttChart *g)
{
    reset_stats(p, n);
    g->count = 0;
    int time = 0, completed = 0;
    int earliest = INT_MAX;
    for (int i = 0; i < n; i++)
        if (p[i].arrival < earliest)
            earliest = p[i].arrival;
    if (earliest != INT_MAX)
        time = earliest;

    while (completed < n)
    {
        // Lower priority value = higher priority
        int idx = -1, best_pr = INT_MAX, best_pid = INT_MAX;
        for (int i = 0; i < n; i++)
        {
            if (p[i].remaining > 0 && p[i].arrival <= time)
            {
                if (p[i].priority < best_pr || (p[i].priority == best_pr && p[i].pid < best_pid))
                {
                    best_pr = p[i].priority;
                    best_pid = p[i].pid;
                    idx = i;
                }
            }
        }
        if (idx == -1)
        {
            int next_t = next_arrival_time(p, n, time);
            time = next_t;
            continue;
        }
        p[idx].start_time = time;
        time += p[idx].burst;
        p[idx].remaining = 0;
        p[idx].completion_time = time;
        push_event(g, p[idx].pid, p[idx].start_time, p[idx].completion_time);
        completed++;
    }
    printf("\n=== Priority (Non-Preemptive) ===\n");
    print_gantt(g);
    return compute_and_print_table(p, n);
}

// Round Robin

typedef struct
{
    int q[MAXP];
    int front, back;
} Queue;

static void q_init(Queue *q) { q->front = q->back = 0; }
static int q_empty(Queue *q) { return q->front == q->back; }
static void q_push(Queue *q, int v) { q->q[q->back++ % MAXP] = v; }
static int q_pop(Queue *q) { return q->q[q->front++ % MAXP]; }

static Averages rr(Process p[], int n, int quantum, GanttChart *g)
{
    reset_stats(p, n);
    g->count = 0;
    Queue q;
    q_init(&q);

    // Start at earliest arrival
    int time = INT_MAX;
    for (int i = 0; i < n; i++)
        if (p[i].arrival < time)
            time = p[i].arrival;
    // Enqueue all that arrive at 'time'
    for (int i = 0; i < n; i++)
        if (p[i].arrival == time)
            q_push(&q, i);

    int completed = 0, last_pid = -1, slice_start = time;

    while (completed < n)
    {
        if (q_empty(&q))
        {
            // jump to next arrival
            int next_t = next_arrival_time(p, n, time);
            time = next_t;
            for (int i = 0; i < n; i++)
                if (p[i].arrival == time && p[i].remaining > 0)
                    q_push(&q, i);
            slice_start = time;
            last_pid = -1;
            continue;
        }
        int i = q_pop(&q);
        if (p[i].remaining <= 0)
            continue;

        if (!p[i].started)
        {
            p[i].started = 1;
            p[i].start_time = time;
        }

        int run = (p[i].remaining < quantum) ? p[i].remaining : quantum;
        int before = time;
        time += run;
        p[i].remaining -= run;

        // Enqueue any new arrivals that came during (before, time]
        for (int t = 0; t < n; t++)
        {
            if (p[t].remaining > 0 && p[t].arrival > before && p[t].arrival <= time)
            {
                q_push(&q, t);
            }
        }

        // Close and record this execution slice in Gantt
        push_event(g, p[i].pid, before, time);
        last_pid = p[i].pid;
        slice_start = time;

        if (p[i].remaining == 0)
        {
            p[i].completion_time = time;
            completed++;
        }
        else
        {
            // Put back to queue's end
            q_push(&q, i);
        }
    }
    printf("\n=== Round Robin (q=%d) ===\n", quantum);
    print_gantt(g);
    return compute_and_print_table(p, n);
}

// Input and menu handling

static void take_input(Process p[], int *n, int *quantum)
{
    *n = read_int_safe("Enter number of processes (1-100): ", 1, MAXP);
    for (int i = 0; i < *n; i++)
    {
        printf("\n--- Process %d ---\n", i + 1);
        p[i].pid = i + 1;
        p[i].arrival = read_int_safe("Arrival time (>=0): ", 0, INT_MAX / 2);
        p[i].burst = read_int_safe("Burst time   (>0): ", 1, INT_MAX / 2);
        p[i].priority = read_int_safe("Priority (1=high): ", INT_MIN / 2, INT_MAX / 2);
        p[i].remaining = p[i].burst;
        p[i].start_time = -1;
        p[i].completion_time = -1;
        p[i].waiting_time = 0;
        p[i].turnaround_time = 0;
        p[i].started = 0;
    }
    *quantum = read_int_safe("\nTime Quantum for RR (>0): ", 1, INT_MAX / 2);
}

static void run_algorithm_menu(Process base[], int n, int quantum)
{
    while (1)
    {
        printf("\n==============================\n");
        printf(" CPU Scheduling Simulator (C)\n");
        printf("==============================\n");
        printf("1) FCFS\n");
        printf("2) SJF (Non-Preemptive)\n");
        printf("3) Priority (Non-Preemptive)\n");
        printf("4) Round Robin\n");
        printf("5) Compare All (Best by Avg Waiting Time)\n");
        printf("0) Exit\n");
        int choice = read_int_safe("Choose: ", 0, 5);

        if (choice == 0)
        {
            printf("Bye.\n");
            break;
        }
        Process p[MAXP];
        GanttChart g;
        Averages a;
        switch (choice)
        {
        case 1:
            copy_processes(p, base, n);
            a = fcfs(p, n, &g);
            break;
        case 2:
            copy_processes(p, base, n);
            a = sjf_np(p, n, &g);
            break;
        case 3:
            copy_processes(p, base, n);
            a = priority_np(p, n, &g);
            break;
        case 4:
            copy_processes(p, base, n);
            a = rr(p, n, quantum, &g);
            break;
        case 5:
        {
            Process p1[MAXP], p2[MAXP], p3[MAXP], p4[MAXP];
            GanttChart g1, g2, g3, g4;
            Averages a1, a2, a3, a4;
            copy_processes(p1, base, n);
            a1 = fcfs(p1, n, &g1);
            copy_processes(p2, base, n);
            a2 = sjf_np(p2, n, &g2);
            copy_processes(p3, base, n);
            a3 = priority_np(p3, n, &g3);
            copy_processes(p4, base, n);
            a4 = rr(p4, n, quantum, &g4);

            double best = a1.avg_wait;
            int best_id = 1;
            if (a2.avg_wait < best)
            {
                best = a2.avg_wait;
                best_id = 2;
            }
            if (a3.avg_wait < best)
            {
                best = a3.avg_wait;
                best_id = 3;
            }
            if (a4.avg_wait < best)
            {
                best = a4.avg_wait;
                best_id = 4;
            }

            const char *names[] = {"", "FCFS", "SJF (NP)", "Priority (NP)", "RR"};
            printf("\n=== Comparison Result ===\n");
            printf("FCFS        : Avg WT = %.2f\n", a1.avg_wait);
            printf("SJF (NP)    : Avg WT = %.2f\n", a2.avg_wait);
            printf("Priority(NP): Avg WT = %.2f\n", a3.avg_wait);
            printf("RR (q=%d)   : Avg WT = %.2f\n", quantum, a4.avg_wait);
            printf("\nBest (lowest Avg Waiting Time): %s\n", names[best_id]);
            break;
        }
        default:
            printf("Invalid choice.\n");
        }
    }
}

int main(void)
{
    Process base[MAXP];
    int n = 0, quantum = 0;
    printf("=== CPU Scheduling Algorithm Simulator (C) ===\n");
    take_input(base, &n, &quantum);
    run_algorithm_menu(base, n, quantum);
    return 0;
}

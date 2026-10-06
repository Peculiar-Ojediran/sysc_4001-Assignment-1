#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

void add_new_queue(queue **queue_start, ProcessSpec spec)
{
    if (*queue_start != NULL)
    {
        queue *current = *queue_start;
        while (current != NULL && current->next != NULL)
            current = current->next;
        queue *addition = (queue *)malloc(sizeof(queue));
        addition->PCB = initialize_PCB(spec);
        addition->next = NULL;
        current->next = addition;
    }
    else
    {
        queue *addition = (queue *)malloc(sizeof(queue));
        addition->PCB = initialize_PCB(spec);
        addition->next = NULL;
        *queue_start = addition;
    }
}

void add_existing_queue(queue **queue_start, queue *current_queue)
{
    if (*queue_start != NULL)
    {
        queue *current = *queue_start;
        while (current != NULL && current->next != NULL)
            current = current->next;

        current->next = current_queue;
    }
    else
    {
        *queue_start = current_queue;
    }
}

queue *dequeue(queue **queue_start)
{
    if (*queue_start == NULL)
        return NULL;
    queue *current = *queue_start;
    if ((*queue_start)->next == NULL)
    {
        *queue_start = NULL;
        current->next = NULL;
        return current;
    }
    (*queue_start) = (*queue_start)->next;
    current->next = NULL;
    return current;
}

queue *dequeue_shortest_BT(queue **queue_start, int max_BT)
{
    if (*queue_start == NULL)
        return NULL;

    queue *current = *queue_start;
    queue *previous = NULL;

    queue *min_node = NULL;
    queue *min_previous = NULL;

    int min_BT = max_BT;

    while (current != NULL)
    {
        int burst = current->PCB->current_spec.cpu_burst;

        if (burst < min_BT)
        {
            min_BT = burst;
            min_node = current;
            min_previous = previous;
        }

        previous = current;
        current = current->next;
    }

    if (min_node == NULL)
        return NULL;

    // Minimum is the first element
    if (min_previous == NULL)
    {
        *queue_start = min_node->next;
    }
    else
    {
        min_previous->next = min_node->next;
    }

    min_node->next = NULL;

    return min_node;
}
void print_values_in_the_queue(queue **queue_start)
{
    if (*queue_start == NULL)
    {
        printf("This queue is empty\n");
    }
    else
    {
        queue *current = *queue_start;
        int i = 1;
        while (current != NULL)
        {
            printf("in queue position %d PCB stores:\n", i);
            printf("PID:%d\n", current->PCB->current_spec.pid);
            printf("arrival time:%d\n", current->PCB->current_spec.arrival_time);
            printf("cpu burst:%d\n", current->PCB->current_spec.cpu_burst);
            printf("io burst:%d\n", current->PCB->current_spec.io_burst);
            printf("number of cpu burst:%d\n", current->PCB->current_spec.num_cpu_bursts);
            printf("\n");
            i++;
            current = current->next;
        }
    }
}
void load_waiting_into_ready_queue(Simulation *simulation, int clock, OutputContext *output)
{
    if (simulation->waiting_queue == NULL)
        return;
    else
    {
        queue *current = simulation->waiting_queue;
        while (current != NULL)
        {
            if (current->PCB->current_spec.arrival_time == clock)
            {
                simulation->waiting_queue = simulation->waiting_queue->next;
                current->next = NULL;
                add_existing_queue(&(simulation->Ready_queue), current);
                log_transition(output, simulation->clock, current->PCB->starting_spec.pid, STATE_NEW, STATE_READY, REASON_ARRIVAL);
            }
            else
                return;
            current = simulation->waiting_queue;
        }
    }
}

bool has_shorter_process(queue *queue_start, int running_BT)
{
    queue *current = queue_start;

    while (current != NULL)
    {
        if (current->PCB->current_spec.cpu_burst < running_BT)
            return true;

        current = current->next;
    }

    return false;
}
bool select_next_process(Simulation *simulation, SimulationConfig config, OutputContext *output)
{
    if (config.algorithm == ALG_FCFS)
    {
        if (simulation->currently_proccessing == NULL && simulation->Ready_queue != NULL)
        {
            add_existing_queue(&(simulation->currently_proccessing), dequeue(&(simulation->Ready_queue)));
            // simulation->clock++;
            return true;
        }
        return false;
    }
    if (config.algorithm == ALG_SRTF)
    {
        /*
         * CASE 1:
         * A process is currently running.
         *
         * Only check whether a strictly shorter process
         * exists in the Ready queue.
         */
        if (simulation->currently_proccessing != NULL)
        {
            int running_BT =
                simulation->currently_proccessing
                    ->PCB
                    ->current_spec
                    .cpu_burst;

            if (has_shorter_process(
                    simulation->Ready_queue,
                    running_BT))
            {
                /*
                 * Running process is preempted.
                 */
                log_transition(
                    output,
                    simulation->clock,
                    simulation->currently_proccessing
                        ->PCB
                        ->starting_spec
                        .pid,
                    STATE_RUNNING,
                    STATE_READY,
                    REASON_SRTF_PREEMPTION);

                /*
                 * Put the running process back into Ready.
                 *
                 * dequeue() also makes currently_proccessing NULL.
                 */
                add_existing_queue(
                    &(simulation->Ready_queue),
                    dequeue(&(simulation->currently_proccessing)));

                return true;
            }

            /*
             * Nothing shorter exists.
             * Current process keeps running.
             */
            return false;
        }

        /*
         * CASE 2:
         * CPU is empty.
         *
         * This happens after a context switch or when
         * the CPU initially needs a process.
         */
        if (simulation->Ready_queue != NULL)
        {
            queue *cache = dequeue_shortest_BT(
                &(simulation->Ready_queue),
                INT_MAX);

            if (cache != NULL)
            {
                add_existing_queue(
                    &(simulation->currently_proccessing),
                    cache);

                return true;
            }
        }

        return false;
    }
    if (config.algorithm == ALG_RR)
    {

        if (simulation->currently_proccessing == NULL && simulation->Ready_queue != NULL)
        {
            // add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
            add_existing_queue(&(simulation->currently_proccessing), dequeue(&(simulation->Ready_queue)));
            // simulation->clock++;
            printf("(dispatch)\n");
            return true;
        }
        return false;
    }
    return false;
}
void decrement_queue_times(Simulation *simulation, SimulationConfig config, bool is_dispatch, OutputContext *output, TransitionReason *prev_cpu_reason, int *PID_tracker, bool *is_prev_cpu_reason, ProcessState *pending_new_state)
{
    if (simulation->currently_proccessing != NULL && is_dispatch == false)
    {
        simulation->currently_proccessing->PCB->current_spec.cpu_burst--;
        int PID = simulation->currently_proccessing->PCB->starting_spec.pid;
        if (simulation->currently_proccessing->PCB->current_spec.cpu_burst == 0)
        {
            if (simulation->currently_proccessing->PCB->starting_spec.num_cpu_bursts > 1)
            {
                if (simulation->currently_proccessing->PCB->starting_spec.io_burst > 0)
                {
                    *PID_tracker = PID;
                    // log_transition(output,simulation->clock,simulation->currently_proccessing->PCB->starting_spec.pid,STATE_RUNNING,STATE_WAITING,REASON_CPU_BURST_COMPLETE);
                    simulation->currently_proccessing->PCB->starting_spec.num_cpu_bursts--;
                    simulation->currently_proccessing->PCB->current_spec = simulation->currently_proccessing->PCB->starting_spec;
                    add_existing_queue(&(simulation->IO_queue), dequeue(&(simulation->currently_proccessing)));
                    *prev_cpu_reason = REASON_CPU_BURST_COMPLETE;
                    *is_prev_cpu_reason = true;
                    *pending_new_state = STATE_WAITING;
                }
                else
                {
                    *PID_tracker = PID;
                    // log_transition(output,simulation->clock,simulation->currently_proccessing->PCB->starting_spec.pid,STATE_RUNNING,STATE_READY,REASON_CPU_BURST_COMPLETE);
                    simulation->currently_proccessing->PCB->starting_spec.num_cpu_bursts--;
                    simulation->currently_proccessing->PCB->current_spec = simulation->currently_proccessing->PCB->starting_spec;
                    add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
                    *prev_cpu_reason = REASON_CPU_BURST_COMPLETE;
                    *is_prev_cpu_reason = true;
                    *pending_new_state = STATE_READY;
                }
            }
            else
            {
                *PID_tracker = PID;
                // log_transition(output,simulation->clock,PID,STATE_RUNNING,STATE_TERMINATED,REASON_PROCESS_COMPLETE);
                *prev_cpu_reason = REASON_PROCESS_COMPLETE;
                *pending_new_state = STATE_TERMINATED;
                *is_prev_cpu_reason = true;
                printf("A process ended in clock %d \n", simulation->clock);
                simulation->currently_proccessing = NULL;
                simulation->rolling_quatum_count = config.quantum;
            }
            simulation->rolling_quatum_count = config.quantum;
            // process has been terminated
            // select_next_process(simulation, config);
        }
        if (config.algorithm == ALG_RR && simulation->currently_proccessing != NULL)
        {
            simulation->rolling_quatum_count--;
            if (simulation->rolling_quatum_count == 0)
            {
                simulation->rolling_quatum_count = config.quantum;
                // log_transition(output,simulation->clock,simulation->currently_proccessing->PCB->starting_spec.pid,STATE_RUNNING,STATE_READY,REASON_QUANTUM_EXPIRED);
                add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
                // select_next_process(simulation, config);
                *prev_cpu_reason = REASON_QUANTUM_EXPIRED;
                *pending_new_state = STATE_READY;
                *PID_tracker = PID;
                *is_prev_cpu_reason = true;
            }
        }
        printf("processed in clock:  %d \n", simulation->clock);
    }
}
void decrement_IO_times(Simulation *simulation)
{
    queue *current = simulation->IO_queue;

    while (current != NULL)
    {
        if (current->PCB->current_spec.io_burst > 0)
        {
            current->PCB->current_spec.io_burst--;
        }

        current = current->next;
    }
}

void complete_IO(Simulation *simulation, OutputContext *output)
{
    queue *current = simulation->IO_queue;
    queue *previous = NULL;

    while (current != NULL)
    {
        if (current->PCB->current_spec.io_burst <= 0)
        {
            queue *finished = current;

            if (previous == NULL)
            {
                simulation->IO_queue = current->next;
                current = simulation->IO_queue;
            }
            else
            {
                previous->next = current->next;
                current = previous->next;
            }

            finished->next = NULL;

            finished->PCB->state = STATE_READY;

            log_transition(
                output,
                simulation->clock,
                finished->PCB->starting_spec.pid,
                STATE_WAITING,
                STATE_READY,
                REASON_IO_COMPLETE
            );

            add_existing_queue(
                &(simulation->Ready_queue),
                finished
            );
        }
        else
        {
            previous = current;
            current = current->next;
        }
    }
}
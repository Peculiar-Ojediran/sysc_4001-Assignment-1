#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

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
    if (queue_start == NULL)
    {
        printf("This queue is empty");
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
void load_waiting_into_ready_queue(Simulation *simulation, int clock)
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
                add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->waiting_queue)));
            }
            current = current->next;
        }
    }
}
bool select_next_process(Simulation *simulation, SimulationConfig config)
{
    if (config.algorithm == ALG_FCFS)
    {
        if (simulation->currently_proccessing == NULL && simulation->Ready_queue != NULL)
        {
            add_existing_queue(&(simulation->currently_proccessing), dequeue(&(simulation->Ready_queue)));
            simulation->clock++;
            return true;
        }
        return false;
    }
    if (config.algorithm == ALG_SRTF)
    {
        if (simulation->currently_proccessing != NULL)
        {
            queue *cache = NULL;
            cache = dequeue_shortest_BT(&(simulation->Ready_queue), simulation->currently_proccessing->PCB->current_spec.cpu_burst);
            if (simulation->Ready_queue != NULL && cache != NULL)
            {
                add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
                add_existing_queue(&(simulation->currently_proccessing), cache);
                simulation->clock++;
                return true;
            }
            return false;
        }
        else
        {
            queue *cache = NULL;
            cache = dequeue_shortest_BT(&(simulation->Ready_queue), 999999);
            if (simulation->Ready_queue != NULL && cache != NULL)
            {
                add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
                add_existing_queue(&(simulation->currently_proccessing), cache);
                simulation->clock++;
                return true;
            }
            return false;
        }
    }
    if (config.algorithm == ALG_RR)
    {

        if (simulation->Ready_queue != NULL)
        {
            // add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
            add_existing_queue(&(simulation->currently_proccessing), dequeue(&(simulation->Ready_queue)));
            simulation->clock++;
            return true;
        }
        return false;
    }
    return false;
}
void decrement_queue_times(Simulation *simulation, SimulationConfig config)
{
    if (simulation->currently_proccessing != NULL)
    {
        simulation->currently_proccessing->PCB->current_spec.cpu_burst--;
        if (simulation->currently_proccessing->PCB->current_spec.cpu_burst == 0)
        {
            if (simulation->currently_proccessing->PCB->starting_spec.num_cpu_bursts > 1)
            {
                if (simulation->currently_proccessing->PCB->starting_spec.io_burst > 0)
                {
                    simulation->currently_proccessing->PCB->starting_spec.num_cpu_bursts--;
                    simulation->currently_proccessing->PCB->current_spec = simulation->currently_proccessing->PCB->starting_spec;
                    add_existing_queue(&(simulation->IO_queue), dequeue(&(simulation->currently_proccessing)));
                }
                else
                {
                    simulation->currently_proccessing->PCB->starting_spec.num_cpu_bursts--;
                    simulation->currently_proccessing->PCB->current_spec = simulation->currently_proccessing->PCB->starting_spec;
                    add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
                }
            }
            // process has been terminated
            simulation->currently_proccessing = NULL;
            select_next_process(simulation, config);
        }
        if (config.algorithm == ALG_RR)
        {
            simulation->rolling_quatum_count--;
            if (simulation->rolling_quatum_count == 0)
            {
                simulation->rolling_quatum_count = config.quantum;
                add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->currently_proccessing)));
                select_next_process(simulation, config);
            }
        }
    }

    if (simulation->IO_queue != NULL)
    {
        queue *current = simulation->IO_queue;
        queue *previous = NULL;
        if (current == NULL)
            return;
        while (current != NULL)
        {
            current->PCB->current_spec.io_burst--;
            if (current->PCB->current_spec.io_burst == 0)
            {
                if (previous == NULL)
                {
                    add_existing_queue(&(simulation->Ready_queue), dequeue(&(simulation->IO_queue)));
                }
                else
                {
                    previous->next = current->next;
                    add_existing_queue(&(simulation->Ready_queue), current);
                    current = previous->next;
                }
            }
            previous = current;
            current = current->next;
        }
    }
}
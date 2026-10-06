#ifndef QUEUE_H
#define QUEUE_H

#include "scheduler.h"

void add_new_queue(queue **queue_start, ProcessSpec spec);
void add_existing_queue(queue **queue_start, queue *current_queue);
queue *dequeue(queue **queue_start);
queue *dequeue_shortest_BT(queue **queue_start, int max_BT);
void print_values_in_the_queue(queue **queue_start);
void load_waiting_into_ready_queue(Simulation *simulation,int clock,OutputContext *output);
bool select_next_process(Simulation *simulation, SimulationConfig config, OutputContext *output);
void decrement_queue_times(Simulation *simulation,SimulationConfig config, bool is_dispatch,OutputContext *output,TransitionReason *prev_cpu_reason,int *PID_tracker, bool *is_prev_cpu_reason, ProcessState *pending_new_state);
void complete_IO(Simulation *simulation, OutputContext *output);
void decrement_IO_times(Simulation *simulation);
#endif

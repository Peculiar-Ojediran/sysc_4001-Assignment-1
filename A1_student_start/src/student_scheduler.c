#include "scheduler.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * This is the source file you must complete.
 *
 * You are responsible for implementing the simulated operating-system
 * behaviour, including:
 *   - PCBs and process states;
 *   - the ready queue and waiting processes;
 *   - CPU and I/O burst accounting;
 *   - FCFS, Round Robin, and preemptive SRTF;
 *   - dispatch/context-switch overhead; and
 *   - calls to the supplied metrics and CSV logging functions.
 *
 * You may replace this file completely and add any .c/.h files you need.
 * The types and helper functions below are only a suggested decomposition.
 * You may rename, replace, combine, or remove them.
 */

/*
 * Suggested PCB starting point. Add the fields required by the handout, such
 * as remaining CPU time, completed bursts, I/O completion time, Ready-entry
 * order, and RR quantum usage.
 */

/* You may choose to maintain a structure for your queues.
 * Do so in whichever way you see fit.
 */

/* Keeping whole-simulation state together makes helper signatures smaller.
 * Store whatever you feel is necessary for your implementation.
 */

/*
 * Supplied output API examples
 * ----------------------------
 * These functions are declared in scheduler.h and implemented in io.c. They
 * format the CSV rows for you and calculate the metrics. You must use them
 * to ensure your output can be correctly processed by the test harness.
 *
 * State transition:
 *
 *   log_transition(output, time, pid,
 *                  STATE_NEW, STATE_READY, REASON_ARRIVAL);
 *
 * One timeline interval [time, time + 1):
 *
 *  If the CPU is running a process:
 *      record_timeline_tick(output, time, ACTIVITY_CPU, running_pid);
 *
 *  If the CPU is performing a context switch:
 *      record_timeline_tick(output, time, ACTIVITY_CONTEXT_SWITCH, 0);
 *
 *  If the CPU is idle:
 *      record_timeline_tick(output, time, ACTIVITY_IDLE, 0);
 *
 * Final metrics (one call after every process terminates):
 *
 *   if (!write_metrics(output, specs, count)) {
 *       return false;
 *   }
 *
 */

/*
 * Suggested helper decomposition (optional)
 * -----------------------------------------
 * You may find helpers with responsibilities like these useful:
 *
 *   initialize_pcbs(...)
 *   ready_enqueue(...)
 *   choose_next_process(...)
 *   admit_arrivals(...)
 *   complete_io(...)
 *   maybe_preempt_srtf(...)
 *   update_dispatch(...)
 *   advance_one_interval(...)
 *
 * Required event-loop order
 * -------------------------
 *
 *   while not all processes are terminated:
 *       resolve the CPU interval that ended at the current time
 *       admit arrivals in ascending PID order
 *       complete I/O in ascending PID order
 *       apply the SRTF preemption rule, if applicable
 *       complete or begin dispatch/context-switch activity
 *       record exactly one CPU, context-switch, or idle timeline tick
 *       advance time by one
 *
 * You must design and implement the loop yourself. The reference outputs care
 * about observable timing and ordering, not the names of your private helpers.
 */
PCB *initialize_PCB(ProcessSpec spec)
{
    PCB *process = (PCB *)malloc(sizeof(PCB));
    process->current_spec = spec;
    process->starting_spec = spec;
    process->state = STATE_NEW;
    return process;
}
void initialize_simulation(Simulation *simulation, SimulationConfig config)
{
    simulation->clock = 0;
    simulation->currently_proccessing = NULL;
    simulation->IO_queue = NULL;
    simulation->Ready_queue = NULL;
    simulation->waiting_queue = NULL;
    simulation->rolling_quatum_count = config.quantum;
}
void load_all_processes(const ProcessSpec *specs, size_t count, Simulation *simulation)
{
    for (size_t i = 0; i < count; i++)
    {
        add_new_queue(&(simulation->waiting_queue), specs[i]);
    }
}

/*
 * Run one complete scheduling simulation.
 *
 * Parameters
 * ----------
 * specs:
 *     A read-only array of validated process descriptions, sorted by ascending
 *     PID. Each entry supplies the arrival time, CPU- and I/O-burst lengths,
 *     and number of CPU bursts for one process. Copy entries into your PCBs if
 *     you need mutable per-process state. Do not modify or free this array.
 *
 * count:
 *     The number of ProcessSpec entries in specs.
 *
 * config:
 *     A read-only SimulationConfig containing the selected algorithm, the RR
 *     quantum, and the context-switch duration. The quantum affects scheduling
 *     only when config->algorithm is ALG_RR. Do not modify or free config.
 *
 * output:
 *     An already-open output context. Pass it to log_transition(),
 *     record_timeline_tick(), and write_metrics() as described above. main.c
 *     owns this context and will close it; do not close or free it here.
 *
 * Return true after the simulation and final metrics are written successfully.
 * Return false if initialization, simulation, or output generation fails.
 * main.c retains ownership of specs, config, and output in either case.
 */
bool run_simulation(const ProcessSpec *specs,
                    size_t count,
                    const SimulationConfig *config,
                    OutputContext *output)
{
    (void)specs;
    (void)count;
    (void)config;
    (void)output;
    Simulation simulation;
    initialize_simulation(&simulation, *config);
    bool is_dispatch = false;
    bool first_tick = true;
    int context_remaining = 0;
    TransitionReason prev_cpu_reason;
    bool is_prev_cpu_reason = false;
    int PID_tracker = -1;
    ProcessState pending_new_state;
    load_all_processes(specs, count, &simulation);

    while (simulation.currently_proccessing != NULL ||
           simulation.IO_queue != NULL ||
           simulation.Ready_queue != NULL ||
           simulation.waiting_queue != NULL)
    {
        if (!first_tick)
        {
            /*
             * This should:
             * - decrement/resolve previous CPU execution if CPU ran
             * - detect CPU burst completion
             * - detect RR quantum expiration
             * - progress/complete I/O
             *
             * You may need a small change to decrement_queue_times()
             * so it knows whether the previous interval was CPU,
             * context switch, or idle.
             */

            decrement_queue_times(&simulation, *config, is_dispatch, output, &prev_cpu_reason, &PID_tracker, &is_prev_cpu_reason, &pending_new_state);
            if (is_prev_cpu_reason == true && is_dispatch == false)
            {
                if (prev_cpu_reason == REASON_PROCESS_COMPLETE)
                    log_transition(output, simulation.clock, PID_tracker, STATE_RUNNING, pending_new_state, REASON_PROCESS_COMPLETE);
                else if (prev_cpu_reason == REASON_CPU_BURST_COMPLETE)
                    log_transition(output, simulation.clock, PID_tracker, STATE_RUNNING, pending_new_state, REASON_CPU_BURST_COMPLETE);
                else if (prev_cpu_reason == REASON_QUANTUM_EXPIRED)
                    log_transition(output, simulation.clock, PID_tracker, STATE_RUNNING, pending_new_state, REASON_QUANTUM_EXPIRED);
                is_prev_cpu_reason = false;
            }
            complete_IO( &simulation,output);
        }
        first_tick = false;
        load_waiting_into_ready_queue(&simulation, simulation.clock, output);

        if (!is_dispatch &&
            config->algorithm == ALG_SRTF &&
            simulation.currently_proccessing != NULL)
        {
            /*
             * Your select_next_process() may need a slight change so
             * that this call only performs SRTF preemption here.
             */
            select_next_process(&simulation, *config, output);
        }

        if (!is_dispatch && simulation.currently_proccessing == NULL && simulation.Ready_queue != NULL)
        {
            is_dispatch = true;
            context_remaining = config->context_switch;
        }

        if (is_dispatch && context_remaining > 0)
        {
             decrement_IO_times(&simulation);
            record_timeline_tick(output, simulation.clock, ACTIVITY_CONTEXT_SWITCH, 0);

            context_remaining--;

            /*
             * Advance exactly one interval.
             */
            simulation.clock++;

            continue;
        }

        if (is_dispatch && context_remaining == 0)
        {
            is_dispatch = false;

            bool selected = select_next_process(&simulation, *config, output);

            if (selected && simulation.currently_proccessing != NULL)
            {
                log_transition(output, simulation.clock, simulation.currently_proccessing->PCB->starting_spec.pid, STATE_READY, STATE_RUNNING, REASON_DISPATCH);

                /*
                 * RR process gets a fresh quantum when dispatched.
                 */
                if (config->algorithm == ALG_RR)
                {
                    simulation.rolling_quatum_count =
                        config->quantum;
                }
            }
        }
        decrement_IO_times(&simulation);
        if (simulation.currently_proccessing == NULL &&
            simulation.Ready_queue == NULL &&
            simulation.IO_queue == NULL &&
            simulation.waiting_queue == NULL &&
            !is_dispatch)
        {
            break;
        }
        if (simulation.currently_proccessing != NULL)
        {
            record_timeline_tick(output, simulation.clock, ACTIVITY_CPU, simulation.currently_proccessing->PCB->starting_spec.pid);
        }
        else
        {
            record_timeline_tick(output, simulation.clock, ACTIVITY_IDLE, 0);
        }
        simulation.clock++;
    }
    if (!write_metrics(output, specs, count))
        return false;

    return true;
}

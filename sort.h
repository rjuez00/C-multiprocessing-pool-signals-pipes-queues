/**
 * @file ejercicio_arbol.c
 * @author Rodrigo Juez Hernández (rodrigo.juezh@estudiante.uam.es)
 * @author Nicolas Andreu Magee Rosado (nicolas.magee@estudiante.uam.es)
 * @author Teaching team of SOPER
 * @group G2292_P03
 * @brief replica el esquema del ejercicio, para tener 3 procesos debe modificar el NUM_PROC a 3
 * @date 2020-02-23
 * 
 * 
 */


#ifndef _SORT_H
#define _SORT_H

#include <mqueue.h>
#include <semaphore.h>
#include <sys/types.h>
#include "global.h"

/* Constants. */
#define MAX_DATA 100000
#define MAX_LEVELS 10
#define MAX_PARTS 512
#define MAX_STRING 1024

#define PLOT_PERIOD 1
#define NO_MID -1

/* Type definitions. */

/* Completed flag for the tasks. */
typedef enum {
    INCOMPLETE,
    SENT,
    PROCESSING,
    COMPLETED
} Completed;

/* Task. */
typedef struct {
    Completed completed;
    int ini;
    int mid;
    int end;
} Task;

/* Job for sending Task location*/
typedef struct{
    int level;
    int part;
} Job;


/* Structure for the sorting problem. */
typedef struct{
    Task tasks[MAX_LEVELS][MAX_PARTS];
    int data[MAX_DATA];
    int delay;
    int n_elements;
    int n_levels;
    int n_processes;
    pid_t ppid;
    sem_t mutextasks;
    mqd_t queue;
    int pids[MAX_PARTS+2];  /*saves a -1 extra and the pid of the ilustrator (that's why is MAX_PARTS +2*/
} Sort;

/* Prototypes. */

/**
 * Sorts an array using bubble-sort.
 * @method bubble_sort
 * @date   2020-04-09
 * @author Teaching team of SOPER
 * @param  vector      Array with the data.
 * @param  n_elements  Number of elements in the array.
 * @param  delay       Delay for the algorithm.
 * @return             ERROR in case of error, OK otherwise.
 */
Status bubble_sort(int *vector, int n_elements, int delay);

/**
 * Merges two ordered parts of an array keeping the global order.
 * @method merge
 * @date   2020-04-09
 * @author Teaching team of SOPER
 * @param  vector     Array with the data.
 * @param  middle     Division between the first and second parts.
 * @param  n_elements Number of elements in the array.
 * @param  delay      Delay for the algorithm.
 * @return            ERROR in case of error, OK otherwise.
 */
Status merge(int *vector, int middle, int n_elements, int delay);

/**
 * Computes the number of parts (division) for a certain level of the sorting
 * algorithm.
 * @method get_number_parts
 * @date   2020-04-09
 * @author Teaching team of SOPER
 * @param  level            Level of the algorithm.
 * @param  n_levels         Total number of levels in the algorithm.
 * @return                  Number of parts in the level.
 */
int get_number_parts(int level, int n_levels);

/**
 * Initializes the sort structure.
 * @method init_sort
 * @date   2020-04-09
 * @author Teaching team of SOPER
 * @param  file_name   File with the data.
 * @param  sort        Pointer to the sort structure.
 * @param  n_levels    Total number of levels in the algorithm.
 * @param  n_processes Number of processes.
 * @param  delay       Delay for the algorithm.
 * @return             ERROR in case of error, OK otherwise.
 */
Status init_sort(char *file_name, Sort *sort, int n_levels, int n_processes, int delay);

/**
 * Checks if a task is ready to be solved.
 * @method check_task_ready
 * @date   2020-04-09
 * @author Teaching team of SOPER
 * @param  sort             Pointer to the sort structure.
 * @param  level            Level of the algorithm.
 * @param  part             Part inside the level.
 * @return                  FALSE if it is not ready, TRUE otherwise.
 */
Bool check_task_ready(Sort *sort, int level, int part);



/**
 * Solves a sorting problem using a single process.
 * @method sort_single_process
 * @date   2020-04-09
 * @author Teaching team of SOPER
 * @param  file_name        File with the data.
 * @param  n_levels         Total number of levels in the algorithm.
 * @param  n_processes      Number of processes.
 * @param  delay            Delay for the algorithm.
 * @return                  ERROR in case of error, OK otherwise.
 */
Status sort_multiple_process(char *file_name, int n_levels, int n_processes, int delay);


/**
 * @brief executes the function of the worker
 * 
 * @return Status 
 */
Status worker();

/**
 * @brief executes the clean of the worker, including the task and its shared memory
 * 
 * @param myTask 
 */
void worker_clean(char* myTask);

/**
 * @brief solves the task passed as argument
 * 
 * @param task task to solve
 * @return Status 
 */
Status  solve_task(Task* task);

/**
 * @brief cleans the main process (also known as task injector) shared memory and mallocs
 * 
 */
void injector_clean();

/**
 * @brief ilustrates the algorithm when it has received the state of everyworker
 * 
 * @param pipes 
 */
void ilustrador(int* pipes);

/**
 * @brief sends a task to workers
 * 
 * @param level 
 * @param part 
 * @return Status 
 */
Status send_task(int level, int part);

/**
 * @brief initialization of the sort shared memory
 * 
 * @return Status 
 */
Status init_shared_memory();

/**
 * @brief initialization of pipes
 * 
 * @param pipes 
 * @return Status 
 */
Status init_pipes(int* pipes);

/**
 * @brief initialization of queue
 * 
 * @return Status 
 */
Status init_queue();

/**
 * @brief initialization of handlers
 * 
 * @return Status 
 */
Status init_handlers();


#endif

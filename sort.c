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


#define _POSIX_C_SOURCE 200112L


#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "sort.h"
#include "utils.h"

#define SHM_NAME "/sharemem"
#define QUEUE_NAME "/queue01"
#define SEM_TASKS_NAME "/sem01"
#define SEM_LECTURA_NAME "/sem02"


/* Shared memory */
Sort* sort = NULL;



/* FLAGS FOR HANDLERS*/
short flag_term = 0;
short flag_usr = 0;

/*GLOBAL VARIABLES FOR WORKERS*/
int pipewriteworker, pipereadworker;
Job* actualjob;


void SIGTERM_handler(int sig) {
   flag_term = 1;
}

void SIGUSR1_handler(int sig) {
   flag_usr = 1;
}

void SIGALRM_handler(int sig) {
    Job incasenull;
    int returnfromil=-1;
    if(actualjob == NULL){
        incasenull.level = -1;
        incasenull.part = -1;
        if(write(pipewriteworker, &incasenull , sizeof(Job)) == -1){
            perror("error sending nothing job to ilustrator");
            return;
        }
    }
    else {
        if(write(pipewriteworker, actualjob , sizeof(Job)) == -1){
            perror("error sending job to ilustrator");
            return;
        }    
    }
    if(flag_term == 1) return;

    if (read(pipereadworker, &returnfromil, sizeof(int)) == -1) {
        perror("error receiving response worker");
        exit(EXIT_FAILURE);
    }
    if(returnfromil == -1){
        perror("ILUSTRATOR FAIL");
    }
    if(flag_term == 1) return;

    alarm(1);
}

void SIGINT_handler(int sig){
    int j, counter=0;
    for (j = 0; sort->pids[j] != -1; j++, counter++) 
        kill(sort->pids[j], SIGTERM); 
    
    while(1){
        if(wait(NULL) == -1){
            if(errno !=  ECHILD){
                perror("in wait");
            }
            break;
        }
    }
    injector_clean();
    exit(EXIT_SUCCESS);
}

Status bubble_sort(int *vector, int n_elements, int delay) {
    int i, j;
    int temp;

    if ((!(vector)) || (n_elements <= 0)) {
        return ERROR;
    }

    for (i = 0; i < n_elements - 1; i++) {
        for (j = 0; j < n_elements - i - 1; j++) {
            /* Delay. */
            fast_sleep(delay);
            if (vector[j] > vector[j+1]) {
                temp = vector[j];
                vector[j] = vector[j + 1];
                vector[j + 1] = temp;
            }
        }
    }

    return OK;
}

Status merge(int *vector, int middle, int n_elements, int delay) {
    int *aux = NULL;
    int i, j, k, l, m;

    if (!(aux = (int *)malloc(n_elements * sizeof(int)))) {
        return ERROR;
    }

    for (i = 0; i < n_elements; i++) {
        aux[i] = vector[i];
    }

    i = 0; j = middle;
    for (k = 0; k < n_elements; k++) {
        /* Delay. */
        fast_sleep(delay);
        if ((i < middle) && ((j >= n_elements) || (aux[i] < aux[j]))){
            vector[k] = aux[i];
            i++;
        }
        else {
            vector[k] = aux[j];
            j++;
        }

        /* This part is not needed, and it is computationally expensive, but
        it allows to visualize a partial mixture. */
        m = k + 1;
        for (l = i; l < middle; l++) {
            vector[m] = aux[l];
            m++;
        }
        for (l = j; l < n_elements; l++) {
            vector[m] = aux[l];
            m++;
        }
    }

    free((void *)aux);
    return OK;
}

int get_number_parts(int level, int n_levels) {
    /* The number of parts is 2^(n_levels - 1 - level). */
    return 1 << (n_levels - 1 - level);
}

Status init_sort(char *file_name, Sort *sort, int n_levels, int n_processes, int delay) {
    char string[MAX_STRING];
    FILE *file = NULL;
    int i, j, log_data;
    int block_size, modulus;
    sort->pids[0] = -1;
    

    if ((!(file_name)) || (!(sort))) {
        fprintf(stderr, "init_sort - Incorrect arguments\n");
        return ERROR;
    }

    /* At most MAX_LEVELS levels. */
    sort->n_levels = MAX(1, MIN(n_levels, MAX_LEVELS));
    /* At most MAX_PARTS processes can work together. */
    sort->n_processes = MAX(1, MIN(n_processes, MAX_PARTS));
    /* The main process PID is stored. */
    sort->ppid = getpid();
    /* Delay for the algorithm in ns (less than 1s). */
    sort->delay = MAX(1, MIN(999999999, delay));

    if (!(file = fopen(file_name, "r"))) {
        perror("init_sort - fopen");
        return ERROR;
    }

    /* The first line contains the size of the data, truncated to MAX_DATA. */
    if (!(fgets(string, MAX_STRING, file))) {
        fprintf(stderr, "init_sort - Error reading file\n");
        fclose(file);
        return ERROR;
    }
    sort->n_elements = atoi(string);
    if (sort->n_elements > MAX_DATA) {
        sort->n_elements = MAX_DATA;
    }

    /* The remaining lines contains one integer number each. */
    for (i = 0; i < sort->n_elements; i++) {
        if (!(fgets(string, MAX_STRING, file))) {
            fprintf(stderr, "init_sort - Error reading file\n");
            fclose(file);
            return ERROR;
        }
        sort->data[i] = atoi(string);
    }
    fclose(file);

    /* Each task should have at least one element. */
    log_data = compute_log(sort->n_elements);
    if (n_levels > log_data) {
        n_levels = log_data;
    }
    sort->n_levels = n_levels;

    /* The data is divided between the tasks, which are also initialized. */
    block_size = sort->n_elements / get_number_parts(0, sort->n_levels);
    modulus = sort->n_elements % get_number_parts(0, sort->n_levels);
    sort->tasks[0][0].completed = INCOMPLETE;
    sort->tasks[0][0].ini = 0;
    sort->tasks[0][0].end = block_size + (modulus > 0);
    sort->tasks[0][0].mid = NO_MID;
    for (j = 1; j < get_number_parts(0, sort->n_levels); j++) {
        sort->tasks[0][j].completed = INCOMPLETE;
        sort->tasks[0][j].ini = sort->tasks[0][j - 1].end;
        sort->tasks[0][j].end = sort->tasks[0][j].ini \
            + block_size + (modulus > j);
        sort->tasks[0][j].mid = NO_MID;
    }
    for (i = 1; i < n_levels; i++) {
        for (j = 0; j < get_number_parts(i, sort->n_levels); j++) {
            sort->tasks[i][j].completed = INCOMPLETE;
            sort->tasks[i][j].ini = sort->tasks[i - 1][2 * j].ini;
            sort->tasks[i][j].mid = sort->tasks[i - 1][2 * j].end;
            sort->tasks[i][j].end = sort->tasks[i - 1][2 * j + 1].end;
        }
    }

    return OK;
}

Bool check_task_ready(Sort *sort, int level, int part) {
    if (!(sort)) {
        return FALSE;
    }

    if ((level < 0) || (level >= sort->n_levels) \
        || (part < 0) || (part >= get_number_parts(level, sort->n_levels))) {
        return FALSE;
    }

    if (sort->tasks[level][part].completed != INCOMPLETE) {
        return FALSE;
    }

    /* The tasks of the first level are always ready. */
    if (level == 0) {
        return TRUE;
    }
    /* Other tasks depend on the hierarchy. */
    if ((sort->tasks[level - 1][2 * part].completed == COMPLETED) && \
        (sort->tasks[level - 1][2 * part + 1].completed == COMPLETED)) {
        return TRUE;
    }
    return FALSE;
}

Status send_task(int level, int part){
    Job sendtask;
    sendtask.level = level;
    sendtask.part = part;
    while (mq_send(sort->queue, (char*)&sendtask, sizeof(Task), 1) == -1) {
        if(errno == EINTR){
            continue;
        }
        perror("Error sending message");
        return ERROR;
    }
    sem_wait(&(sort->mutextasks));
    (sort->tasks[level][part]).completed = SENT;
    sem_post(&(sort->mutextasks));
    return OK;
}

Status  solve_task(Task* task) {
    /* In the first level, bubble-sort. */
    if (task->mid == NO_MID) {
        return bubble_sort(\
            sort->data + task->ini, \
            task->end - task->ini, \
            sort->delay);
    }
    /* In other levels, merge. */
    else {
        return merge(\
            sort->data + task->ini, \
            task->mid - task->ini, \
            task->end - task->ini, \
            sort->delay);
    }
}






/*INITIALIZATION AND CLEANING*/
Status init_shared_memory(){
    int shared_fd, ret;
    shared_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (shared_fd == -1) {
        perror("Creating shared memory");
        close(shared_fd);
        return ERROR;
    }
    ret = shm_unlink(SHM_NAME);
    if (ret == -1) {
        perror("Unlinking shared memory");
        return ERROR;
    }
    ret = ftruncate(shared_fd, sizeof(Sort));
    if (ret == -1) {
        perror("Truncating shared memory");
        close(shared_fd);
        return ERROR;
    }

    sort = (Sort*) mmap(NULL,      
        sizeof(Sort),              
        PROT_READ | PROT_WRITE,   
        MAP_SHARED, shared_fd, 0); 
    if (sort == MAP_FAILED) {
        perror("Maping shared memory");
        close(shared_fd);
        return ERROR;
    }
    close(shared_fd);





    if(sem_init(&(sort->mutextasks), 1, 1) == -1){
        perror("sem_init failed");
        return ERROR;
    }
    return OK;
}

Status init_pipes(int* pipes){
    /* PIPES
       One pipe per process (worker). two descriptors per pipe.
       Woker --> Illustrator
       pipe[worker_id * 4 + 0] -> illustrator reads
       pipe[worker_id * 4 + 1] -> worker      writes
       pipe[worker_id * 4 + 2] -> worker      reads
       pipe[worker_id * 4 + 3] -> illustrator writes*/

    int i, ret;
    for (i = 0; i < sort->n_processes * 2; i++) {
        ret = pipe(&(pipes[2*i]));
        if (ret == -1) {
            perror("Creating pipes");
            return ERROR;
        }
    }
    return OK;
}

Status init_queue(){
    struct mq_attr attributes;
    attributes.mq_flags = 0,
    attributes.mq_maxmsg = 10,
    attributes.mq_curmsgs = 0,
    attributes.mq_msgsize = sizeof(Task);
    sort->queue = mq_open(QUEUE_NAME,
        O_CREAT | O_RDWR, 
        S_IRUSR | S_IWUSR, 
        &attributes);
    if (sort->queue == -1) {
        perror("Opening queue");
        mq_unlink(QUEUE_NAME);
        munmap(sort, sizeof(*sort));
        sem_destroy(&(sort->mutextasks));
        return ERROR;
    }
    mq_unlink(QUEUE_NAME);
    return OK;
}

Status init_handlers(){
    struct sigaction act, act2, actcancel, actarlm;

    sigemptyset(&(act.sa_mask));
    act.sa_flags = 0;
    act.sa_handler = SIGTERM_handler;
    if (sigaction(SIGTERM, &act, NULL) < 0) {
        perror("sigaction SIGTERM");
        injector_clean();
        return ERROR;
    }

    sigemptyset(&(act2.sa_mask));
    act2.sa_flags = 0;
    act2.sa_handler = SIGUSR1_handler;
    if (sigaction(SIGUSR1, &act2, NULL) < 0) {
        perror("sigaction SIGUSR1");
        injector_clean();
        return ERROR;
    }

    sigemptyset(&(actcancel.sa_mask));
    actcancel.sa_flags = 0;
    actcancel.sa_handler = SIGINT_handler;
    if (sigaction(SIGINT, &actcancel, NULL) < 0) {
        perror("sigaction SIGUSR1");
        injector_clean();
        return ERROR;
    }

    sigemptyset(&(actarlm.sa_mask));
    actarlm.sa_flags = 0;
    actarlm.sa_handler = SIGALRM_handler;
    if (sigaction(SIGALRM, &actarlm, NULL) < 0) {
        perror("sigaction SIGUSR1");
        injector_clean();
        return ERROR;
    }
    return OK;
}

void injector_clean(){
    mq_close(sort->queue);
    sem_destroy(&(sort->mutextasks));
    munmap(sort, sizeof(*sort));
}

void worker_clean(char* myTask){
        mq_close(sort->queue);
        munmap(sort, sizeof(*sort));
        free(myTask);
}

void ignore_SIGINT(){
    struct sigaction actwork;
    sigemptyset(&(actwork.sa_mask));
        actwork.sa_flags = 0;
        actwork.sa_handler = SIG_IGN;
        if (sigaction(SIGINT, &actwork, NULL) < 0)
            perror("sigaction SIGTERM");
}



/*SENDING TASKS*/
Status sort_multiple_process(char *file_name, int n_levels, int n_processes, int delay) {
    int i, j;  short flag_completed = 0;
    sigset_t nowset, oldset, empty;    
    int pipes[MAX_PARTS*4]; /*for each worker 4 pipes worker_side|ilustrator_side     first direction ilustrator second direction worker*/
    sigemptyset(&empty);  sigemptyset(&oldset); sigemptyset(&nowset);


    if(init_shared_memory() == ERROR) return ERROR; /* cleaning handled inside*/
    if (init_sort(file_name, sort, n_levels, n_processes, delay) == ERROR) { /* The data is loaded and the structure initialized. */
        fprintf(stderr, "sort_single_process - init_sort\n");
        return ERROR;
    }
    if(init_queue() == ERROR) return ERROR;    
    if(init_pipes(pipes) == ERROR) return ERROR;
    if(init_handlers() == ERROR) return ERROR;
    



    printf("\nStarting algorithm with %d levels and %d processes...\n", sort->n_levels, sort->n_processes);

    sort->pids[0] = fork();
    if(sort->pids[0] == -1){
        perror("forking ilustrator");
        injector_clean();
        return ERROR;
    }
    else if(sort->pids[0] == 0){
        ilustrador(pipes);
        exit(EXIT_FAILURE);
    }
    

    
    

    
    for (j = 1; j < n_processes+1; j++) {
        sort->pids[j] = fork();       

        if (sort->pids[j] == -1) {
            perror("forking worker");
            injector_clean();
            return ERROR;
        }
        else if(sort->pids[j] == 0){
            ignore_SIGINT();
            close(pipes[(j-1)*4 +0]);
            close(pipes[(j-1)*4 +3]);
            pipereadworker = pipes[(j-1)*4 + 2];
            pipewriteworker = pipes[(j-1)*4 +1];
            worker();
            exit(EXIT_FAILURE);
        }
        sort->pids[j+1] = -1; /*signals the end so that the handler knows where to stop sending signals*/
    }

    sigaddset(&nowset, SIGUSR1);

    
    /*sending first batch of tasks*/
    if (sigprocmask(SIG_BLOCK, &nowset, &oldset) < 0) { injector_clean(); exit(EXIT_FAILURE); }
    for (j = 0; j < get_number_parts(0, sort->n_levels); j++) 
        if(send_task(0, j) == ERROR){ injector_clean(); exit(EXIT_FAILURE);   }
    if (sigprocmask(SIG_SETMASK, &oldset, NULL) < 0) injector_clean();
    
    
    /*wait for the workers to answer*/
    while(1){
        if(flag_usr == 1){
            flag_usr = 0;
            flag_completed = 1;

            sem_wait(&(sort->mutextasks));   
            for(i=0; i<sort->n_levels; i++){
                for(j=0; j<get_number_parts(i, sort->n_levels); j++){
                    if(sort->tasks[i][j].completed == INCOMPLETE){
                        if(check_task_ready(sort, i, j) == TRUE) send_task(i, j);
                        flag_completed = 0;
                    }
                    else if(sort->tasks[i][j].completed == COMPLETED) continue; 
                    else flag_completed = 0;    
                }
            }
            sem_post(&(sort->mutextasks));

            if(flag_completed == 1) break;
        }     

        sigsuspend(&empty); 

        
        
    }

       
    

    for (j = 0; j < n_processes+1; j++) 
        kill(sort->pids[j], SIGTERM);

    for (j = 0; j < n_processes+1; j++) 
        if(wait(NULL) == -1)
            if(errno != EINTR) perror("error waiting for workers");

    plot_vector(sort->data, sort->n_elements);
    printf("\nAlgorithm completed\n");
    injector_clean();
    return OK;
}



/*EACH PROCESS FUNCTION*/
Status worker(){
    char* buffer;
    Job*  received;
    Task* perform;
    buffer = (char*) malloc(sizeof(Task));
    actualjob = NULL;
    alarm(1);
    while(1){
        if(flag_term == 1) break;
        while (mq_receive(sort->queue, buffer, sizeof(Task), NULL) == -1) {
            if(errno == EINTR){
                if(flag_term == 1) break;
                continue;         
            }
            else {
                perror("Error receiving message");
                
                worker_clean(buffer);
                kill(getppid(), SIGUSR1);                    
                exit(EXIT_FAILURE);
            }
        }

        received = (Job*) buffer;
        perform = &(sort->tasks[received->level][received->part]);
        actualjob = received;

      


        if(solve_task(perform) == ERROR){
            sem_wait(&(sort->mutextasks));
            (perform)->completed = INCOMPLETE;
            sem_post(&(sort->mutextasks));
            perror("error in solve_task");
            worker_clean(buffer);
            exit(EXIT_FAILURE);
        }
        


        (perform)->completed = COMPLETED;

       
        if(flag_term == 1) break;
        kill(getppid(), SIGUSR1);
        actualjob = NULL;
    }
    
    worker_clean(buffer);
    exit(EXIT_SUCCESS);
}



/*ILUSTRATOR*/
void ilustrador(int *pipes){
    int i = 0, j=0;
    int counter = 0; /*couynts the inactive processes*/
    int send = 0;
    Job receiver[MAX_PARTS];
   /*pipe[worker_id * 4 + 0] -> illustrator reads
     pipe[worker_id * 4 + 1] -> worker      writes
     pipe[worker_id * 4 + 2] -> worker      reads
     pipe[worker_id * 4 + 3] -> illustrator writes
    */
    for(i=0; i<sort->n_processes; i++){
        close(pipes[i*4 +1]);
        close(pipes[i*4 +2]);
    }

    while(1){
        counter= 0;
        for(i=0; i<sort->n_processes; i++){
            if (read(pipes[i*4 + 0], &(receiver[i]), sizeof(Job)) == -1) {
                if(errno == EINTR){
                    break;
                }
                perror("error receiving worker state");
                break;
            }
        }
        
        plot_vector(sort->data, sort->n_elements);
        
        
        printf("\n\n Process data: ");
        for(i=0; i<sort->n_processes; i++){
            if(receiver[i].level == -1){
                counter++;
            }
            else {printf("Process State: %i, %i | ", receiver[i].level, receiver[i].part);}
        }

        printf("Inactive processes: %d\n", counter);
        

        printf("\n\n TASKS STATE:\n");
        for(i=0; i<sort->n_levels; i++){
            for(j=0; j<get_number_parts(i, sort->n_levels); j++){
                printf("%i, ", (sort->tasks[i][j]).completed);
            }
            printf("\n");
        }

        for(i=0; i<sort->n_processes; i++){
            if (write(pipes[i*4 + 3], &send, sizeof(int)) == -1) {
                perror("error writing response ilustrator");
                break;
            }
        }
        if(flag_term == 1) break;

    }

    for(i=0; i<sort->n_processes; i++){
        close(pipes[i*4 +0]);
        close(pipes[i*4 +3]);
    }
    exit(EXIT_SUCCESS);

        
}
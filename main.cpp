/* ********************************************************* *
 * main.cpp:                                                 *
 *                                                           *
 * os.cs.siue.edu                                            *
 *                                                           *
 * compile: "cc main.cpp"                                    *
 *                                                           *
 * Joseph Haenel, 385                                        *
 *                                                           *
 * 4:45 p.m., February 5, 2023                               *
 * ********************************************************* */
#include <stdio.h>  // for printf
#include <unistd.h> // for usleep

#include <stdlib.h>  // for exit
#include <stdbool.h> // for "false" and "true"
#include <string.h>  // for strcpy

#include <time.h> // for time()

#include <sys/ipc.h> // for IPC
#include <sys/shm.h> // for shared memory system calls
#include <sys/sem.h> // for semaphore system calls
#include <sys/msg.h> // for message queue

#define SHM_KEY 5510 // the shared memory key
#define MSG_KEY 8275 // Message queue key (unique)

#define NUM_REPEATS 200 // number of loops for high-priority processes
#define NUM_CHILD 4     // number of the child processes

#define BUFFER_SIZE 1024 // max. message queue size

#define ONE_SECOND 1000    //    1 second
#define THREE_SECONDS 3000 //    3 seconds

int ret_val;

unsigned int uniform_rand(void); // a random number generator

void millisleep(unsigned ms); // for random sleep time

void process_C1(struct my_mem *p_shm, int msqid); // Child #1

void process_C2(struct my_mem *p_shm, int msqid); // Child #2

void process_C3(struct my_mem *p_shm, int msqid); // Child #3

void process_C4(struct my_mem *p_shm, int msqid); // Child #4

// definition of message -------------------------------------------
struct message
{
    long mtype;
    unsigned int mnum;
};

// definition of shared memory --------------------------------------
struct my_mem
{
    unsigned int Go_Flag;
    unsigned int Done_Flag[NUM_CHILD];
    int Individual_Sum[NUM_CHILD];
};

int main()
{

    int msqid;    // message queue ID
    key_t msgkey; // message-queue key

    struct message buf_01;
    struct message buf_02;
    struct message buf_03;
    struct message buf_04;

    msgkey = MSG_KEY; // the messge-que ID key

    // create a new message queue -----------------------------------
    msqid = msgget(msgkey, 0666 | IPC_CREAT);
    if (msqid < 0)
    {
        printf("a new message queue is not created ....\n");
    }

    // take care of "mtype" -----------------------------------------
    buf_03.mtype = 1; // UNIX standard says, any number
    buf_04.mtype = 1; // UNIX standard says, any number

    struct my_mem *p_shm; // pointer to shared memory
    int smh_id;

    p_shm = (struct my_mem *)shmat(shm_id, NULL, 0);

    // Initializing shared memory Values -----------------------------
    p_shm->Go_Flag = 0;

    for (int i = 0; i < NUM_CHILD; i++)
    {
        p_shm->Done_Flag[i] = 0;
        p_shm->Individual_Sum[i] = 0;
    }
    // ---------------------------------------------------------------

    printf("========== THE MASTER PROCESS STARTS ==========\n");

    for (int i = 1; i <= NUM_CHILD; ++i)
    {
        pid_t pid = fork();
        if (pid > 0)
        { /* I am the parent, create more children */
            continue;
        }
        else if (pid == 0)
        {
            if (i == 1)
            {
                // Consumer1
                process_C1(p_shm, pid);
            }
            else if (i == 2)
            {
                // Consumer2
                process_C2(p_shm, pid);
            }
            else if (i == 3)
            {
                // Producer1
                process_C3(p_shm, pid);
            }
            else
            {
                // Producer2
                process_C4(p_shm, pid);
            }
            break;
        }
        else
        {
            printf("fork error\n");
            exit(1);
        }
    }

    // Make Go_Flag to 1

    p_shm->Go_Flag = 1;

    printf("----------------------- the master process waits for children to terminate ... \n\n");

    while ((p_shm->Done_Flag[0] == 0) || (p_shm->Done_Flag[1] == 0) || (p_shm->Done_Flag[2] == 0) || (p_shm->Done_Flag[3] == 0))
    {
        // Do nothing, spin-wait until ALL Done_Flag's are 1
    }

    int send_checksum = p_shm->Individual_Sum[2] + p_shm->Individual_Sum[3];
    int recieved_checksum = p_shm->Individual_Sum[0] + p_shm->Individual_Sum[1];

    printf("Master Process Report **************************/n");
    printf("    C1 Checksum: %d\n", p_shm->Individual_Sum[0]);
    printf("    C2 Checksum: %d\n", p_shm->Individual_Sum[1]);
    printf("    C3 Checksum: %d\n", p_shm->Individual_Sum[2]);
    printf("    C4 Checksum: %d\n", p_shm->Individual_Sum[3]);
    printf("    SEND-CHECKSUM: %d\n", send_checksum);
    printf("    RECV-CHECKSUM: %d\n", recieved_checksum);
    printf("---------------------- the master process is terminating ...\n\n");

    // detach the shared memory ---
    ret_val = shmdt(p_shm);
    if (ret_val != 0)
    {
        printf("shared memory detach failed ....\n");
    }

    ret_val = shmctl(shm_id, IPC_RMID, 0);
    if (ret_val != 0)
    {
        printf("shared memory ID remove ID failed ... \n");
    }

    return 0;
}

/* Process C1 ============================================================= */
void process_C1(struct my_mem *p_shm, int msqid)
{
    int i;                     // the loop counter
    int status;                // result status code
    unsigned int my_rand;      // a randon number
    unsigned int checksum = 0; // the local checksum

    struct message buf_01;

    // REQUIRED output #1 -------------------------------------------
    // NOTE: C1 can not make any output before this output
    printf("    Child Process #1 is created ....\n");
    printf("    I am the first consumer ....\n\n");

    // REQUIRED: shuffle the seed for random generator --------------
    srand(time(0));

    while (p_shm->Go_Flag == 0)
    {
        // Spin-wait until Go_Flag is updated to 1 by Master process-
    }

    // Consumer Process Starting -------------------------------------
    for (i = 0; i < NUM_REPEATS; i++)
    {
        // receive a message from the message queue ---------------------
        status = msgrcv(msqid, (struct msgbuf *)&buf_01, sizeof(buf_01.mnum), 0, 0);
        if (status < 0)
        {
            printf("         msgrcv ERROR ....  terminating\n");
        }
        else
        {
            printf("I am here p1");
            // extract the data (message) ------------------------------------
            int temp = buf_01.mnum;
            checksum = checksum + temp;
        }

        p_shm->Individual_Sum[0] = checksum;
    }

    // REQUIRED 3 second wait ---------------------------------------
    millisleep(THREE_SECONDS);

    // REQUIRED output #2 -------------------------------------------
    // NOTE: after the following output, C1 can not make any output
    printf("    Child Process #1 is terminating (checksum: %d) ....\n\n", checksum);

    // raise my "Done_Flag" -----------------------------------------
    p_shm->Done_Flag[0] = 1; // I m done!

    _Exit(3); // Terminate the child process
}

/* Process C1 ============================================================= */
void process_C2(struct my_mem *p_shm, int msqid)
{
    int i;                     // the loop counter
    int status;                // result status code
    unsigned int my_rand;      // a randon number
    unsigned int checksum = 0; // the local checksum

    struct message buf_02;

    // REQUIRED output #1 -------------------------------------------
    // NOTE: C1 can not make any output before this output
    printf("    Child Process #2 is created ....\n");
    printf("    I am the first consumer ....\n\n");

    // REQUIRED: shuffle the seed for random generator --------------
    srand(time(0));

    while (p_shm->Go_Flag == 0)
    {
        // Spin-wait until Go_Flag is updated to 1 by Master process-
    }

    // Consumer Process Starting -------------------------------------
    for (i = 0; i < NUM_REPEATS; i++)
    {
        // receive a message from the message queue ---------------------
        status = msgrcv(msqid, (struct msgbuf *)&buf_02, sizeof(buf_02.mnum), 0, 0);
        if (status < 0)
        {
            printf("         msgrcv ERROR ....  terminating\n");
        }
        else
        {
            printf("I am here p2");
            // extract the data (message) ------------------------------------
            int temp = buf_02.mnum;
            checksum = checksum + temp;
        }

        p_shm->Individual_Sum[1] = checksum;
    }

    // REQUIRED 3 second wait ---------------------------------------
    millisleep(THREE_SECONDS);

    // REQUIRED output #2 -------------------------------------------
    // NOTE: after the following output, C1 can not make any output
    printf("    Child Process #2 is terminating (checksum: %d) ....\n\n", checksum);

    // raise my "Done_Flag" -----------------------------------------
    p_shm->Done_Flag[1] = 1; // I m done!

    _Exit(3); // Terminate the child process
}

/* Process C3 ============================================================= */
void process_C3(struct my_mem *p_shm, int msqid)
{
    int i;                 // the loop counter
    int status;            // result status code
    unsigned int my_rand;  // a randon number
    unsigned int checksum; // the local checksum

    struct message buf_03;

    // REQUIRED output #1 -------------------------------------------
    // NOTE: C1 can not make any output before this output
    printf("    Child Process #3 is created ....\n");
    printf("    I am the first producer ....\n\n");

    // REQUIRED: shuffle the seed for random generator --------------
    srand(time(0));

    while (p_shm->Go_Flag == 0)
    {
        // Spin-wait until Go_Flag is updated to 1 by Master process-
    }

    printf("After Go_flag "); // NEVER PRINTS????

    for (i = 0; i < NUM_REPEATS; i++)
    {
        // copy a number ------------------------------------------------
        buf_03.mnum = uniform_rand();

        // send a number to the message queue ---------------------------
        status = msgsnd(msqid, (struct msgbuf *)&buf_03, sizeof(buf_03.mnum), 0);
        if (status < 0)
        {
            printf("         msgsnd ERROR ....  terminating\n");
        }
        else
        {
            printf("I am here p3");
            int temp = buf_03.mnum;
            checksum = checksum + temp;
        }
    }

    p_shm->Individual_Sum[2] = checksum;

    // REQUIRED 3 second wait ---------------------------------------
    millisleep(THREE_SECONDS);

    // REQUIRED output #2 -------------------------------------------
    // NOTE: after the following output, C1 can not make any output
    printf("    Child Process #3 is terminating (checksum: %d) ....\n\n", checksum);

    // raise my "Done_Flag" -----------------------------------------
    p_shm->Done_Flag[2] = 1; // I m done!

    _Exit(3); // Terminate the child process
}

/* Process C4 ============================================================= */
void process_C4(struct my_mem *p_shm, int msqid)
{
    int i;                 // the loop counter
    int status;            // result status code
    unsigned int my_rand;  // a randon number
    unsigned int checksum; // the local checksum

    struct message buf_04;

    // REQUIRED output #1 -------------------------------------------
    // NOTE: C1 can not make any output before this output
    printf("    Child Process #4 is created ....\n");
    printf("    I am the first producer ....\n\n");

    // REQUIRED: shuffle the seed for random generator --------------
    srand(time(0));

    while (p_shm->Go_Flag == 0)
    {
        // Spin-wait until Go_Flag is updated to 1 by Master process-
    }

    for (i = 0; i < NUM_REPEATS; i++)
    {
        // copy a number ------------------------------------------------
        buf_04.mnum = uniform_rand();

        // send a number to the message queue ---------------------------
        status = msgsnd(msqid, (struct msgbuf *)&buf_04, sizeof(buf_04.mnum), 0);
        if (status < 0)
        {
            printf("         msgsnd ERROR ....  terminating\n");
        }
        else
        {
            printf("I am here p4");
            int temp = buf_04.mnum;
            checksum = checksum + temp;
        }
    }

    p_shm->Individual_Sum[3] = checksum;

    // REQUIRED 3 second wait ---------------------------------------
    millisleep(THREE_SECONDS);

    // REQUIRED output #2 -------------------------------------------
    // NOTE: after the following output, C1 can not make any output
    printf("    Child Process #4 is terminating (checksum: %d) ....\n\n", checksum);

    // raise my "Done_Flag" -----------------------------------------
    p_shm->Done_Flag[3] = 1; // I m done!

    _Exit(3); // Terminate the child process
}

// uniform_rand
unsigned int uniform_rand(void) /* generate a random number 0 ~ 999 */
{
    unsigned int my_rand;

    my_rand = rand() % 1000;

    return (my_rand);
}

/* function "millisleep" ------------------------------------------ */
void millisleep(unsigned milli_seconds)
{
    usleep(milli_seconds * 1000);
}

// for (i = 0; i < NUM_REPEATS; i++)
// {
//     int random_num = uniform_rand(); // Generate random number
// }

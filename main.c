#include <mpi.h>
#include <stdio.h>

#define ASSUMED_MAX_PROCESSES 1000

#define TAG_TEST 100
#define TAG_ANSWER 150

#define DESK 1
#define LAPTOP 2
#define DRUKARKA 3

#define AGREED 1
#define REFUSED 2
#define DO_NOT_CARE 3

int state = DESK;
int B = 25;
int L = 15;
int D = 10;

void answerRequest(int rank, int round, int recipient, int request[3]) {

    int agreed[2] = {rank, AGREED};
    int refused[2] = {rank, REFUSED};
    int doNotCare[2] = {rank, DO_NOT_CARE};

    if (request[2] == DESK) {
        if (state == DESK) {
            if (round > request[1]) {
                MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            } else if (round < request[1]) {
                MPI_Send(refused, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            } else {
                if (rank < request[0]) {
                    MPI_Send(refused, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
                } else {
                    MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
                }
            }
        } else {
            MPI_Send(doNotCare, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
        }
    } else if (request[2] == LAPTOP) {
        if (state == DESK) {
            MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
        } else if (state == LAPTOP) {
            if (round > request[1]) {
                MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            } else if (round < request[1]) {
                MPI_Send(refused, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            } else {
                if (rank < request[0]) {
                    MPI_Send(refused, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
                } else {
                    MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
                }
            }
        } else if (state == DRUKARKA) {
            MPI_Send(doNotCare, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
        }
    } else if (request[2] == DRUKARKA) {
        if (state < DRUKARKA) {
            MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
        } else {
            if (round > request[1]) {
                MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            } else if (round < request[1]) {
                MPI_Send(refused, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            } else {
                if (rank < request[0]) {
                    MPI_Send(refused, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
                } else {
                    MPI_Send(agreed, 2, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
                }
            }
        }
    }
}

void askForStuff(int rank, int round, int recipient) {
    int message[3] = {rank, round, state};
    MPI_Send(message, 3, MPI_INT, recipient, TAG_TEST, MPI_COMM_WORLD);
}

void acquireResource(int index, int timesUsed, int size) {

    for (int i = 0; i < size; i++) {
        if (i != index) {
            askForStuff(index, timesUsed, i);
        }
    }

    int answer[ASSUMED_MAX_PROCESSES][3];
    MPI_Status status;
    for (int sender = 0; sender < size; sender++) {
        if (sender != index) {
            MPI_Recv(
                answer[sender], 3, MPI_INT,
                sender, TAG_TEST, MPI_COMM_WORLD, &status
            );
        }
    }

    for (int i = 0; i < size; i++) {
        if (i != index) {
            answerRequest(index, timesUsed, i, answer[i]);
        }
    }

    int received[ASSUMED_MAX_PROCESSES][2];
    int globalAgree = AGREED;
    int hasDeskCount = 0;
    for (int requestResponse = 0; requestResponse < size; requestResponse++) {
        if (requestResponse != index) {
            MPI_Recv(
                    received[requestResponse], 2, MPI_INT,
                    requestResponse, TAG_ANSWER, MPI_COMM_WORLD, &status
            );

            if (received[requestResponse][1] == REFUSED) {
                globalAgree = REFUSED;
            } else if (received[requestResponse][1] == DO_NOT_CARE) {
                hasDeskCount++;
            }
        }
    }

    int maxGlobalCount;
    if (state == DESK) maxGlobalCount = B;
    if (state == LAPTOP) maxGlobalCount = L;
    if (state ==DRUKARKA) maxGlobalCount = D;

    if (globalAgree == REFUSED || hasDeskCount >= B) {
        acquireResource(index, timesUsed, size);
    }
}

int main (int argc, char* argv[])
{
    int rank, size, clock, round;

    clock = 0;
    round = 0;

    MPI_Init (&argc, &argv);      /* starts MPI */
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);        /* get current process id */
    MPI_Comm_size (MPI_COMM_WORLD, &size);        /* get number of processes */

    while (1) {
        state = DESK;
        acquireResource(rank, round, size);
        state = LAPTOP;
        acquireResource(rank, round, size);
        state = DRUKARKA;
        acquireResource(rank, round, size);
        round++;
        printf("Proces %d załatwił sprawę po raz %d \n", rank, round);
    }

    MPI_Finalize();
    return 0;
}

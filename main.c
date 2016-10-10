#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSUMED_MAX_PROCESSES 1000

#define TAG_TEST 100
#define TAG_ANSWER 150

#define DESK 1
#define LAPTOP 2
#define DRUKARKA 3

#define AGREED 1
#define REFUSED 2
#define DO_NOT_CARE 3

int clock = 0;
int state = DESK;
int B = 25;
int L = 15;
int D = 10;

int max(int a, int b) {
	return a > b ? a : b;
}

void answerRequest(int rank, int round, int recipient, int request[3]) {

    int agreed[3] = {rank, AGREED, clock};
    int refused[3] = {rank, REFUSED, clock};
    int doNotCare[3] = {rank, DO_NOT_CARE, clock};

    if (request[2] < state) {
    	MPI_Send(doNotCare, 3, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
    } else if (request[2] == state) {
    	if (round > request[1]) {
            MPI_Send(agreed, 3, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
        } else if (round < request[1]) {
            MPI_Send(refused, 3, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
        } else {
            if (rank < request[0]) {
                MPI_Send(refused, 3, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            } else {
                MPI_Send(agreed, 3, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
            }
        }
    } else {
    	MPI_Send(agreed, 3, MPI_INT, recipient, TAG_ANSWER, MPI_COMM_WORLD);
    }

    clock++;
}

void askForStuff(int rank, int round, int recipient) {
    int message[4] = {rank, round, state, clock};
    MPI_Send(message, 4, MPI_INT, recipient, TAG_TEST, MPI_COMM_WORLD);
    clock++;
}

void acquireResource(int index, int timesUsed, int size) {

    for (int i = 0; i < size; i++) {
        if (i != index) {
            askForStuff(index, timesUsed, i);
        }
    }

    int answer[ASSUMED_MAX_PROCESSES][4];
    MPI_Status status;
    for (int sender = 0; sender < size; sender++) {
        if (sender != index) {
            MPI_Recv(
                answer[sender], 4, MPI_INT,
                sender, TAG_TEST, MPI_COMM_WORLD, &status
            );
            clock = (max(clock, answer[sender][3])+1);
        }
    }

    for (int i = 0; i < size; i++) {
        if (i != index) {
            answerRequest(index, timesUsed, i, answer[i]);
        }
    }

    int received[ASSUMED_MAX_PROCESSES][3];
    int hasDeskCount = 0;
    for (int requestResponse = 0; requestResponse < size; requestResponse++) {
        if (requestResponse != index) {
            MPI_Recv(
                    received[requestResponse], 3, MPI_INT,
                    requestResponse, TAG_ANSWER, MPI_COMM_WORLD, &status
            );
            clock = (max(clock, answer[requestResponse][2])+1);
			if (received[requestResponse][1] == DO_NOT_CARE || received[requestResponse][1] == REFUSED) {
                hasDeskCount++;
            }
        }
    }

    int maxGlobalCount;
    if (state == DESK) maxGlobalCount = B;
    if (state == LAPTOP) maxGlobalCount = L;
    if (state ==DRUKARKA) maxGlobalCount = D;

    if (hasDeskCount >= maxGlobalCount) {
        acquireResource(index, timesUsed, size);
    }
}

int main (int argc, char* argv[])
{

	B = atoi(argv[1]);
	L = atoi(argv[2]);
	D = atoi(argv[3]);

	int rank, size, round;

    round = 0;

    MPI_Init (&argc, &argv);      /* starts MPI */
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);        /* get current process id */
    MPI_Comm_size (MPI_COMM_WORLD, &size);        /* get number of processes */

    for (int i = 0; i < 10; i++) {
        state = DESK;
        acquireResource(rank, round, size);
        state = LAPTOP;
        acquireResource(rank, round, size);
        state = DRUKARKA;
        acquireResource(rank, round, size);
        round++;
        clock++;
        printf("Proces %d załatwił sprawę po raz %d, clock: %d \n", rank, round, clock);
    }

    MPI_Finalize();
    return 0;
}

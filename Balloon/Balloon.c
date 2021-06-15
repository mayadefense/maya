#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <omp.h>

void msleep(int ms) {
    struct timespec tm, tm2;
    tm.tv_sec = 0;
    tm.tv_nsec = ms * 1000000L;
    nanosleep(&tm, &tm2);
}

void musleep(int mus) {
    struct timespec tm, tm2;
    tm.tv_sec = 0;
    tm.tv_nsec = mus * 1000L;
    nanosleep(&tm, &tm2);
}

void nsleep(long ns) {
    struct timespec tm, tm2;
    tm.tv_sec = 0;
    tm.tv_nsec = ns;
    nanosleep(&tm, &tm2);
}

int main(int argc, char* argv[]) {
    FILE* fp;
    const char * name = "/dev/shm/powerBalloon.txt";
    const char * nameMax = "/dev/shm/powerBalloonMax.txt";
    int ret;
    int maxthreads;
    int level = 0;
    int param, threads;
    int reps, rank;
    int t, r;

    double ***A;
    double maxdiff;
    int i, j, k;
    int n;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <max threads>\n", argv[0]);
        exit(-1);
    }

    maxthreads = atoi(argv[1]);
    fp = fopen(nameMax, "w");
    //printf("Running with maximum threads %d\n", maxthreads);
    fprintf(fp, "%d", 20);
    fclose(fp);
    fp = fopen(name, "w");
    fprintf(fp, "%d", 1);
    fclose(fp);

    omp_set_num_threads(maxthreads);

    n = 500;
    int preps[] = {0, 1, 2, 4, 2, 1, 2, 1, 2, 2, 100};
    int kreps[] = {0, 1, 1, 1, 4, 10, 9, 10, 4, 8, 100};
    int sdur[] = {0, 25000, 12000, 10000, 8000, 4000, 250, 200, 10, 0, 100};

    A = (double***) malloc(sizeof (double**)*maxthreads);

#pragma omp parallel for private(i, j)
    for (t = 0; t < maxthreads; t++) {
        A[t] = (double**) malloc(sizeof (double*)*n);
        A[t][0] = (double*) malloc(sizeof (double)*n * n);
        for (i = 1; i < n; i++) {
            A[t][i] = A[t][0] + i*n;
        }

        for (i = 0; i < n; i++) {
            for (j = 0; j < n; j++) {
                A[t][i][j] = (double) rand() / RAND_MAX * 2.0 - 1.0;
            }
        }
    }
    int old = level;
    while (1) {
        fp = fopen(name, "r");
        ret = fscanf(fp, "%d", &level);
        fclose(fp);
        if (level < 0)
            level = 0;
        if (level > 20)
            level = 20;

        param = level / 2;
        threads = level * (maxthreads + 1) / 20;
        if (param < 10) {
#pragma omp parallel private(rank, r, t, i, j, k)
            {
                rank = omp_get_thread_num();
                if ((param <= 0) || (rank >= threads))
                    msleep(10);
                else {
                    r = preps[param];
                    for (t = 0; t < r; t++) {
                        for (i = 1; i < n - 1; i++)
                            for (j = 0; j < n; j++) {
                                for (k = 0; k < kreps[param]; k++)
                                    A[rank][i][j] = (A[rank][i - 1][j] +
                                        A[rank][i][j] +
                                        A[rank][i + 1][j])*0.333;
                            }
                        nsleep(sdur[param]*50);
                    }
                }
            }
        } else {
            r = preps[param];
            for (t = 0; t < r; t++) {
                maxdiff = 0.0;
#pragma omp parallel for private(j) reduction(max:maxdiff)
                for (i = 1; i < n - 1; i++) {
                    for (j = 1; j < n - 1; j++) {
                        A[1][i][j] = 0.2 * (A[0][i][j] + A[0][i - 1][j] + A[0][i + 1][j] +
                                A[0][i][j - 1] + A[0][i][j + 1]);
                        if (fabs(A[1][i][j] - A[0][i][j]) > maxdiff)
                            maxdiff = fabs(A[1][i][j] - A[0][i][j]);
                    }
                }

            }
        }

    }
    return 0;
}

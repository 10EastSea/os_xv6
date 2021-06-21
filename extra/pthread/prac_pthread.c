#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 4
#define MAX_NUM 40000000

int arr[MAX_NUM];
int sorted[MAX_NUM];

struct thread_data
{
    int p;
    int r;
};

struct thread_data thread_data_array[NUM_THREADS];

int check_sorted(int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (arr[i] != i)
            return 0;
    return 1;
}

// Implement your solution here

void merge(int p, int q, int r) {
    int i, j, k, l;
    i = p;
    j = q+1;
    k = p;

    while(i<=q && j<=r) {
        if(arr[i]<=arr[j]) sorted[k++] = arr[i++];
        else sorted[k++] = arr[j++];
    }

    if(i>q) for(l=j; l<=r; l++) sorted[k++] = arr[l];
    else for(l=i; l<=q; l++) sorted[k++] = arr[l];

    for(l=p; l<=r; l++) arr[l] = sorted[l];
}

void mergeSort(int p, int r) {
    int q;
    if(p<r) {
        q = (p+r)/2;
        mergeSort(p , q);
        mergeSort(q+1, r);
        merge(p, q, r);
    }
}

void *PrintHello(void *threadarg)
{
    int p, r;
    struct thread_data *my_data;

    my_data = (struct thread_data *) threadarg;
    p = my_data->p;
    r = my_data->r;
    mergeSort(p,r);
    pthread_exit(NULL);
}

///////////////////////////////

int main(void)
{
    srand((unsigned int)time(NULL));
    const int n = MAX_NUM;
    int i;

    for (i = 0; i < n; i++)
        arr[i] = i;
    for (i = n - 1; i >= 1; i--)
    {
        int j = rand() % (i + 1);
        int t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }

    printf("Sorting %d elements...\n", n);

    // Create threads and execute.

    // mergeSort(0, n-1);

    pthread_t threads[NUM_THREADS];
    int rc, t;
    void *status;

    int num = 0;
    for(t = 0; t < NUM_THREADS; t++) {
        thread_data_array[t].p = num;
        num += n/NUM_THREADS;
        thread_data_array[t].r = num-1;
        rc = pthread_create(&threads[t], NULL, PrintHello, (void *)&thread_data_array[t]);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for(t = 0; t < NUM_THREADS; t++) {
        rc = pthread_join(threads[t], &status);
        if (rc) {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }

    merge(0, (n/2-1)/2, n/2-1);
    merge(n/2, (n/2 + n-1)/2, n-1);
    merge(0, (n-1)/2, n-1);

    //////////////////////////////

    if (!check_sorted(n))
    {
        printf("Sort failed!\n");
        return 0;
    }

    printf("Ok %d elements sorted\n", n);
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>





int main(void){    int comm_size, my_rank, error;
    
    
    /* Initialize MPI environment - Get number of processes and rank. */
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    
    if(my_rank == 0){
        MPI_Request x;
        int *y, t = 5;
        int l = 7;
        y = &t;
        MPI_Send_init(y, 1, MPI_INT, 1,0, MPI_COMM_WORLD, &x);
        MPI_Start(&x);
        MPI_Waitall(1, &x,MPI_STATUS_IGNORE);
        y = &l;
        t = 3;
        MPI_Start(&x);
    }
    else{
        MPI_Status stat;
        int x;

        MPI_Recv(&x,1, MPI_INT,0,0,MPI_COMM_WORLD,&stat);
        printf("%d\n",x);
            MPI_Recv(&x,1, MPI_INT,0,0,MPI_COMM_WORLD,&stat);
        printf("%d\n",x);
    }


    /* Terminate MPI execution */
    MPI_Finalize();

    return 0;
}

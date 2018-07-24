#include <stdio.h>
//#include <mpi.h>
#include "utils.h"

int main(void){
    int image_type, image_width, image_height, image_seed;
    int **filter;
    int error;
    //int comm_size, my_rank;
    
    // Create filter //
    error = allocate_mem_filter(&filter);
    if(error != 0)
        return error;
    
    // Read parameters of program //
    error = read_user_input(&image_type,&image_width,&image_height,&image_seed,filter);
    if(error != 0)
        return error;

    /*
    //  Just all procceses print parameters //
    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);

    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

    printf("%d\n",image_type);

    MPI_Finalize();  
*/
    error = free_mem_filter(filter);
    if(error != 0)
        return error;

    return 0;
}

// Sdi1500195 - Charalambous
// Sdi1500129 - Petropoulakis

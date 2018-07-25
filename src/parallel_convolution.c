/***********************************/
/* paraller_convolution.h */

/* Editors: Panagiotis Petropoulakis
 *          Andreas Charalampous
 *
 * A.M:     1115201500129
 *          1115201500195
 *
 * e-mail:  sdi1500129@di.uoa.gr
 *          sdi1500195@di.uoa.gr
 */
/***********************************/
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

    //printf("image_type: %d\nimage_width: %d\nimage_height: %d\nimage_seed: %d\n", image_type,image_width,image_height,image_seed);
    /*
    //  Just all procceses print parameters //
    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);

    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

    printf("%d\n",image_type);

    MPI_Finalize();
*/
    printf("GIVEN FILTER: \n");
    for(int i = 0; i < FILTER_SIZE; i++){
        for(int j = 0; j < FILTER_SIZE; j++){
            printf("%d ",filter[i][j]);
        }
        puts("");
    }

    error = free_mem_filter(filter);
    if(error != 0)
        return error;

    return 0;
}

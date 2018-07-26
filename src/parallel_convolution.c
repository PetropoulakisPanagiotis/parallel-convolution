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
#include <mpi.h>
#include "utils.h"
    
typedef struct arguments{
    int image_type, image_width, image_height, image_seed;
    double filter[FILTER_SIZE][FILTER_SIZE];
}arguments; 

int main(void){
    MPI_Datatype args_type, oldtypes[2], filter_type;
    MPI_Status stat;
    MPI_Aint offsets[2], extent;
    arguments my_args;
    int error;
    int comm_size, my_rank, source=0, dest, tag;
    int block_counts[2];


    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);


    // Define 2-d filter in contiguous derived type //
    MPI_Type_contiguous(FILTER_SIZE,MPI_DOUBLE,&filter_type);
    MPI_Type_commit(&filter_type);
    
    // Setup description of 4 MPI_INT fields - for arguments //
    offsets[0] = 0;
    oldtypes[0] = MPI_INT;
    block_counts[0] = 4;

    // Find offset //
    MPI_Type_extent(MPI_INT,&extent);
    
    // Setup description of 2-d filter - use filter_type //
    offsets[1] = 4 * extent;
    oldtypes[1] = filter_type;
    block_counts[1] = FILTER_SIZE; // 3 X 3 

    MPI_Type_struct(2,block_counts,offsets,oldtypes,&args_type);
    MPI_Type_commit(&args_type);

    if(my_rank == 0){
        int image_type, image_width, image_height, image_seed;
        double **filter;
        int i,j;

        // Create filter //
        error = allocate_mem_filter(&filter);
        if(error != 0)
            return error;

        // Read parameters of program //
        error = read_user_input(&image_type,&image_width,&image_height,&image_seed,filter);
        if(error != 0){
            free_mem_filter(filter);
            return error;
        }
        
        // Copy arguments //
        
        my_args.image_type = image_type;
        my_args.image_width = image_width;
        my_args.image_height = image_height;
        my_args.image_seed = image_seed;
        
        for(i = 0; i < FILTER_SIZE; i++)
            for(j = 0; j < FILTER_SIZE; j++)
                my_args.filter[i][j] = filter[i][j];
        
        error = free_mem_filter(filter);
        if(error != 0)
            return error;
    }

    MPI_Bcast(&my_args,1,args_type,0,MPI_COMM_WORLD);
    
    printf("%d\n",my_args.image_width);

    // Realse resources //
    MPI_Type_free(&filter_type);
    MPI_Type_free(&args_type);

    // Terminate mpi processes //
    MPI_Finalize();

    return 0;
}

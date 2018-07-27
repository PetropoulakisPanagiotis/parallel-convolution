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
#include <math.h>
#include "utils.h"
    
int main(void){
    MPI_Datatype args_type, oldtypes[2], filter_type; /* Define new types */
    MPI_Status stat;
    MPI_Aint offsets[3], extent; /* For derived types */
    Args_type my_args; /* Arguments for current process */
    int comm_size, my_rank, tag, error;
    int block_counts[3];

    /* Initialize MPI environment - Get number of processes etc. */
    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

    /* Define in contiguous derived type - use it for filter */
    MPI_Type_contiguous(FILTER_SIZE,MPI_DOUBLE,&filter_type);
    MPI_Type_commit(&filter_type);
    
    /* Setup description of 4 MPI_INT fields - for arguments */
    offsets[0] = 0;
    oldtypes[0] = MPI_INT;
    block_counts[0] = 4;

    // Find offset of MPI_INIT //
    MPI_Type_extent(MPI_INT,&extent);
    
    // Setup description of 2-d filter - use filter_type //
    offsets[1] = 4 * extent;
    oldtypes[1] = filter_type;
    block_counts[1] = FILTER_SIZE; // 3 X 3 

    // Setup description of 4 MPI_INT fields */
    offsets[2] = 4 * extent + (FILTER_SIZE * FILTER_SIZE);
    oldtypes[2] = MPI_INT;
    block_counts[2] = 4;

    /* Define structure type and commit it*/
    MPI_Type_struct(3,block_counts,offsets,oldtypes,&args_type);
    MPI_Type_commit(&args_type);

    /* Read from user the input and share arguments among all processes */
    if(my_rank == 0){
        Args_type tmp_args;
        int image_type, image_width, image_height, image_seed;
        double filter[FILTER_SIZE][FILTER_SIZE];
        int i,j;

        error = read_user_input(&image_type,&image_width,&image_height,&image_seed,filter);
        if(error != 0)
            MPI_Abort(MPI_COMM_WORLD,error);
        
        /* Copy arguments in our new type */ 
        my_args.image_type = image_type;
        my_args.image_width = image_width;
        my_args.image_height = image_height;
        my_args.image_seed = image_seed;
       
        /* Copy tmp arguments for process 0 */
        tmp_args.image_type = image_type;
        tmp_args.image_width = image_width;
        tmp_args.image_height = image_height;
        tmp_args.image_seed = image_seed;
        
        for(i = 0; i < FILTER_SIZE; i++)
            for(j = 0; j < FILTER_SIZE; j++){
                my_args.filter[i][j] = filter[i][j];
                tmp_args.filter[i][j] = filter[i][j];
           }

        int width_per_process = image_width / (int)sqrt(comm_size);
        int remaining_width = image_width % (int)sqrt(comm_size);
        int height_per_process = image_height / (int)sqrt(comm_size);
        int remaining_height = image_height % (int)sqrt(comm_size);
        
        for(j = 0; j < (int)sqrt(comm_size); j++){
    
            for(i = 0; i < (int)sqrt(comm_size); i++){
                /* Fix width */
                if(j < remaining_width){
                    if(j == 0 && i == 0){
                        tmp_args.my_data_width_start = width_per_process + 1;
                        tmp_args.my_data_width_end = j*(width_per_process + 1);
                        remaining_width--;
                    }
                    else{
                        my_args.my_data_width_start = width_per_process + 1;
                        my_args.my_data_width_end = j*(width_per_process + 1);
                        remaining_width--;
                    }
                }
                else{
                    if(j == 0 && i == 0){
                        tmp_args.my_data_width_start = width_per_process;
                        tmp_args.my_data_width_end = j*(width_per_process);
                        remaining_width--;
                    }
                    else{
                        my_args.my_data_width_start = width_per_process;
                        my_args.my_data_width_end = j*(width_per_process);
                        remaining_width--;
                    }
                }
                
                /* Fix height */
                if(i < remaining_width){
                    if(j == 0 && i == 0){
                        tmp_args.my_data_height_start = height_per_process + 1;
                        tmp_args.my_data_height_end = i*(height_per_process + 1);
                        remaining_height--;
                    }
                    else{
                        my_args.my_data_height_start = height_per_process + 1;
                        my_args.my_data_height_end = i*(height_per_process + 1);
                        remaining_height--;
                    }
                }
                else{
                    if(j == 0 && i == 0){
                        tmp_args.my_data_height_start = height_per_process;
                        tmp_args.my_data_height_end = i*(height_per_process);
                        remaining_height--;
                    }
                    else{
                        my_args.my_data_height_start = height_per_process;
                        my_args.my_data_height_end = i*(height_per_process);
                        remaining_height--;
                    }
                }

                /* Isend ?? */
                if(i != 0 || j != 0){
                    MPI_Send(&my_args,1,args_type,j + (i * (int)sqrt(comm_size)),1,MPI_COMM_WORLD);
                }
            } // End for
        } // End for

        /* Set process 0 arguments -> must change -> copy one by one field  */
        my_args = tmp_args;
    }
    else{
        /* Get arguments */
        MPI_Recv(&my_args,1,args_type,0,1,MPI_COMM_WORLD,&stat);    
        printf("my_id[%d]-width_start:[%d]\n",my_rank,my_args.my_data_width_start);
    }
 
    /* Deallocate data types */
    MPI_Type_free(&filter_type);
    MPI_Type_free(&args_type);

    /* Terminate MPI execution */
    MPI_Finalize();

    return 0;
}

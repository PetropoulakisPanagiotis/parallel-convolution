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
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <stddef.h>
#include "utils.h"

int main(void){
    MPI_Datatype args_type, filter_type, filter_type1; // Define new types
    MPI_Status stat; // for recv
    Args_type my_args; // Arguments of current process
    int comm_size, my_rank, error;

    /* Initialize MPI environment - Get number of processes and rank. */
    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

    int procs_per_line = (int)sqrt(comm_size); // Number of processes in each row/column

    /* Check if perfect square number of processes was provided */
    if(my_rank == 0){
        if(procs_per_line * procs_per_line != comm_size){ // Not perfect square
            printf("Invalid number of processes given. Must be perfect square: 1, 4, 9, 16,...\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    /* Define in contiguous derived type - use it for filter */
    MPI_Type_contiguous(FILTER_SIZE, MPI_DOUBLE, &filter_type1); // 3x1
    MPI_Type_commit(&filter_type1);

    MPI_Type_contiguous(FILTER_SIZE, filter_type1, &filter_type); // 3x3
    MPI_Type_commit(&filter_type);

    /* Create derived MPI type, same as Args_type struct */
    const int items = 9;
    int blocklengths[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Datatype types[9] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, filter_type,
                             MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint offsets[9];

    offsets[0] = offsetof(Args_type, image_type);
    offsets[1] = offsetof(Args_type, image_width);
    offsets[2] = offsetof(Args_type, image_height);
    offsets[3] = offsetof(Args_type, image_seed);
    offsets[4] = offsetof(Args_type, filter);
    offsets[5] = offsetof(Args_type, width_per_process);
    offsets[6] = offsetof(Args_type, width_remaining);
    offsets[7] = offsetof(Args_type, height_per_process);
    offsets[8] = offsetof(Args_type, height_remaining);

    MPI_Type_create_struct(items, blocklengths, offsets, types, &args_type);
    MPI_Type_commit(&args_type);

    /* Read from user the input and share arguments among all processes */
    if(my_rank == 0){

        int i;

        error = read_user_input(&my_args, procs_per_line);
        if(error != 0)
            MPI_Abort(MPI_COMM_WORLD,error);

        /* Calculate remaining arguments */
        my_args.width_per_process = my_args.image_width / (int)sqrt(comm_size);
        my_args.width_remaining = my_args.image_width % (int)sqrt(comm_size);
        my_args.height_per_process = my_args.image_height / (int)sqrt(comm_size);
        my_args.height_remaining = my_args.image_height % (int)sqrt(comm_size);

        printf("{0}t: %d, w:%d, h:%d, s:%d\nF:%lf %lf %lf %lf %lf %lf %lf %lf %lf\nwp: %d, wr: %d, hp: %d, hr: %d, iter: %d\n",
        my_args.image_type,my_args.image_width,my_args.image_height,my_args.image_seed,
        my_args.filter[0][0],my_args.filter[0][1],my_args.filter[0][2],my_args.filter[1][0],my_args.filter[1][1],
        my_args.filter[1][2],my_args.filter[2][0],my_args.filter[2][1],my_args.filter[2][2],my_args.width_per_process,
        my_args.width_remaining, my_args.height_per_process, my_args.height_remaining,my_args.iterations);

        for(i = 1; i < comm_size; i++) // share to all
            MPI_Send(&my_args, 1, args_type, i, 1, MPI_COMM_WORLD);
    }
    else{
        /* Get arguments from process 0*/
        MPI_Recv(&my_args, 1, args_type, 0, 1, MPI_COMM_WORLD, &stat);
        printf("{%d}t: %d, w:%d, h:%d, s:%d\nF:%lf %lf %lf %lf %lf %lf %lf %lf %lf\nwp: %d, wr: %d, hp: %d, hr: %d, iter: %d\n",
        my_rank,my_args.image_type,my_args.image_width,my_args.image_height,my_args.image_seed,
        my_args.filter[0][0],my_args.filter[0][1],my_args.filter[0][2],my_args.filter[1][0],my_args.filter[1][1],
        my_args.filter[1][2],my_args.filter[2][0],my_args.filter[2][1],my_args.filter[2][2],my_args.width_per_process,
        my_args.width_remaining, my_args.height_per_process, my_args.height_remaining,my_args.iterations);

    }

    /* For the next step we must find the 8 neighbours of the current process */
    /* An array with 8 cells will be used in order to keep the neighbours */
    /* 0: N, 1: NE, 2: E, 3: SE, 4: S, 5: SW, 6: W, 7: NW */

    /* The image is divided to processes, so we can imagine that it is a */
    /* (sqrt(comm_size) x sqrt(comm_size)) matrix. Each process, occupies */
    /* one cell, having a (x,y) position, where x is the number of row and */
    /* y the number of column. We need x and y in order to find the */
    /* neighbours of each process, since most of them wont have 8 neighbours */
    /* (occupying cells at the edges) */

    int neighbours[8]; // all neighbours of process

    /*
       0 1 2
       0 1 2
       0 1 2
    */
    int row_id = my_rank % procs_per_line;

    /*
       0 0 0
       1 1 1
       2 2 2
    */
    int column_id = my_rank / procs_per_line;


    /* [N]North Neighbour (0) */
    if(column_id != 0) // if not on top of image
        neighbours[0] = my_rank - procs_per_line;
    else
        neighbours[0] = MPI_PROC_NULL; // no neighbour from North

    /* [NE]North-East Neighbour (1) */
    if(column_id != 0 && row_id != procs_per_line - 1) // if not on right up corner
        neighbours[1] = my_rank - procs_per_line + 1;
    else
        neighbours[1] = MPI_PROC_NULL;

    /* [E]East Neighbour (2) */
    if(row_id != procs_per_line - 1) // if not on the right edge
        neighbours[2] = my_rank + 1;
    else
        neighbours[2] = MPI_PROC_NULL;

    /* [SE]South-East Neighbour (3) */
    if(column_id != procs_per_line - 1 && row_id != procs_per_line - 1) // if not on the right down corner
        neighbours[3] = my_rank + procs_per_line + 1;
    else
        neighbours[3] = MPI_PROC_NULL;

    /* [S]South Neighbour (4) */
    if(column_id != procs_per_line - 1) // if not on bottom of image
        neighbours[4] = my_rank + procs_per_line;
    else
        neighbours[4] = MPI_PROC_NULL; // no neighbour from South

    /* [SW]South-West Neighbour (5) */
    if(column_id != procs_per_line -1 && row_id != 0) // if not on left down corner
        neighbours[5] = my_rank + procs_per_line - 1;
    else
        neighbours[5] = MPI_PROC_NULL;

    /* [W]West Neighbour (6) */
    if(row_id != 0) // if not on the left edge
        neighbours[6] = my_rank - 1;
    else
        neighbours[6] = MPI_PROC_NULL;

    /* [NW]North-West Neighbour (7) */
    if(row_id != 0 && column_id != 0) // if not on left up corner
        neighbours[7] = my_rank - procs_per_line - 1;
    else
        neighbours[7] = MPI_PROC_NULL;

    printf("[%d]{%d,%d} %d %d %d %d %d %d %d %d\n\n",my_rank,row_id,column_id,
                                                     neighbours[0],
                                                     neighbours[1],
                                                     neighbours[2],
                                                     neighbours[3],
                                                     neighbours[4],
                                                     neighbours[5],
                                                     neighbours[6],
                                                     neighbours[7]);


    /* The resolution of the image that each process has, must be found */
    int my_width;
    int my_height;

    /* If width or height is not perfectly divided into processes, share */
    /* the n remaining pixels to the first n processes */
    if(row_id < my_args.width_remaining)
        my_width = my_args.width_per_process + 1;
    else
        my_width = my_args.width_per_process;

    if(column_id < my_args.height_remaining)
        my_height = my_args.height_per_process + 1;
    else
        my_height = my_args.height_per_process;

    /* For random images, set the seed differently to each process, in order */
    /* to have a fully random image and not repetitive cells */

    srand( my_args.image_seed * ((my_rank + 333) * (my_rank + 333)) );

    /* Create array that will hold all pixels and generate random image */
    int** my_image = (int**)malloc(sizeof(int*) * my_height); // create rows
    for(int i = 0; i < my_height; i++){
        my_image[i] = (int*)malloc(sizeof(int) * my_width); // create columns
    }

//////////////////////////////////////////////////////////////////////
/* Test if image is correct

    char fileName[10];
    sprintf(fileName,"File%d",my_rank);

    FILE* my_file = fopen(fileName, "w");

    for(int i = 0; i < my_height; i++){
        for(int j = 0; j < my_width; j++){
            my_image[i][j] = rand() % 256;
            fprintf(my_file, "%d ", my_image[i][j]);
        }
        fprintf(my_file, "\n");
    }

*/
///////////////////////////////////////////////////////////////////////


    /* Deallocate data types */
    MPI_Type_free(&filter_type);
    MPI_Type_free(&filter_type1);
    MPI_Type_free(&args_type);

    /* Terminate MPI execution */
    MPI_Finalize();

    return 0;
}

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
    MPI_Status stat; // For communication
    Args_type my_args; // Arguments of current process
    int comm_size, my_rank, error;
    int i, j, k, iter, index;

    /* Initialize MPI environment - Get number of processes and rank. */
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int procs_per_line = (int)sqrt(comm_size); // Number of processes in each row/column
    int procs_per_line_1 = procs_per_line - 1; // Frequent var

    /* Check if number of processes is a perfect square */
    if(my_rank == 0){
        if(procs_per_line * procs_per_line != comm_size){
            printf("Invalid number of processes given. Must be a perfect square: 1, 4, 9, 16,...\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    /* Define in contiguous derived type - use it for filter */
    MPI_Type_contiguous(FILTER_SIZE, MPI_DOUBLE, &filter_type1); // 3x1
    MPI_Type_commit(&filter_type1);

    MPI_Type_contiguous(FILTER_SIZE, filter_type1, &filter_type); // 3x3
    MPI_Type_commit(&filter_type);

    /* Create derived MPI type for Args_type struct */
    const int items = 10;
    int blocklengths[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1,1};
    MPI_Datatype types[10] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, filter_type,
                             MPI_INT, MPI_INT, MPI_INT, MPI_INT,MPI_INT};
    MPI_Aint offsets[10];

    offsets[0] = offsetof(Args_type, image_type);
    offsets[1] = offsetof(Args_type, image_width);
    offsets[2] = offsetof(Args_type, image_height);
    offsets[3] = offsetof(Args_type, image_seed);
    offsets[4] = offsetof(Args_type, filter);
    offsets[5] = offsetof(Args_type, width_per_process);
    offsets[6] = offsetof(Args_type, width_remaining);
    offsets[7] = offsetof(Args_type, height_per_process);
    offsets[8] = offsetof(Args_type, height_remaining);
    offsets[9] = offsetof(Args_type, iterations);

    MPI_Type_create_struct(items, blocklengths, offsets, types, &args_type);
    MPI_Type_commit(&args_type);

    /* Read the user input and share arguments among all processes */
    if(my_rank == 0){

        error = read_user_input(&my_args, procs_per_line);
        if(error != 0)
            MPI_Abort(MPI_COMM_WORLD, error);

        /* Calculate the limits for every images */
        my_args.width_per_process = my_args.image_width / (int)sqrt(comm_size);
        my_args.width_remaining = my_args.image_width % (int)sqrt(comm_size);
        my_args.height_per_process = my_args.image_height / (int)sqrt(comm_size);
        my_args.height_remaining = my_args.image_height % (int)sqrt(comm_size);

        /* Print all arguments found
        printf("{0}t: %d, w:%d, h:%d, s:%d\nF:%lf %lf %lf %lf %lf %lf %lf %lf %lf\nwp: %d, wr: %d, hp: %d, hr: %d, iter: %d\n",
        my_args.image_type,my_args.image_width,my_args.image_height,my_args.image_seed,
        my_args.filter[0][0],my_args.filter[0][1],my_args.filter[0][2],my_args.filter[1][0],my_args.filter[1][1],
        my_args.filter[1][2],my_args.filter[2][0],my_args.filter[2][1],my_args.filter[2][2],my_args.width_per_process,
        my_args.width_remaining, my_args.height_per_process, my_args.height_remaining,my_args.iterations);
        */

        /* Send arguments in other proccesses */
        for(i = 1; i < comm_size; i++)
            MPI_Send(&my_args, 1, args_type, i, 1, MPI_COMM_WORLD);
    }
    else{

        /* Get arguments from process 0*/
        MPI_Recv(&my_args, 1, args_type, 0, 1, MPI_COMM_WORLD, &stat);

        /* Print arguments received
        printf("{%d}t: %d, w:%d, h:%d, s:%d\nF:%lf %lf %lf %lf %lf %lf %lf %lf %lf\nwp: %d, wr: %d, hp: %d, hr: %d, iter: %d\n",
        my_rank,my_args.image_type,my_args.image_width,my_args.image_height,my_args.image_seed,
        my_args.filter[0][0],my_args.filter[0][1],my_args.filter[0][2],my_args.filter[1][0],my_args.filter[1][1],
        my_args.filter[1][2],my_args.filter[2][0],my_args.filter[2][1],my_args.filter[2][2],my_args.width_per_process,
        my_args.width_remaining, my_args.height_per_process, my_args.height_remaining,my_args.iterations);
        */
    }

    /* For the next step find the 8 neighbours of the current process     */
    /* An array with 8 cells will be used in order to keep the neighbours */
    /* 0: N, 1: NE, 2: E, 3: SE, 4: S, 5: SW, 6: W, 7: NW                 */

    /* The image is divided to the processes, so we can imagine that it is a */
    /* (sqrt(comm_size) x sqrt(comm_size)) matrix. Each process, occupies    */
    /* one cell, having a (x,y) position, where x is the number of row and   */
    /* y the number of column. We need x and y in order to find the          */
    /* neighbours of each process, since most of them wont have 8 neighbours */
    /* (occupying cells at the edges) */

    int neighbours[NUM_NEIGHBOURS]; // Keep neighbours

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
    if(column_id != 0) // If not on top of image
        neighbours[N] = my_rank - procs_per_line;
    else
        neighbours[N] = MPI_PROC_NULL; // No neighbour from North

    /* [NE]North-East Neighbour (1) */
    if(column_id != 0 && row_id != procs_per_line - 1) // If not on right up corner
        neighbours[NE] = my_rank - procs_per_line + 1;
    else
        neighbours[NE] = MPI_PROC_NULL;

    /* [E]East Neighbour (2) */
    if(row_id != procs_per_line - 1) // If not on the right edge
        neighbours[E] = my_rank + 1;
    else
        neighbours[E] = MPI_PROC_NULL;

    /* [SE]South-East Neighbour (3) */
    if(column_id != procs_per_line - 1 && row_id != procs_per_line - 1) // If not on the right down corner
        neighbours[SE] = my_rank + procs_per_line + 1;
    else
        neighbours[SE] = MPI_PROC_NULL;

    /* [S]South Neighbour (4) */
    if(column_id != procs_per_line - 1) // If not on bottom of image
        neighbours[S] = my_rank + procs_per_line;
    else
        neighbours[S] = MPI_PROC_NULL; // No neighbour from South

    /* [SW]South-West Neighbour (5) */
    if(column_id != procs_per_line -1 && row_id != 0) // If not on left down corner
        neighbours[SW] = my_rank + procs_per_line - 1;
    else
        neighbours[SW] = MPI_PROC_NULL;

    /* [W]West Neighbour (6) */
    if(row_id != 0) // If not on the left edge
        neighbours[W] = my_rank - 1;
    else
        neighbours[W] = MPI_PROC_NULL;

    /* [NW]North-West Neighbour (7) */
    if(row_id != 0 && column_id != 0) // If not on left up corner
        neighbours[NW] = my_rank - procs_per_line - 1;
    else
        neighbours[NW] = MPI_PROC_NULL;

    /*
    printf("[%d]{%d,%d} %d %d %d %d %d %d %d %d\n\n",my_rank,row_id,column_id,
                                                     neighbours[0],
                                                     neighbours[1],
                                                     neighbours[2],
                                                     neighbours[3],
                                                     neighbours[4],
                                                     neighbours[5],
                                                     neighbours[6],
                                                     neighbours[7]);

    */

    /* The resolution of the image that each process has - Add some frequent vars */
    int my_width, my_width_incr_1, my_width_decr_1, my_width_incr_2;
    int my_height, my_height_incr_1, my_height_decr_1, my_height_incr_2;

    /* If width or height is not perfectly divided into processes, share */
    /* the n remaining pixels to the first n processes                   */
    if(row_id < my_args.width_remaining)
        my_width = my_args.width_per_process + 1;
    else
        my_width = my_args.width_per_process;

    if(column_id < my_args.height_remaining)
        my_height = my_args.height_per_process + 1;
    else
        my_height = my_args.height_per_process;

    /* Fix frequent sums into new variables(height and width of image including hallow points etc) */
    my_width_incr_1 = my_width + 1;
    my_height_incr_1 = my_height + 1;

    my_width_incr_2 = my_width_incr_1 + 1;
    my_height_incr_2 = my_height_incr_1 + 1;

    my_width_decr_1 = my_width - 1;
    my_height_decr_1 = my_height - 1;

    /* For random images, set the seed differently to each process, in order */
    /* to have a fully random image and not repetitive cells                 */

    srand(my_args.image_seed * ((my_rank + 333) * (my_rank + 333)));

    /* Create array that will hold all pixels and generate a random image           */
    /* Add two rows and two collumns as "hallow points" -> Keep neighrous pixels    */
    /* Note: Allocate image with a way that array has a constant offset in collumns */
    int** my_image_before, **my_image_after, *tmp_ptr; // Tmp for swapping

    /* Allocate pointers for height */
    my_image_before = malloc((my_height_incr_2) * sizeof(int*));
    if(my_image_before == NULL)
        MPI_Abort(MPI_COMM_WORLD, error);

    /* Allocate a contiguous array */
    my_image_before[0] = malloc((my_height_incr_2) * (my_width_incr_2) * sizeof(int));
    if(my_image_before[0] == NULL)
        MPI_Abort(MPI_COMM_WORLD, error);

    /* Fix array(rows) */
    for(i = 1; i < (my_height_incr_2); i++)
        my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

    /* Fill initial image with random numbers */
    for(i = 1; i <  my_height_incr_1; i++)
        for(j = 1; j < my_width_incr_1; j++)
            my_image_before[i][j] = rand() % 256;

    /* Set edges(hallow points, -1 for now, until neighbours send theirs) */
    for(i = 0; i < my_height_incr_2; i++){
        my_image_before[i][0] = -1;
        my_image_before[i][my_width_incr_1] = -1;
    }

    for(j = 0; j < my_width_incr_2; j++){
        my_image_before[0][j] = -1;
        my_image_before[my_height_incr_1][j] = -1;
    }

    /* Allocate an image to save the result */
    my_image_after = malloc((my_height_incr_2) * sizeof(int*));
    if(my_image_after == NULL)
        MPI_Abort(MPI_COMM_WORLD, error);

    /* Allocate a contiguous array */
    my_image_after[0] = malloc((my_height_incr_2) * (my_width_incr_2) * sizeof(int));
    if(my_image_after[0] == NULL)
        MPI_Abort(MPI_COMM_WORLD, error);
    
    /* Fix array */
    for(i = 1; i < (my_height_incr_2); i++)
        my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);

    for(i = 0; i <  my_height_incr_2; i++)
        for(j = 0; j < my_width_incr_2; j++)
            my_image_after[i][j] = 0;

    char fileName[10] = "";
    sprintf(fileName,"File%dA",my_rank);

    FILE* my_file = fopen(fileName, "w");

    for(i = 0; i < my_height_incr_2; i++){
        for(j = 0; j < my_width_incr_2; j++){
            fprintf(my_file, "%d\t", my_image_before[i][j]);
        }
        fprintf(my_file, "\n");
    }

    fclose(my_file);
    
    /* Set columns type for sending columns East and West */
    MPI_Datatype column_type;
    MPI_Type_vector(my_height, 1, my_width_incr_2, MPI_INT, &column_type);
    MPI_Type_commit(&column_type);

    /* Initialize communication with neighbours */
    MPI_Request send_requests[NUM_NEIGHBOURS];
    MPI_Request recv_requests[NUM_NEIGHBOURS];

    /* Send to each neighbour, tagging it with the opposite direction of the receiving process(eg N->S, SW -> NE) */
    MPI_Send_init(&my_image_before[1][1], my_width, MPI_INT, neighbours[N], S, MPI_COMM_WORLD, &send_requests[N]);
    MPI_Send_init(&my_image_before[1][my_width], 1, MPI_INT, neighbours[NE], SW,MPI_COMM_WORLD, &send_requests[NE]);
    MPI_Send_init(&my_image_before[1][my_width], 1, column_type, neighbours[E], W, MPI_COMM_WORLD, &send_requests[E]);
    MPI_Send_init(&my_image_before[my_height][my_width], 1, MPI_INT, neighbours[SE], NW, MPI_COMM_WORLD, &send_requests[SE]);
    MPI_Send_init(&my_image_before[my_height][1], my_width, MPI_INT, neighbours[S], N, MPI_COMM_WORLD, &send_requests[S]);
    MPI_Send_init(&my_image_before[my_height][1], 1, MPI_INT, neighbours[SW], NE, MPI_COMM_WORLD, &send_requests[SW]);
    MPI_Send_init(&my_image_before[1][1], 1, column_type, neighbours[W], E, MPI_COMM_WORLD, &send_requests[W]);
    MPI_Send_init(&my_image_before[1][1], 1, MPI_INT, neighbours[NW], SE, MPI_COMM_WORLD, &send_requests[NW]);

    /* Receive from all neighbours */
    MPI_Recv_init(&my_image_before[0][1], my_width, MPI_INT, neighbours[N], N, MPI_COMM_WORLD, &recv_requests[N]);
    MPI_Recv_init(&my_image_before[0][my_width_incr_1], 1, MPI_INT, neighbours[NE], NE, MPI_COMM_WORLD, &recv_requests[NE]);
    MPI_Recv_init(&my_image_before[1][my_width_incr_1], 1, column_type, neighbours[E], E, MPI_COMM_WORLD, &recv_requests[E]);
    MPI_Recv_init(&my_image_before[my_height_incr_1][my_width_incr_1], 1, MPI_INT, neighbours[SE], SE, MPI_COMM_WORLD, &recv_requests[SE]);
    MPI_Recv_init(&my_image_before[my_height_incr_1][1], my_width, MPI_INT, neighbours[S], S, MPI_COMM_WORLD, &recv_requests[S]);
    MPI_Recv_init(&my_image_before[my_height_incr_1][0], 1, MPI_INT, neighbours[SW],SW,MPI_COMM_WORLD,&recv_requests[SW]);
    MPI_Recv_init(&my_image_before[1][0], 1, column_type, neighbours[W], W, MPI_COMM_WORLD, &recv_requests[W]);
    MPI_Recv_init(&my_image_before[0][0], 1, MPI_INT, neighbours[NW], NW, MPI_COMM_WORLD, &recv_requests[NW]);

    /* Note for flags    */
    /* ur -> upper right */
    /* ul -> upper left  */
    /* ll -> lower left  */
    /* lr -> lower right */

    /* Only one process */
    if(comm_size == 1){
        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){
 
            //////////////////////////////////
            /* Convolute all pixels at once */
            //////////////////////////////////
            for(i = 1; i < my_height_incr_1; i++){ // For every inner row
                for(j = 1; j < my_width_incr_1; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < my_height_incr_2; i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End for iter

        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);
    } // End if comm_size == 1
    /* Left upper process - active neighbours E, SE, S */
    else if(my_rank == 0){

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* Left column - except upper and lower left corners */
            for(i = 2; i < my_height; i++){
                my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                        my_image_before[i - 1][1] * my_args.filter[0][1] +
                                        my_image_before[i - 1][2] * my_args.filter[0][2] +
                                        my_image_before[i][2] * my_args.filter[1][2] +
                                        my_image_before[i + 1][2] * my_args.filter[2][2] +
                                        my_image_before[i + 1][1] * my_args.filter[2][1]);

                /* Truncated unexpected values */
                if(my_image_after[i][1] < 0)
                    my_image_after[i][1] = 0;
                else if(my_image_after[i][1] > 255)
                    my_image_after[i][1] = 255;
            } // End for

            /* Left upper corner */
            my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                    my_image_before[1][2] * my_args.filter[1][2] +
                                    my_image_before[2][2] * my_args.filter[2][2] +
                                    my_image_before[2][1] * my_args.filter[2][1]);

            /* Truncated unexpected values */
            if(my_image_after[1][1] < 0)
                my_image_after[1][1] = 0;
            else if(my_image_after[1][1] > 255)
                my_image_after[1][1] = 255;

            /* First line - except left and right upper corners */
            for(j = 2; j < my_width; j++){
                my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                        my_image_before[1][j + 1] * my_args.filter[1][2] +
                                        my_image_before[2][j + 1] * my_args.filter[2][2] +
                                        my_image_before[2][j] * my_args.filter[2][1] +
                                        my_image_before[2][j - 1] * my_args.filter[2][0] +
                                        my_image_before[1][j - 1] * my_args.filter[1][0]);

                /* Truncated unexpected values */
                if(my_image_after[1][j] < 0)
                    my_image_after[1][j] = 0;
                else if(my_image_after[1][j] > 255)
                    my_image_after[1][j] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
                
                /* Convolute right column and right upper corner */
                if(recv_stat.MPI_TAG == E){
                    /* Right column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[i - 1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[i][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[i + 1][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                        my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][my_width] < 0)
                            my_image_after[i][my_width] = 0;
                        else if(my_image_after[i][my_width] > 255)
                            my_image_after[i][my_width] = 255;
                    } // End for

                    /* Right upper corner */
                    my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                    my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                    my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                    my_image_before[2][my_width] * my_args.filter[2][1] +
                                                    my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                    my_image_before[1][my_width_decr_1] * my_args.filter[1][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[1][my_width] < 0)
                        my_image_after[1][my_width] = 0;
                    else if(my_image_after[1][my_width] > 255)
                        my_image_after[1][my_width] = 255;
                } // End if E 
                /* Convolute last line and left lower corner */
                else if(recv_stat.MPI_TAG == S){
                    /* Last line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][j + 1] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][j] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][j - 1] * my_args.filter[2][0] +
                                                        my_image_before[my_height][j - 1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[my_height][j] < 0)
                            my_image_after[my_height][j] = 0;
                        else if(my_image_after[my_height][j] > 255)
                            my_image_after[my_height][j] = 255;
                    } // End for

                    /* Left lower corner */
                    my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                    my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                    my_image_before[my_height][2] * my_args.filter[1][2] +
                                                    my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                    my_image_before[my_height_incr_1][1] * my_args.filter[2][1]);

                    /* Truncated unexpected values */
                    if(my_image_after[my_height][1] < 0)
                        my_image_after[my_height][1] = 0;
                    else if(my_image_after[my_height][1] > 255)
                        my_image_after[my_height][1] = 255;
                } // End if S
            } // End for - Wait any
            
            /* Convolute right lower corner */
            my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                    my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                    my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                    my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                    my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                    my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                    my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                    my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
            /* Truncated unexpected values */
            if(my_image_after[my_height][my_width] < 0)
                my_image_after[my_height][my_width] = 0;
            else if(my_image_after[my_height][my_width] > 255)
                my_image_after[my_height][my_width] = 255;
            
            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < my_height_incr_2; i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter

        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);        
        FILE* my_file = fopen(fileName, "w");

        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        fclose(my_file);
    
    } // End if a)
    /* Right upper process - active neighbours S, SW, W */
    else if(my_rank == procs_per_line_1){

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* Right column - except from upper and lower right corners */
            for(i = 2; i < my_height; i++){
                my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[i][my_width] < 0)
                    my_image_after[i][my_width] = 0;
                else if(my_image_after[i][my_width] > 255)
                    my_image_after[i][my_width] = 255;
            } // End for

            /* Right upper corner */
            my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                            my_image_before[2][my_width] * my_args.filter[2][1] +
                                            my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                            my_image_before[1][my_width_decr_1] * my_args.filter[1][0]);

            /* Truncated unexpected values */
            if(my_image_after[1][my_width] < 0)
                my_image_after[1][my_width] = 0;
            else if(my_image_after[1][my_width] > 255)
                my_image_after[1][my_width] = 255;

            /* First line - except left and right upper corners */
            for(j = 2; j < my_width; j++){
                my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                        my_image_before[1][j + 1] * my_args.filter[1][2] +
                                        my_image_before[2][j + 1] * my_args.filter[2][2] +
                                        my_image_before[2][j] * my_args.filter[2][1] +
                                        my_image_before[2][j - 1] * my_args.filter[2][0] +
                                        my_image_before[1][j - 1] * my_args.filter[1][0]);

                /* Truncated unexpected values */
                if(my_image_after[1][j] < 0)
                    my_image_after[1][j] = 0;
                else if(my_image_after[1][j] > 255)
                    my_image_after[1][j] = 255;
            } // End for
            
            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
        
                /* Convolute left column and left upper corner */
                if(recv_stat.MPI_TAG == W){
                    /* Left column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                                my_image_before[i - 1][1] * my_args.filter[0][1] +
                                                my_image_before[i - 1][2] * my_args.filter[0][2] +
                                                my_image_before[i][2] * my_args.filter[1][2] +
                                                my_image_before[i + 1][2] * my_args.filter[2][2] +
                                                my_image_before[i + 1][1] * my_args.filter[2][1] +
                                                my_image_before[i + 1][0] * my_args.filter[2][0] +
                                                my_image_before[i][0] * my_args.filter[1][0] +
                                                my_image_before[i - 1][0] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][1] < 0)
                            my_image_after[i][1] = 0;
                        else if(my_image_after[i][1] > 255)
                            my_image_after[i][1] = 255;
                    } // End for

                    /* Left upper corner */
                    my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                            my_image_before[1][2] * my_args.filter[1][2] +
                                            my_image_before[2][2] * my_args.filter[2][2] +
                                            my_image_before[2][1] * my_args.filter[2][1] +
                                            my_image_before[2][0] * my_args.filter[2][0] +
                                            my_image_before[1][0] * my_args.filter[1][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[1][1] < 0)
                        my_image_after[1][1] = 0;
                    else if(my_image_after[1][1] > 255)
                        my_image_after[1][1] = 255;
                } // End if W 
                /* Convolute last line and right lower corner */
                else if(recv_stat.MPI_TAG == S){
                    /* Last line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][j + 1] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][j] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][j - 1] * my_args.filter[2][0] +
                                                        my_image_before[my_height][j - 1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[my_height][j] < 0)
                            my_image_after[my_height][j] = 0;
                        else if(my_image_after[my_height][j] > 255)
                            my_image_after[my_height][j] = 255;
                    } // End for

                    /* Right lower corner */
                    my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                            my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                            my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                            my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                            my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                            my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                    /* Truncated unexpected values */
                    if(my_image_after[my_height][my_width] < 0)
                        my_image_after[my_height][my_width] = 0;
                    else if(my_image_after[my_height][my_width] > 255)
                        my_image_after[my_height][my_width] = 255;
                } // End if S
            } // End for - Wait any

            /* Convolute left lower corner */
            my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                            my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                            my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                            my_image_before[my_height][2] * my_args.filter[1][2] +
                                            my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                            my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                            my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                            my_image_before[my_height][0] * my_args.filter[1][0] +
                                            my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
            /* Truncated unexpected values */
            if(my_image_after[my_height][1] < 0)
                my_image_after[my_height][1] = 0;
            else if(my_image_after[my_height][1] > 255)
                my_image_after[my_height][1] = 255;

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < my_height_incr_2; i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);

    } // End if b)
    /* Right lower process - active neighbours W, NW, N */
    else if(my_rank == comm_size - 1){

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* Right column - except from upper and lower right corners */
            for(i = 2; i < my_height; i++){
                my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[i][my_width] < 0)
                    my_image_after[i][my_width] = 0;
                else if(my_image_after[i][my_width] > 255)
                    my_image_after[i][my_width] = 255;
            } // End for

            /* Right lower corner */
            my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                    my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                    my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);

            /* Truncated unexpected values */
            if(my_image_after[my_height][my_width] < 0)
                my_image_after[my_height][my_width] = 0;
            else if(my_image_after[my_height][my_width] > 255)
                my_image_after[my_height][my_width] = 255;

            /* Last line - except from left and right lower corners */
            for(j = 2; j < my_width; j++){
                my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                my_image_before[my_height][j - 1] * my_args.filter[2][0] +
                                                my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[my_height][j] < 0)
                    my_image_after[my_height][j] = 0;
                else if(my_image_after[my_height][j] > 255)
                    my_image_after[my_height][j] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);

                /* Convolute left column and left lower corner */
                if(recv_stat.MPI_TAG == W){
                    /* Left column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                                my_image_before[i - 1][1] * my_args.filter[0][1] +
                                                my_image_before[i - 1][2] * my_args.filter[0][2] +
                                                my_image_before[i][2] * my_args.filter[1][2] +
                                                my_image_before[i + 1][2] * my_args.filter[2][2] +
                                                my_image_before[i + 1][1] * my_args.filter[2][1] +
                                                my_image_before[i + 1][0] * my_args.filter[2][0] +
                                                my_image_before[i][0] * my_args.filter[1][0] +
                                                my_image_before[i - 1][0] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][1] < 0)
                            my_image_after[i][1] = 0;
                        else if(my_image_after[i][1] > 255)
                            my_image_after[i][1] = 255;
                    } // End for

                    /* Left lower corner */
                    my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                    my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                    my_image_before[my_height][2] * my_args.filter[1][2] +
                                                    my_image_before[my_height][0] * my_args.filter[1][0] +
                                                    my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
                    
                    /* Truncated unexpected values */
                    if(my_image_after[my_height][1] < 0)
                        my_image_after[my_height][1] = 0;
                    else if(my_image_after[my_height][1] > 255)
                        my_image_after[my_height][1] = 255;
                } // End if W 
                /* Convolute first line and right upper corner */
                else if(recv_stat.MPI_TAG == N){
                    /* First line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                                my_image_before[0][j] * my_args.filter[0][1] +
                                                my_image_before[0][j + 1] * my_args.filter[0][2] +
                                                my_image_before[1][j + 1] * my_args.filter[1][2] +
                                                my_image_before[2][j + 1] * my_args.filter[2][2] +
                                                my_image_before[2][j] * my_args.filter[2][1] +
                                                my_image_before[2][j - 1] * my_args.filter[2][0] +
                                                my_image_before[1][j - 1] * my_args.filter[1][0] +
                                                my_image_before[0][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[1][j] < 0)
                            my_image_after[1][j] = 0;
                        else if(my_image_after[1][j] > 255)
                            my_image_after[1][j] = 255;
                    } // End for

                    /* Right upper corner */
                    my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                    my_image_before[0][my_width] * my_args.filter[0][1] +
                                                    my_image_before[2][my_width] * my_args.filter[2][1] +
                                                    my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                    my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                    my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
                    /* Truncated unexpected values */
                    if(my_image_after[1][my_width] < 0)
                        my_image_after[1][my_width] = 0;
                    else if(my_image_after[1][my_width] > 255)
                        my_image_after[1][my_width] = 255;
                } // End if N
            } // End for - Wait any

            /* Convolute left upper corner */
            my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                    my_image_before[0][1] * my_args.filter[0][1] +
                                    my_image_before[0][2] * my_args.filter[0][2] +
                                    my_image_before[1][2] * my_args.filter[1][2] +
                                    my_image_before[2][2] * my_args.filter[2][2] +
                                    my_image_before[2][1] * my_args.filter[2][1] +
                                    my_image_before[2][0] * my_args.filter[2][0] +
                                    my_image_before[1][0] * my_args.filter[1][0] +
                                    my_image_before[0][0] * my_args.filter[0][0]);
            
            /* Truncated unexpected values */
            if(my_image_after[1][1] < 0)
                my_image_after[1][1] = 0;
            else if(my_image_after[1][1] > 255)
                my_image_after[1][1] = 255;

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_incr_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);
    } // End if c)
    /* Left lower process - active neighbours N, NE, E */
    else if(my_rank == comm_size - procs_per_line){

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* Left column - except upper and lower left corners */
            for(i = 2; i < my_height; i++){
                my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                        my_image_before[i - 1][1] * my_args.filter[0][1] +
                                        my_image_before[i - 1][2] * my_args.filter[0][2] +
                                        my_image_before[i][2] * my_args.filter[1][2] +
                                        my_image_before[i + 1][2] * my_args.filter[2][2] +
                                        my_image_before[i + 1][1] * my_args.filter[2][1]);

                /* Truncated unexpected values */
                if(my_image_after[i][1] < 0)
                    my_image_after[i][1] = 0;
                else if(my_image_after[i][1] > 255)
                    my_image_after[i][1] = 255;
            } // End for

            /* Left lower corner */
            my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                            my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                            my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                            my_image_before[my_height][2] * my_args.filter[1][2]);

            /* Truncated unexpected values */
            if(my_image_after[my_height][1] < 0)
                my_image_after[my_height][1] = 0;
            else if(my_image_after[my_height][1] > 255)
                my_image_after[my_height][1] = 255;

            /* Last line - except from left and right lower corners */
            for(j = 2; j < my_width; j++){
                my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                my_image_before[my_height][j - 1] * my_args.filter[2][0] +
                                                my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[my_height][j] < 0)
                    my_image_after[my_height][j] = 0;
                else if(my_image_after[my_height][j] > 255)
                    my_image_after[my_height][j] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
                
                /* Convolute right column and right lower corner */
                if(recv_stat.MPI_TAG == E){
                    /* Right column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[i - 1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[i][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[i + 1][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                        my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][my_width] < 0)
                            my_image_after[i][my_width] = 0;
                        else if(my_image_after[i][my_width] > 255)
                            my_image_after[i][my_width] = 255;
                    } // End for

                    /* Right lower corner */
                    my_image_after[my_height][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                    
                    /* Truncated unexpected values */
                    if(my_image_after[my_height][my_width] < 0)
                        my_image_after[my_height][my_width] = 0;
                    else if(my_image_after[my_height][my_width] > 255)
                        my_image_after[my_height][my_width] = 255;
                } // End if E
                /* Convolute first line and left upper corner */
                else if(recv_stat.MPI_TAG == N){
                    /* First line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                                my_image_before[0][j] * my_args.filter[0][1] +
                                                my_image_before[0][j + 1] * my_args.filter[0][2] +
                                                my_image_before[1][j + 1] * my_args.filter[1][2] +
                                                my_image_before[2][j + 1] * my_args.filter[2][2] +
                                                my_image_before[2][j] * my_args.filter[2][1] +
                                                my_image_before[2][j - 1] * my_args.filter[2][0] +
                                                my_image_before[1][j - 1] * my_args.filter[1][0] +
                                                my_image_before[0][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[1][j] < 0)
                            my_image_after[1][j] = 0;
                        else if(my_image_after[1][j] > 255)
                            my_image_after[1][j] = 255;
                    } // End for
                    
                    /* Left upper corner */
                    my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                            my_image_before[0][1] * my_args.filter[0][1] +
                                            my_image_before[0][2] * my_args.filter[0][2] +
                                            my_image_before[1][2] * my_args.filter[1][2] +
                                            my_image_before[2][2] * my_args.filter[2][2] +
                                            my_image_before[2][1] * my_args.filter[2][1]);
                    
                    /* Truncated unexpected values */
                    if(my_image_after[1][1] < 0)
                        my_image_after[1][1] = 0;
                    else if(my_image_after[1][1] > 255)
                        my_image_after[1][1] = 255;
                } // End if N
            } // End for - Wait any

            /* Convolute right upper corner */
            my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                            my_image_before[0][my_width] * my_args.filter[0][1] +
                                            my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                            my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                            my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                            my_image_before[2][my_width] * my_args.filter[2][1] +
                                            my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                            my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                            my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
            /* Truncated unexpected values */
            if(my_image_after[1][my_width] < 0)
                my_image_after[1][my_width] = 0;
            else if(my_image_after[1][my_width] > 255)
                my_image_after[1][my_width] = 255;

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];
    
            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_incr_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);
    } // End if d)
    /* First line processes - except from first and last process in this line - active neighbours E, SE , S, SW, W */
    else if(my_rank < procs_per_line_1){
        int flag_corner_ll = 0, flag_corner_lr = 0; // Flag == 3, convolute corners  

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* First line - except left and right upper corners */
            for(j = 2; j < my_width; j++){
                my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                        my_image_before[1][j + 1] * my_args.filter[1][2] +
                                        my_image_before[2][j + 1] * my_args.filter[2][2] +
                                        my_image_before[2][j] * my_args.filter[2][1] +
                                        my_image_before[2][j - 1] * my_args.filter[2][0] +
                                        my_image_before[1][j - 1] * my_args.filter[1][0]);

                /* Truncated unexpected values */
                if(my_image_after[1][j] < 0)
                    my_image_after[1][j] = 0;
                else if(my_image_after[1][j] > 255)
                    my_image_after[1][j] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
    
                /* Convolute right column, right upper corner and right lower corner*/
                if(recv_stat.MPI_TAG == E){
                    flag_corner_lr += 1;

                    /* Right column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[i - 1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[i][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[i + 1][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                        my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][my_width] < 0)
                            my_image_after[i][my_width] = 0;
                        else if(my_image_after[i][my_width] > 255)
                            my_image_after[i][my_width] = 255;
                    } // End for

                    /* Right upper corner */
                    my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                    my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                    my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                    my_image_before[2][my_width] * my_args.filter[2][1] +
                                                    my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                    my_image_before[1][my_width_decr_1] * my_args.filter[1][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[1][my_width] < 0)
                        my_image_after[1][my_width] = 0;
                    else if(my_image_after[1][my_width] > 255)
                        my_image_after[1][my_width] = 255;

                    /* Convolute right lower corner */
                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if E 
                /* Check if it is possible to convolute right lower corner */
                else if(recv_stat.MPI_TAG == SE){
                    flag_corner_lr += 1;

                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if SE
                /* Convolute last line, left lower corner and right lower corner */
                else if(recv_stat.MPI_TAG == S){
                    flag_corner_ll += 1;
                    flag_corner_lr += 1;

                    /* Last line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][j + 1] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][j] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][j - 1] * my_args.filter[2][0] +
                                                        my_image_before[my_height][j - 1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[my_height][j] < 0)
                            my_image_after[my_height][j] = 0;
                        else if(my_image_after[my_height][j] > 255)
                            my_image_after[my_height][j] = 255;
                    } // End for

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner 

                    /* Convolute right lower corner */
                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if S
                /* Check if it is possible to convolute left lower corner */
                else if(recv_stat.MPI_TAG == SW){
                    flag_corner_ll += 1;

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner
                } // End if SW
                /* Convolute left column, left upper corner and left lower corner */
                else if(recv_stat.MPI_TAG == W){
                    flag_corner_ll += 1;

                    /* Left column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                                my_image_before[i - 1][1] * my_args.filter[0][1] +
                                                my_image_before[i - 1][2] * my_args.filter[0][2] +
                                                my_image_before[i][2] * my_args.filter[1][2] +
                                                my_image_before[i + 1][2] * my_args.filter[2][2] +
                                                my_image_before[i + 1][1] * my_args.filter[2][1] +
                                                my_image_before[i + 1][0] * my_args.filter[2][0] +
                                                my_image_before[i][0] * my_args.filter[1][0] +
                                                my_image_before[i - 1][0] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][1] < 0)
                            my_image_after[i][1] = 0;
                        else if(my_image_after[i][1] > 255)
                            my_image_after[i][1] = 255;
                    } // End for

                    /* Left upper corner */
                    my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                            my_image_before[1][2] * my_args.filter[1][2] +
                                            my_image_before[2][2] * my_args.filter[2][2] +
                                            my_image_before[2][1] * my_args.filter[2][1] +
                                            my_image_before[2][0] * my_args.filter[2][0] +
                                            my_image_before[1][0] * my_args.filter[1][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[1][1] < 0)
                        my_image_after[1][1] = 0;
                    else if(my_image_after[1][1] > 255)
                        my_image_after[1][1] = 255;

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner
                } // End if W
            } // End for - Wait any

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_incr_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);
    } // End if e)
    /* Right column processes - except from first and last process in this column - active neighbours N, S, SW, W, NW */
    else if(my_rank % procs_per_line == procs_per_line_1){
        int flag_corner_ul = 0, flag_corner_ll = 0; // Flag == 3, convolute corners  

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){
            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* Right column - except from upper and lower right corners */
            for(i = 2; i < my_height; i++){
                my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[i][my_width] < 0)
                    my_image_after[i][my_width] = 0;
                else if(my_image_after[i][my_width] > 255)
                    my_image_after[i][my_width] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);

                /* Convolute first line, right upper corner and left upper corner */
                if(recv_stat.MPI_TAG == N){
                    flag_corner_ul += 1;

                    /* First line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                                my_image_before[0][j] * my_args.filter[0][1] +
                                                my_image_before[0][j + 1] * my_args.filter[0][2] +
                                                my_image_before[1][j + 1] * my_args.filter[1][2] +
                                                my_image_before[2][j + 1] * my_args.filter[2][2] +
                                                my_image_before[2][j] * my_args.filter[2][1] +
                                                my_image_before[2][j - 1] * my_args.filter[2][0] +
                                                my_image_before[1][j - 1] * my_args.filter[1][0] +
                                                my_image_before[0][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[1][j] < 0)
                            my_image_after[1][j] = 0;
                        else if(my_image_after[1][j] > 255)
                            my_image_after[1][j] = 255;
                    } // End for

                    /* Right upper corner */
                    my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                    my_image_before[0][my_width] * my_args.filter[0][1] +
                                                    my_image_before[2][my_width] * my_args.filter[2][1] +
                                                    my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                    my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                    my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
                    /* Truncated unexpected values */
                    if(my_image_after[1][my_width] < 0)
                        my_image_after[1][my_width] = 0;
                    else if(my_image_after[1][my_width] > 255)
                        my_image_after[1][my_width] = 255;
                
                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                        my_image_after[1][1] = 255;
                    } // End if corner 
                } // End if N
                /* Convolute last line, right lower corner and left lower corner */
                else if(recv_stat.MPI_TAG == S){
                    flag_corner_ll += 1;

                    /* Last line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][j + 1] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][j] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][j - 1] * my_args.filter[2][0] +
                                                        my_image_before[my_height][j - 1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[my_height][j] < 0)
                            my_image_after[my_height][j] = 0;
                        else if(my_image_after[my_height][j] > 255)
                            my_image_after[my_height][j] = 255;
                    } // End for

                    /* Right lower corner */
                    my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                            my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                            my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                            my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                            my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                            my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                    /* Truncated unexpected values */
                    if(my_image_after[my_height][my_width] < 0)
                        my_image_after[my_height][my_width] = 0;
                    else if(my_image_after[my_height][my_width] > 255)
                        my_image_after[my_height][my_width] = 255;

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner   
                } // End if S
                /* Check if it is possible to convolute left lower corner */
                else if(recv_stat.MPI_TAG == SW){
                    flag_corner_ll += 1;

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner
                } // End if SW
                /* Convolute left column, left lower corner and left upper corner */
                if(recv_stat.MPI_TAG == W){
                    flag_corner_ll += 1;
                    flag_corner_ul += 1;

                    /* Left column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                                my_image_before[i - 1][1] * my_args.filter[0][1] +
                                                my_image_before[i - 1][2] * my_args.filter[0][2] +
                                                my_image_before[i][2] * my_args.filter[1][2] +
                                                my_image_before[i + 1][2] * my_args.filter[2][2] +
                                                my_image_before[i + 1][1] * my_args.filter[2][1] +
                                                my_image_before[i + 1][0] * my_args.filter[2][0] +
                                                my_image_before[i][0] * my_args.filter[1][0] +
                                                my_image_before[i - 1][0] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][1] < 0)
                            my_image_after[i][1] = 0;
                        else if(my_image_after[i][1] > 255)
                            my_image_after[i][1] = 255;
                    } // End for

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner

                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner
                } // End if W 
                /* Check if it is possible to convolute left upper corner */
                else if(recv_stat.MPI_TAG == NW){
                    flag_corner_ul += 1;
                    
                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner 
                } // End if NW
            } // End for - Wait any
            
            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < my_height_incr_2; i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);

    } // End if f)
    /* Last line processes - except from first and last process in this line - active neighbours N, NE, E, W, NW */
    else if(my_rank > procs_per_line * procs_per_line_1){
        int flag_corner_ul = 0, flag_corner_ur = 0; // Flag == 3, convolute corners  

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* Last line - except from left and right lower corners */
            for(j = 2; j < my_width; j++){
                my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                my_image_before[my_height][j - 1] * my_args.filter[2][0] +
                                                my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[my_height][j] < 0)
                    my_image_after[my_height][j] = 0;
                else if(my_image_after[my_height][j] > 255)
                    my_image_after[my_height][j] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
           
                /* Convolute first line */
                if(recv_stat.MPI_TAG == N){
                    flag_corner_ul += 1;
                    flag_corner_ur += 1;

                    /* First line, left upper corner and right upper corner */
                    for(j = 2; j < my_width; j++){
                        my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                                my_image_before[0][j] * my_args.filter[0][1] +
                                                my_image_before[0][j + 1] * my_args.filter[0][2] +
                                                my_image_before[1][j + 1] * my_args.filter[1][2] +
                                                my_image_before[2][j + 1] * my_args.filter[2][2] +
                                                my_image_before[2][j] * my_args.filter[2][1] +
                                                my_image_before[2][j - 1] * my_args.filter[2][0] +
                                                my_image_before[1][j - 1] * my_args.filter[1][0] +
                                                my_image_before[0][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[1][j] < 0)
                            my_image_after[1][j] = 0;
                        else if(my_image_after[1][j] > 255)
                            my_image_after[1][j] = 255;
                    } // End for
                   
                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner
                } // End if N
                /* Check if it is possible to convolute right upper corner */
                else if(recv_stat.MPI_TAG == NE){
                    flag_corner_ur += 1;

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner
                } // End if NE
                /* Convolute right column, right lower corner and right upper corner */
                if(recv_stat.MPI_TAG == E){
                    flag_corner_ur += 1;

                    /* Right column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[i - 1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[i][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[i + 1][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                        my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][my_width] < 0)
                            my_image_after[i][my_width] = 0;
                        else if(my_image_after[i][my_width] > 255)
                            my_image_after[i][my_width] = 255;
                    } // End for

                    /* Right lower corner */
                    my_image_after[my_height][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                    
                    /* Truncated unexpected values */
                    if(my_image_after[my_height][my_width] < 0)
                        my_image_after[my_height][my_width] = 0;
                    else if(my_image_after[my_height][my_width] > 255)
                        my_image_after[my_height][my_width] = 255;
                    
                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner
                } // End if E
                /* Convolute left column, left lower corner and left upper corner */
                if(recv_stat.MPI_TAG == W){
                    flag_corner_ul += 1;
                    
                    /* Left column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                                my_image_before[i - 1][1] * my_args.filter[0][1] +
                                                my_image_before[i - 1][2] * my_args.filter[0][2] +
                                                my_image_before[i][2] * my_args.filter[1][2] +
                                                my_image_before[i + 1][2] * my_args.filter[2][2] +
                                                my_image_before[i + 1][1] * my_args.filter[2][1] +
                                                my_image_before[i + 1][0] * my_args.filter[2][0] +
                                                my_image_before[i][0] * my_args.filter[1][0] +
                                                my_image_before[i - 1][0] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][1] < 0)
                            my_image_after[i][1] = 0;
                        else if(my_image_after[i][1] > 255)
                            my_image_after[i][1] = 255;
                    } // End for

                    /* Left lower corner */
                    my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                    my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                    my_image_before[my_height][2] * my_args.filter[1][2] +
                                                    my_image_before[my_height][0] * my_args.filter[1][0] +
                                                    my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
                    
                    /* Truncated unexpected values */
                    if(my_image_after[my_height][1] < 0)
                        my_image_after[my_height][1] = 0;
                    else if(my_image_after[my_height][1] > 255)
                        my_image_after[my_height][1] = 255;
                    
                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner
                } // End if W 
                /* Check if it is possible to convolute left upper corner */
                if(recv_stat.MPI_TAG == NW){
                    flag_corner_ul += 1;

                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner
                } // End if NW
            } // End for - Wait any

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_incr_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);
    } // End if g)
    /* Left column processes - except from first and last process in this column - active neighbours N, NE, E, SE, S */
    else if(my_rank % procs_per_line == 0){
        int flag_corner_ur = 0, flag_corner_lr = 0; // Flag == 3, convolute corners  

        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            //////////////////////////////////////////////
            /* Convolute outer independent pixels first */
            //////////////////////////////////////////////

            /* Left column - except upper and lower left corners */
            for(i = 2; i < my_height; i++){
                my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                        my_image_before[i - 1][1] * my_args.filter[0][1] +
                                        my_image_before[i - 1][2] * my_args.filter[0][2] +
                                        my_image_before[i][2] * my_args.filter[1][2] +
                                        my_image_before[i + 1][2] * my_args.filter[2][2] +
                                        my_image_before[i + 1][1] * my_args.filter[2][1]);

                /* Truncated unexpected values */
                if(my_image_after[i][1] < 0)
                    my_image_after[i][1] = 0;
                else if(my_image_after[i][1] > 255)
                    my_image_after[i][1] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
            
                /* Convolute first line, left upper corner and right upper corner */
                if(recv_stat.MPI_TAG == N){
                    flag_corner_ur += 1;

                    /* First line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                                my_image_before[0][j] * my_args.filter[0][1] +
                                                my_image_before[0][j + 1] * my_args.filter[0][2] +
                                                my_image_before[1][j + 1] * my_args.filter[1][2] +
                                                my_image_before[2][j + 1] * my_args.filter[2][2] +
                                                my_image_before[2][j] * my_args.filter[2][1] +
                                                my_image_before[2][j - 1] * my_args.filter[2][0] +
                                                my_image_before[1][j - 1] * my_args.filter[1][0] +
                                                my_image_before[0][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[1][j] < 0)
                            my_image_after[1][j] = 0;
                        else if(my_image_after[1][j] > 255)
                            my_image_after[1][j] = 255;
                    } // End for
                    
                    /* Left upper corner */
                    my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                            my_image_before[0][1] * my_args.filter[0][1] +
                                            my_image_before[0][2] * my_args.filter[0][2] +
                                            my_image_before[1][2] * my_args.filter[1][2] +
                                            my_image_before[2][2] * my_args.filter[2][2] +
                                            my_image_before[2][1] * my_args.filter[2][1]);
                    
                    /* Truncated unexpected values */
                    if(my_image_after[1][1] < 0)
                        my_image_after[1][1] = 0;
                    else if(my_image_after[1][1] > 255)
                        my_image_after[1][1] = 255;

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner 
                } // End if N
                /* Check if it is possible to convolute right upper corner */
                else if(recv_stat.MPI_TAG == NE){
                    flag_corner_ur += 1;

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner
                } // End if NE
                /* Convolute right column, right upper corner and right lower corner */
                else if(recv_stat.MPI_TAG == E){
                    flag_corner_ur += 1;
                    flag_corner_lr += 1;

                    /* Right column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[i - 1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[i][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[i + 1][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                        my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][my_width] < 0)
                            my_image_after[i][my_width] = 0;
                        else if(my_image_after[i][my_width] > 255)
                            my_image_after[i][my_width] = 255;
                    } // End for

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner

                    /* Convolute right lower corner */
                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if E
                /* Check if it is possible to convolute right lower corner */
                else if(recv_stat.MPI_TAG == SE){
                    flag_corner_lr += 1;

                    /* Convolute right lower corner */
                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if SE
                /* Convolute last line, left lower corner and right lower corner */
                else if(recv_stat.MPI_TAG == S){
                    flag_corner_lr += 1;

                    /* Last line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][j + 1] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][j] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][j - 1] * my_args.filter[2][0] +
                                                        my_image_before[my_height][j - 1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[my_height][j] < 0)
                            my_image_after[my_height][j] = 0;
                        else if(my_image_after[my_height][j] > 255)
                            my_image_after[my_height][j] = 255;
                    } // End for

                    /* Left lower corner */
                    my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                    my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                    my_image_before[my_height][2] * my_args.filter[1][2] +
                                                    my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                    my_image_before[my_height_incr_1][1] * my_args.filter[2][1]);

                    /* Truncated unexpected values */
                    if(my_image_after[my_height][1] < 0)
                        my_image_after[my_height][1] = 0;
                    else if(my_image_after[my_height][1] > 255)
                        my_image_after[my_height][1] = 255;

                    /* Convolute right lower corner */
                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if S
            } // End for - Wait any
            
            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_incr_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);
    } // End if h)
    /* Inner processes - all neighbours are active */
    else{
        int flag_corner_ul = 0, flag_corner_ur = 0, flag_corner_ll = 0, flag_corner_lr = 0; // Flag == 3, convolute corners  
        
        /* Perform convolution */
        for(iter = 0; iter < my_args.iterations; iter++){

            /* Start sending my pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, send_requests);

            //////////////////////////////////
            /* Convolute inner pixels first */
            //////////////////////////////////

            for(i = 2; i < my_height; i++){ // For every inner row
                for(j = 2; j < my_width; j++){ // and every inner column

                    /* Compute the new value of the current pixel */
                    my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[1][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    else if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
            
                /* Convolute first line, left upper corner and right upper corner */
                if(recv_stat.MPI_TAG == N){
                    flag_corner_ul += 1;
                    flag_corner_ur += 1;

                    /* First line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                                my_image_before[0][j] * my_args.filter[0][1] +
                                                my_image_before[0][j + 1] * my_args.filter[0][2] +
                                                my_image_before[1][j + 1] * my_args.filter[1][2] +
                                                my_image_before[2][j + 1] * my_args.filter[2][2] +
                                                my_image_before[2][j] * my_args.filter[2][1] +
                                                my_image_before[2][j - 1] * my_args.filter[2][0] +
                                                my_image_before[1][j - 1] * my_args.filter[1][0] +
                                                my_image_before[0][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[1][j] < 0)
                            my_image_after[1][j] = 0;
                        else if(my_image_after[1][j] > 255)
                            my_image_after[1][j] = 255;
                    } // End for
                   
                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner
                } // End if N
                /* Check if it is possible to convolute right upper corner */
                else if(recv_stat.MPI_TAG == NE){
                    flag_corner_ur += 1;

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner
                } // End if NE
                /* Convolute right column, right upper corner and right lower corner */
                else if(recv_stat.MPI_TAG == E){
                    flag_corner_ur += 1;
                    flag_corner_lr += 1;

                    /* Right column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][my_width] = (int)(my_image_before[i][my_width] * my_args.filter[1][1] +
                                                        my_image_before[i - 1][my_width] * my_args.filter[0][1] +
                                                        my_image_before[i - 1][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[i][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[i + 1][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[i + 1][my_width] * my_args.filter[2][1] +
                                                        my_image_before[i + 1][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[i][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[i - 1][my_width_decr_1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][my_width] < 0)
                            my_image_after[i][my_width] = 0;
                        else if(my_image_after[i][my_width] > 255)
                            my_image_after[i][my_width] = 255;
                    } // End for

                    /* Convolute right upper corner */
                    if(flag_corner_ur == 3){
                        my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                                        my_image_before[0][my_width] * my_args.filter[0][1] +
                                                        my_image_before[0][my_width_incr_1] * my_args.filter[0][2] +
                                                        my_image_before[1][my_width_incr_1] * my_args.filter[1][2] +
                                                        my_image_before[2][my_width_incr_1] * my_args.filter[2][2] +
                                                        my_image_before[2][my_width] * my_args.filter[2][1] +
                                                        my_image_before[2][my_width_decr_1] * my_args.filter[2][0] +
                                                        my_image_before[1][my_width_decr_1] * my_args.filter[1][0] +
                                                        my_image_before[0][my_width_decr_1] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[1][my_width] < 0)
                            my_image_after[1][my_width] = 0;
                        else if(my_image_after[1][my_width] > 255)
                            my_image_after[1][my_width] = 255;
                    } // End if corner

                    /* Convolute right lower corner */
                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if E
                /* Check if it is possible to convolute right lower corner */
                else if(recv_stat.MPI_TAG == SE){
                    flag_corner_lr += 1;

                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if SE
                /* Convolute last line, left lower corner and right lower corner */
                else if(recv_stat.MPI_TAG == S){
                    flag_corner_ll += 1;
                    flag_corner_lr += 1;

                    /* Last line */
                    for(j = 2; j < my_width; j++){
                        my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][j + 1] * my_args.filter[0][2] +
                                                        my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][j + 1] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][j] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][j - 1] * my_args.filter[2][0] +
                                                        my_image_before[my_height][j - 1] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][j - 1] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[my_height][j] < 0)
                            my_image_after[my_height][j] = 0;
                        else if(my_image_after[my_height][j] > 255)
                            my_image_after[my_height][j] = 255;
                    } // End for

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner   
                    /* Convolute right lower corner */
                    if(flag_corner_lr == 3){
                        my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                                my_image_before[my_height_decr_1][my_width] * my_args.filter[0][1] +
                                                                my_image_before[my_height_decr_1][my_width_incr_1] * my_args.filter[0][2] +
                                                                my_image_before[my_height][my_width_incr_1] * my_args.filter[1][2] +
                                                                my_image_before[my_height_incr_1][my_width_incr_1] * my_args.filter[2][2] +
                                                                my_image_before[my_height_incr_1][my_width] * my_args.filter[2][1] +
                                                                my_image_before[my_height_incr_1][my_width_decr_1] * my_args.filter[2][0] +
                                                                my_image_before[my_height][my_width_decr_1] * my_args.filter[1][0] +
                                                                my_image_before[my_height_decr_1][my_width_decr_1] * my_args.filter[0][0]);
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][my_width] < 0)
                            my_image_after[my_height][my_width] = 0;
                        else if(my_image_after[my_height][my_width] > 255)
                            my_image_after[my_height][my_width] = 255;
                    } // End if corner 
                } // End if S
                /* Check if it is possible to convolute left lower corner */
                else if(recv_stat.MPI_TAG == SW){
                    flag_corner_ll += 1;

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner
                } // End if SW
                /* Convolute left column, left upper corner and left lower corner */
                if(recv_stat.MPI_TAG == W){
                    flag_corner_ul += 1;
                    flag_corner_ll += 1;
                    
                    /* Left column */
                    for(i = 2; i < my_height; i++){
                        my_image_after[i][1] = (int)(my_image_before[i][1] * my_args.filter[1][1] +
                                                my_image_before[i - 1][1] * my_args.filter[0][1] +
                                                my_image_before[i - 1][2] * my_args.filter[0][2] +
                                                my_image_before[i][2] * my_args.filter[1][2] +
                                                my_image_before[i + 1][2] * my_args.filter[2][2] +
                                                my_image_before[i + 1][1] * my_args.filter[2][1] +
                                                my_image_before[i + 1][0] * my_args.filter[2][0] +
                                                my_image_before[i][0] * my_args.filter[1][0] +
                                                my_image_before[i - 1][0] * my_args.filter[0][0]);

                        /* Truncated unexpected values */
                        if(my_image_after[i][1] < 0)
                            my_image_after[i][1] = 0;
                        else if(my_image_after[i][1] > 255)
                            my_image_after[i][1] = 255;
                    } // End for

                    /* Convolute left lower corner */
                    if(flag_corner_ll == 3){
                        my_image_after[my_height][1] = (int)(my_image_before[my_height][1] * my_args.filter[1][1] +
                                                        my_image_before[my_height_decr_1][1] * my_args.filter[0][1] +
                                                        my_image_before[my_height_decr_1][2] * my_args.filter[0][2] +
                                                        my_image_before[my_height][2] * my_args.filter[1][2] +
                                                        my_image_before[my_height_incr_1][2] * my_args.filter[2][2] +
                                                        my_image_before[my_height_incr_1][1] * my_args.filter[2][1] +
                                                        my_image_before[my_height_incr_1][0] * my_args.filter[2][0] +
                                                        my_image_before[my_height][0] * my_args.filter[1][0] +
                                                        my_image_before[my_height_decr_1][0] * my_args.filter[0][0]);
            
                        /* Truncated unexpected values */
                        if(my_image_after[my_height][1] < 0)
                            my_image_after[my_height][1] = 0;
                        else if(my_image_after[my_height][1] > 255)
                            my_image_after[my_height][1] = 255;
                    } // End if corner

                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner
                } // End if W 
                /* Check if it is possible to convolute left upper corner */
                if(recv_stat.MPI_TAG == NW){
                    flag_corner_ul += 1;

                    /* Convolute left upper corner */
                    if(flag_corner_ul == 3){
                        my_image_after[1][1] = (int)(my_image_before[1][1] * my_args.filter[1][1] +
                                                my_image_before[0][1] * my_args.filter[0][1] +
                                                my_image_before[0][2] * my_args.filter[0][2] +
                                                my_image_before[1][2] * my_args.filter[1][2] +
                                                my_image_before[2][2] * my_args.filter[2][2] +
                                                my_image_before[2][1] * my_args.filter[2][1] +
                                                my_image_before[2][0] * my_args.filter[2][0] +
                                                my_image_before[1][0] * my_args.filter[1][0] +
                                                my_image_before[0][0] * my_args.filter[0][0]);
                    
                        /* Truncated unexpected values */
                        if(my_image_after[1][1] < 0)
                            my_image_after[1][1] = 0;
                        else if(my_image_after[1][1] > 255)
                            my_image_after[1][1] = 255;
                    } // End if corner
                } // End if NW 
            } // End for
            
            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_incr_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_incr_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
        } // End of iter
        
        char fileName[10]="";
        sprintf(fileName,"File%dB",my_rank);

        FILE* my_file = fopen(fileName, "w");
        for(i = 0; i < my_height_incr_2; i++){
            for(j = 0; j < my_width_incr_2; j++){
                fprintf(my_file, "%d\t", my_image_after[i][j]);   
            }
            fprintf(my_file, "\n");   
        }
        
        fclose(my_file);
    } // End else i)

    /* Free memory */
    free(my_image_before[0]);
    free(my_image_before);

    free(my_image_after[0]);
    free(my_image_after);

    /* Free persistent requests objects */
    for(i = 0; i < NUM_NEIGHBOURS; i++){
        MPI_Request_free(&send_requests[i]);
        MPI_Request_free(&recv_requests[i]);
    } // End for

    /* Deallocate data types */
    MPI_Type_free(&filter_type);
    MPI_Type_free(&filter_type1);
    MPI_Type_free(&args_type);
    MPI_Type_free(&column_type);

    /* Terminate MPI execution */
    MPI_Finalize();

    return 0;
}

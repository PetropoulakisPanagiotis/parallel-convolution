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
#include <omp.h>

#include "utils.h"

/* Enable behaviors */ 
//#define CHECK_CONVERGENCE
//#define ENABLE_OPEN_MP

int main(void){
    MPI_Datatype args_type, filter_type, filter_type1; // Define new mpi derived types
    MPI_Status recv_stat; // For communication
    Args_type my_args; // Arguments of current process
    int comm_size, my_rank, error;
    int i, j, k, iter, index;
    
    #ifdef CHECK_CONVERGENCE
    int print_message = 0, all_finished, equality_flag = 0; // print_message & all_finished: convergence check
    #endif

    /* Initialize MPI environment - Get number of processes and rank. */
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int procs_per_line = (int)sqrt(comm_size); // Number of processes in each row/column

    /* Check if number of processes is a perfect square number */
    if(my_rank == 0){
        if(procs_per_line * procs_per_line != comm_size){
            printf("Invalid number of processes given. Must be a perfect square: 4, 9, 16,...\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        if(comm_size <= 0 || comm_size > PROCESSES_LIMIT){
            printf("Invalid number of processes given. Must be a positive heigher than 0 and less than %d\n",PROCESSES_LIMIT);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    /* Create a cartesian topology for better performance */
    MPI_Comm old_comm, my_cartesian_comm;
    int ndims, reorder, periods[2], dim_size[2];

    old_comm = MPI_COMM_WORLD;
    ndims = 2;
    dim_size[0] = procs_per_line;
    dim_size[1] = procs_per_line;
    periods[0] = 0;
    periods[1] = 0;
    reorder = 1;

    MPI_Cart_create(old_comm,ndims,dim_size,periods,reorder,&my_cartesian_comm);


    /* Define contiguous derived type - used for filter */
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
            MPI_Abort(my_cartesian_comm, error);

        /* Calculate the limits for every images */
        my_args.width_per_process = my_args.image_width / (int)sqrt(comm_size);
        my_args.width_remaining = my_args.image_width % (int)sqrt(comm_size);
        my_args.height_per_process = my_args.image_height / (int)sqrt(comm_size);
        my_args.height_remaining = my_args.image_height % (int)sqrt(comm_size);

        /* Send arguments in other proccesses */
        for(i = 1; i < comm_size; i++)
            MPI_Send(&my_args, 1, args_type, i, 1, my_cartesian_comm);
    }
    else{

        /* Get arguments from process 0*/
        MPI_Recv(&my_args, 1, args_type, 0, 1, my_cartesian_comm, &recv_stat);
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

    /* Find neighbours */
    MPI_Cart_shift(my_cartesian_comm, 0, 1, &neighbours[N], &neighbours[S]);
    MPI_Cart_shift(my_cartesian_comm, 1, 1, &neighbours[W], &neighbours[E]);


    /* Find corner neighbours */

    /* [NE]North-East Neighbour (1) */
    if(column_id != 0 && row_id != procs_per_line - 1) // If not on right up corner
        neighbours[NE] = my_rank - procs_per_line + 1;
    else
        neighbours[NE] = MPI_PROC_NULL;

    /* [SE]South-East Neighbour (3) */
    if(column_id != procs_per_line - 1 && row_id != procs_per_line - 1) // If not on the right down corner
        neighbours[SE] = my_rank + procs_per_line + 1;
    else
        neighbours[SE] = MPI_PROC_NULL;

    /* [SW]South-West Neighbour (5) */
    if(column_id != procs_per_line -1 && row_id != 0) // If not on left down corner
        neighbours[SW] = my_rank + procs_per_line - 1;
    else
        neighbours[SW] = MPI_PROC_NULL;

    /* [NW]North-West Neighbour (7) */
    if(row_id != 0 && column_id != 0) // If not on left up corner
        neighbours[NW] = my_rank - procs_per_line - 1;
    else
        neighbours[NW] = MPI_PROC_NULL;

    // print all neighbours //
    //printf("[%d] %d %d %d %d %d %d %d %d\n", my_rank, neighbours[N], neighbours[NE], neighbours[E], neighbours[SE], neighbours[S], neighbours[SW], neighbours[W], neighbours[NW]);

    int mult; // Will be used as multiplier to locate pixels, grey: 1, rgb: 3

    /* The resolution of the image that each process has - Add some frequent vars */
    int my_width, my_width_incr_1, my_width_decr_1, my_width_incr_2;
    int my_height, my_height_incr_1, my_height_decr_1, my_height_incr_2;
    int mult_multi_2;

    mult = (my_args.image_type == 0) ? 1 : 3; // if grey type, mult = 1, else mult = 3
    mult_multi_2 = mult * 2;

    /* If width or height is not perfectly divided into processes, share */
    /* the n remaining pixels to the first n processes                   */
    if(row_id < my_args.width_remaining)
        my_width = (my_args.width_per_process + 1) * mult; // In case of rgb, we multiply * 3 for each pixel
    else
        my_width = my_args.width_per_process * mult;

    if(column_id < my_args.height_remaining)
        my_height = my_args.height_per_process + 1;
    else
        my_height = my_args.height_per_process;


    /* Fix frequent sums into new variables(height and width of image including hallow points etc) */
    my_width_incr_1 = my_width + mult;
    my_height_incr_1 = my_height + 1;

    my_width_incr_2 = my_width_incr_1 + mult;
    my_height_incr_2 = my_height_incr_1 + 1;

    my_width_decr_1 = my_width - mult;
    my_height_decr_1 = my_height - 1;

    // print details
    //printf("[%d]width: %d, height: %d, mult: %d, width+1: %d, width+2: %d, height+1: %d, height+2: %d, width-1: %d, heigh-1: %d\n",my_rank, my_width, my_height, mult,my_width_incr_1,my_width_incr_2,my_height_incr_1,my_height_incr_2,my_width_decr_1,my_height_decr_1);

    /* For random images, set the seed differently to each process, in order */
    /* to have a fully random image and not repetitive cells                 */
    srand(my_args.image_seed * ((my_rank + 333) * (my_rank + 333)));

    /* Create array that will hold all pixels and generate a random image           */
    /* Add two rows and two collumns as "hallow points" -> Keep neighbours pixels   */
    /* Note: Allocate image with a way that array has a constant offset in collumns */
    int** my_image_before, **my_image_after, *tmp_ptr; // Tmp for swapping

    /* Allocate pointers for height */
    my_image_before = malloc((my_height_incr_2) * sizeof(int*));
    if(my_image_before == NULL)
        MPI_Abort(my_cartesian_comm, error);

    /* Allocate a contiguous array */
    my_image_before[0] = malloc((my_height_incr_2) * (my_width_incr_2) * sizeof(int));
    if(my_image_before[0] == NULL)
        MPI_Abort(my_cartesian_comm, error);

    /* Fix array(rows) */
    for(i = 1; i < (my_height_incr_2); i++)
        my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

    /* Fill initial image with random numbers */
    for(i = 1; i <  my_height_incr_1; i++)
        for(j = mult; j < my_width_incr_1; j++)
            my_image_before[i][j] = rand() % 256;

    /* Set edges(hallow points, until neighbours send theirs) */
    for(i = 0; i < my_height_incr_2; i++){
        for(j = 0; j < mult; j++){
            my_image_before[i][j] = my_image_before[i][mult + j];
            my_image_before[i][my_width_incr_1 + j] = my_image_before[i][my_width + j];
        }
    }

    for(j = 0; j < my_width_incr_2; j++){
        my_image_before[0][j] = my_image_before[1][j];
        my_image_before[my_height_incr_1][j] = my_image_before[my_height][j];
    }

    /* Allocate an image to save the result */
    my_image_after = malloc((my_height_incr_2) * sizeof(int*));
    if(my_image_after == NULL)
        MPI_Abort(my_cartesian_comm, error);

    /* Allocate a contiguous array */
    my_image_after[0] = malloc((my_height_incr_2) * (my_width_incr_2) * sizeof(int));
    if(my_image_after[0] == NULL)
        MPI_Abort(my_cartesian_comm, error);

    /* Fix array */
    for(i = 1; i < (my_height_incr_2); i++)
        my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);

    for(i = 0; i <  my_height_incr_2; i++)
        for(j = 0; j < my_width_incr_2; j++)
            my_image_after[i][j] = my_image_before[i][j];

    /* Set edges(hallow points, until neighbours send theirs) */
    for(i = 0; i < my_height_incr_2; i++){
        for(j = 0; j < mult; j++){
            my_image_after[i][j] = my_image_before[i][mult + j];
            my_image_after[i][my_width_incr_1 + j] = my_image_before[i][my_width + j];
        }
    }

    for(j = 0; j < my_width_incr_2; j++){
        my_image_after[0][j] = my_image_before[1][j];
        my_image_after[my_height_incr_1][j] = my_image_before[my_height][j];
    }

    printImageBefore(my_image_before,my_height_incr_2,my_width_incr_2,my_rank);

    /* Set columns type for sending columns East and West */
    MPI_Datatype column_type;
    MPI_Type_vector(my_height, mult, my_width_incr_2, MPI_INT, &column_type);
    MPI_Type_commit(&column_type);

    /* Initialize communication with neighbours */
    MPI_Request send_after_requests[NUM_NEIGHBOURS];
    MPI_Request send_before_requests[NUM_NEIGHBOURS];
    MPI_Request recv_after_requests[NUM_NEIGHBOURS];
    MPI_Request recv_before_requests[NUM_NEIGHBOURS];

    /* Send to each neighbour, tagging it with the opposite direction of the receiving process(eg N->S, SW -> NE) */
    MPI_Send_init(&my_image_after[1][mult], my_width, MPI_INT, neighbours[N], S, my_cartesian_comm, &send_after_requests[N]);
    MPI_Send_init(&my_image_after[1][my_width], mult, MPI_INT, neighbours[NE], SW, my_cartesian_comm, &send_after_requests[NE]);
    MPI_Send_init(&my_image_after[1][my_width], 1, column_type, neighbours[E], W, my_cartesian_comm, &send_after_requests[E]);
    MPI_Send_init(&my_image_after[my_height][my_width], mult, MPI_INT, neighbours[SE], NW, my_cartesian_comm, &send_after_requests[SE]);
    MPI_Send_init(&my_image_after[my_height][mult], my_width, MPI_INT, neighbours[S], N, my_cartesian_comm, &send_after_requests[S]);
    MPI_Send_init(&my_image_after[my_height][mult], mult, MPI_INT, neighbours[SW], NE, my_cartesian_comm, &send_after_requests[SW]);
    MPI_Send_init(&my_image_after[1][mult], 1, column_type, neighbours[W], E, my_cartesian_comm, &send_after_requests[W]);
    MPI_Send_init(&my_image_after[1][mult], mult, MPI_INT, neighbours[NW], SE, my_cartesian_comm, &send_after_requests[NW]);
    
    /* Send to each neighbour, tagging it with the opposite direction of the receiving process(eg N->S, SW -> NE) */
    MPI_Send_init(&my_image_before[1][mult], my_width, MPI_INT, neighbours[N], S, my_cartesian_comm, &send_before_requests[N]);
    MPI_Send_init(&my_image_before[1][my_width], mult, MPI_INT, neighbours[NE], SW, my_cartesian_comm, &send_before_requests[NE]);
    MPI_Send_init(&my_image_before[1][my_width], 1, column_type, neighbours[E], W, my_cartesian_comm, &send_before_requests[E]);
    MPI_Send_init(&my_image_before[my_height][my_width], mult, MPI_INT, neighbours[SE], NW, my_cartesian_comm, &send_before_requests[SE]);
    MPI_Send_init(&my_image_before[my_height][mult], my_width, MPI_INT, neighbours[S], N, my_cartesian_comm, &send_before_requests[S]);
    MPI_Send_init(&my_image_before[my_height][mult], mult, MPI_INT, neighbours[SW], NE, my_cartesian_comm, &send_before_requests[SW]);
    MPI_Send_init(&my_image_before[1][mult], 1, column_type, neighbours[W], E, my_cartesian_comm, &send_before_requests[W]);
    MPI_Send_init(&my_image_before[1][mult], mult, MPI_INT, neighbours[NW], SE, my_cartesian_comm, &send_before_requests[NW]);
    
    /* Receive from all neighbours */
    MPI_Recv_init(&my_image_after[0][mult], my_width, MPI_INT, neighbours[N], N, my_cartesian_comm, &recv_after_requests[N]);
    MPI_Recv_init(&my_image_after[0][my_width_incr_1], mult, MPI_INT, neighbours[NE], NE, my_cartesian_comm, &recv_after_requests[NE]);
    MPI_Recv_init(&my_image_after[1][my_width_incr_1], 1, column_type, neighbours[E], E, my_cartesian_comm, &recv_after_requests[E]);
    MPI_Recv_init(&my_image_after[my_height_incr_1][my_width_incr_1], mult, MPI_INT, neighbours[SE], SE, my_cartesian_comm, &recv_after_requests[SE]);
    MPI_Recv_init(&my_image_after[my_height_incr_1][mult], my_width, MPI_INT, neighbours[S], S, my_cartesian_comm, &recv_after_requests[S]);
    MPI_Recv_init(&my_image_after[my_height_incr_1][0], mult, MPI_INT, neighbours[SW],SW, my_cartesian_comm, &recv_after_requests[SW]);
    MPI_Recv_init(&my_image_after[1][0], 1, column_type, neighbours[W], W, my_cartesian_comm, &recv_after_requests[W]);
    MPI_Recv_init(&my_image_after[0][0], mult, MPI_INT, neighbours[NW], NW, my_cartesian_comm, &recv_after_requests[NW]);

    /* Receive from all neighbours */
    MPI_Recv_init(&my_image_before[0][mult], my_width, MPI_INT, neighbours[N], N, my_cartesian_comm, &recv_before_requests[N]);
    MPI_Recv_init(&my_image_before[0][my_width_incr_1], mult, MPI_INT, neighbours[NE], NE, my_cartesian_comm, &recv_before_requests[NE]);
    MPI_Recv_init(&my_image_before[1][my_width_incr_1], 1, column_type, neighbours[E], E, my_cartesian_comm, &recv_before_requests[E]);
    MPI_Recv_init(&my_image_before[my_height_incr_1][my_width_incr_1], mult, MPI_INT, neighbours[SE], SE, my_cartesian_comm, &recv_before_requests[SE]);
    MPI_Recv_init(&my_image_before[my_height_incr_1][mult], my_width, MPI_INT, neighbours[S], S, my_cartesian_comm, &recv_before_requests[S]);
    MPI_Recv_init(&my_image_before[my_height_incr_1][0], mult, MPI_INT, neighbours[SW],SW, my_cartesian_comm, &recv_before_requests[SW]);
    MPI_Recv_init(&my_image_before[1][0], 1, column_type, neighbours[W], W, my_cartesian_comm, &recv_before_requests[W]);
    MPI_Recv_init(&my_image_before[0][0], mult, MPI_INT, neighbours[NW], NW, my_cartesian_comm, &recv_before_requests[NW]);

    /* When Flag == 3, convolute corner */
    int flag_corner_ul = 0, flag_corner_ur = 0, flag_corner_ll = 0, flag_corner_lr = 0;

    MPI_Barrier(my_cartesian_comm);
    double start = MPI_Wtime(); // Get start time before iterations

    /* Perform convolution */
    for(iter = 0; iter < my_args.iterations; iter++){

        /* Note for flags    */
        /* ur -> upper right */
        /* ul -> upper left  */
        /* ll -> lower left  */
        /* lr -> lower right */

        /* Reset flags */
        flag_corner_ul = 0;
        flag_corner_ur = 0; 
        flag_corner_ll = 0;
        flag_corner_lr = 0;

        /* Start sending my outer pixels */
        if(iter % 2 == 0)
            MPI_Startall(NUM_NEIGHBOURS, send_before_requests);
        else
            MPI_Startall(NUM_NEIGHBOURS, send_after_requests);
        
        //////////////////////////////////
        /* Convolute inner pixels first */
        //////////////////////////////////
            
        #ifdef ENABLE_OPEN_MP
        #pragma omp parallel for num_threads(NUM_THREADS) collapse(2) schedule(static, (my_width - 2) * (my_height - 2) / NUM_THREADS)
        #endif
        for(i = 2; i < my_height; i++){ // For every inner row
            for(j = 2 * mult; j < my_width; j++){ // and every inner column

                /* Compute the new value of the current pixel */
                my_image_after[i][j] = (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                        my_image_before[i - 1][j] * my_args.filter[0][1] +
                                        my_image_before[i - 1][j + mult] * my_args.filter[0][2] +
                                        my_image_before[i][j + mult] * my_args.filter[1][2] +
                                        my_image_before[i + 1][j + mult] * my_args.filter[2][2] +
                                        my_image_before[i + 1][j] * my_args.filter[2][1] +
                                        my_image_before[i + 1][j - mult] * my_args.filter[2][0] +
                                        my_image_before[i][j - mult] * my_args.filter[1][0] +
                                        my_image_before[i - 1][j - mult] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[i][j] < 0)
                    my_image_after[i][j] = 0;
                else if(my_image_after[i][j] > 255)
                    my_image_after[i][j] = 255;
            } // End for
        } // End for


        /* Start receiving neighbours pixels/non-blocking */
        if(iter % 2 == 0)
            MPI_Startall(NUM_NEIGHBOURS, recv_before_requests);
        else
            MPI_Startall(NUM_NEIGHBOURS, recv_after_requests);
        
        //////////////////////////////////
        /* Convolute outer pixels first */
        //////////////////////////////////

        /* Keep receiving from all neighbours */
        for(k = 0; k < NUM_NEIGHBOURS; k++){

            if(iter % 2 == 0)
                MPI_Waitany(NUM_NEIGHBOURS, recv_before_requests, &index, &recv_stat);
            else
                MPI_Waitany(NUM_NEIGHBOURS, recv_after_requests, &index, &recv_stat);
            
            /* Convolute first line, left upper corner and right upper corner */
            if(index == N){
                flag_corner_ul++;
                flag_corner_ur++;

                /* First line */
                for(j = 2 * mult; j < my_width; j++){
                    my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                            my_image_before[0][j] * my_args.filter[0][1] +
                                            my_image_before[0][j + mult] * my_args.filter[0][2] +
                                            my_image_before[1][j + mult] * my_args.filter[1][2] +
                                            my_image_before[2][j + mult] * my_args.filter[2][2] +
                                            my_image_before[2][j] * my_args.filter[2][1] +
                                            my_image_before[2][j - mult] * my_args.filter[2][0] +
                                            my_image_before[1][j - mult] * my_args.filter[1][0] +
                                            my_image_before[0][j - mult] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[1][j] < 0)
                        my_image_after[1][j] = 0;
                    else if(my_image_after[1][j] > 255)
                        my_image_after[1][j] = 255;
                } // End for

            } // End if N
            /* Check if it is possible to convolute right upper corner */
            else if(index == NE){
                flag_corner_ur++;

            } // End if NE
            /* Convolute right column, right upper corner and right lower corner */
            else if(index == E){
                flag_corner_ur++;
                flag_corner_lr++;

                /* Right column */
                for(i = 2; i < my_height; i++){
                    for(j = 0; j < mult; j++){  // in case of rgb
                        my_image_after[i][my_width + j] = (int)(my_image_before[i][my_width + j] * my_args.filter[1][1] +
                                                    my_image_before[i - 1][my_width + j] * my_args.filter[0][1] +
                                                    my_image_before[i - 1][my_width_incr_1 + j] * my_args.filter[0][2] +
                                                    my_image_before[i][my_width_incr_1 + j] * my_args.filter[1][2] +
                                                    my_image_before[i + 1][my_width_incr_1 + j] * my_args.filter[2][2] +
                                                    my_image_before[i + 1][my_width + j] * my_args.filter[2][1] +
                                                    my_image_before[i + 1][my_width_decr_1 + j] * my_args.filter[2][0] +
                                                    my_image_before[i][my_width_decr_1 + j] * my_args.filter[1][0] +
                                                    my_image_before[i - 1][my_width_decr_1 + j] * my_args.filter[0][0]);

                        /* Truncate unexpected values */
                        if(my_image_after[i][my_width + j] < 0)
                        my_image_after[i][my_width + j] = 0;
                        else if(my_image_after[i][my_width + j] > 255)
                        my_image_after[i][my_width + j] = 255;
                    } // End for
                } // End for

            } // End if E
            /* Check if it is possible to convolute right lower corner */
            else if(index == SE){
                flag_corner_lr++;
            } // End if SE
            /* Convolute last line, left lower corner and right lower corner */
            else if(index == S){
                flag_corner_ll++;
                flag_corner_lr++;

                /* Last line */
                for(j = 2 * mult; j < my_width; j++){
                    my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][j] * my_args.filter[0][1] +
                                                    my_image_before[my_height_decr_1][j + mult] * my_args.filter[0][2] +
                                                    my_image_before[my_height][j + mult] * my_args.filter[1][2] +
                                                    my_image_before[my_height_incr_1][j + mult] * my_args.filter[2][2] +
                                                    my_image_before[my_height_incr_1][j] * my_args.filter[2][1] +
                                                    my_image_before[my_height_incr_1][j - mult] * my_args.filter[2][0] +
                                                    my_image_before[my_height][j - mult] * my_args.filter[1][0] +
                                                    my_image_before[my_height_decr_1][j - mult] * my_args.filter[0][0]);

                    /* Truncate unexpected values */
                    if(my_image_after[my_height][j] < 0)
                        my_image_after[my_height][j] = 0;
                    else if(my_image_after[my_height][j] > 255)
                        my_image_after[my_height][j] = 255;
                } // End for

            } // End if S
            /* Check if it is possible to convolute left lower corner */
            else if (index == SW){
                flag_corner_ll++;
            } // End if SW
            /* Convolute left column, left upper corner and left lower corner */
            if(index == W){
                flag_corner_ul++;
                flag_corner_ll++;

                /* Left column */
                for(i = 2; i < my_height; i++){
                    for(j = 0; j < mult; j++){
                        my_image_after[i][mult + j] = (int)(my_image_before[i][mult + j] * my_args.filter[1][1] +
                                                my_image_before[i - 1][mult + j] * my_args.filter[0][1] +
                                                my_image_before[i - 1][mult_multi_2 + j] * my_args.filter[0][2] +
                                                my_image_before[i][mult_multi_2 + j] * my_args.filter[1][2] +
                                                my_image_before[i + 1][mult_multi_2 + j] * my_args.filter[2][2] +
                                                my_image_before[i + 1][mult + j] * my_args.filter[2][1] +
                                                my_image_before[i + 1][j] * my_args.filter[2][0] +
                                                my_image_before[i][j] * my_args.filter[1][0] +
                                                my_image_before[i - 1][j] * my_args.filter[0][0]);


                        /* Truncate unexpected values */
                        if(my_image_after[i][mult + j] < 0)
                        my_image_after[i][mult + j] = 0;
                        else if(my_image_after[i][mult + j] > 255)
                        my_image_after[i][mult + j] = 255;
                    }
                } // End for

            } // End if W
            /* Check if it is possible to convolute left upper corner */
            if(index == NW){
                flag_corner_ul++;
            } // End if NW
            /* Convolute left upper corner */
            if(flag_corner_ul == 3){
                for(j = 0; j < mult; j++){
                    my_image_after[1][mult + j] = (int)(my_image_before[1][mult + j] * my_args.filter[1][1] +
                                            my_image_before[0][mult + j] * my_args.filter[0][1] +
                                            my_image_before[0][mult_multi_2 + j] * my_args.filter[0][2] +
                                            my_image_before[1][mult_multi_2 + j] * my_args.filter[1][2] +
                                            my_image_before[2][mult_multi_2 + j] * my_args.filter[2][2] +
                                            my_image_before[2][mult + j] * my_args.filter[2][1] +
                                            my_image_before[2][j] * my_args.filter[2][0] +
                                            my_image_before[1][j] * my_args.filter[1][0] +
                                            my_image_before[0][j] * my_args.filter[0][0]);

                    /* Truncate unexpected values */
                    if(my_image_after[1][mult + j] < 0)
                    my_image_after[1][mult + j] = 0;
                    else if(my_image_after[1][mult + j] > 255)
                    my_image_after[1][mult + j] = 255;
                } // End for

            } // End if corner
            /* Convolute right upper corner */
            if(flag_corner_ur == 3){
                for(j = 0; j < mult; j++){
                    my_image_after[1][my_width + j] = (int)(my_image_before[1][my_width + j] * my_args.filter[1][1] +
                                                    my_image_before[0][my_width + j] * my_args.filter[0][1] +
                                                    my_image_before[0][my_width_incr_1 + j] * my_args.filter[0][2] +
                                                    my_image_before[1][my_width_incr_1 + j] * my_args.filter[1][2] +
                                                    my_image_before[2][my_width_incr_1 + j] * my_args.filter[2][2] +
                                                    my_image_before[2][my_width + j] * my_args.filter[2][1] +
                                                    my_image_before[2][my_width_decr_1 + j] * my_args.filter[2][0] +
                                                    my_image_before[1][my_width_decr_1 + j] * my_args.filter[1][0] +
                                                    my_image_before[0][my_width_decr_1 + j] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[1][my_width + j] < 0)
                        my_image_after[1][my_width + j] = 0;
                    else if(my_image_after[1][my_width + j] > 255)
                        my_image_after[1][my_width + j] = 255;
                }
            } // End if corner

            /* Convolute right lower corner */
            if(flag_corner_lr == 3){
                for(j = 0; j < mult; j++){
                    my_image_after[my_height][my_width + j] = (int)(my_image_before[my_height][my_width + j] * my_args.filter[1][1] +
                                                            my_image_before[my_height_decr_1][my_width + j] * my_args.filter[0][1] +
                                                            my_image_before[my_height_decr_1][my_width_incr_1 + j] * my_args.filter[0][2] +
                                                            my_image_before[my_height][my_width_incr_1 + j] * my_args.filter[1][2] +
                                                            my_image_before[my_height_incr_1][my_width_incr_1 + j] * my_args.filter[2][2] +
                                                            my_image_before[my_height_incr_1][my_width + j] * my_args.filter[2][1] +
                                                            my_image_before[my_height_incr_1][my_width_decr_1 + j] * my_args.filter[2][0] +
                                                            my_image_before[my_height][my_width_decr_1 + j] * my_args.filter[1][0] +
                                                            my_image_before[my_height_decr_1][my_width_decr_1 + j] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[my_height][my_width + j] < 0)
                    my_image_after[my_height][my_width + j] = 0;
                else if(my_image_after[my_height][my_width + j] > 255)
                    my_image_after[my_height][my_width + j] = 255;
                }
            } // End if corner

            /* Convolute left lower corner */
            if(flag_corner_ll == 3){
                for(j = 0; j < mult; j++){
                    my_image_after[my_height][mult + j] = (int)(my_image_before[my_height][mult + j] * my_args.filter[1][1] +
                                                    my_image_before[my_height_decr_1][mult + j] * my_args.filter[0][1] +
                                                    my_image_before[my_height_decr_1][mult_multi_2 + j] * my_args.filter[0][2] +
                                                    my_image_before[my_height][mult_multi_2 + j] * my_args.filter[1][2] +
                                                    my_image_before[my_height_incr_1][mult_multi_2 + j] * my_args.filter[2][2] +
                                                    my_image_before[my_height_incr_1][mult + j] * my_args.filter[2][1] +
                                                    my_image_before[my_height_incr_1][j] * my_args.filter[2][0] +
                                                    my_image_before[my_height][j] * my_args.filter[1][0] +
                                                    my_image_before[my_height_decr_1][j] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[my_height][mult + j] < 0)
                    my_image_after[my_height][mult + j] = 0;
                    else if(my_image_after[my_height][mult + j] > 255)
                    my_image_after[my_height][mult + j] = 255;
                }
            } // End if corner
        } // End for - Wait any

        /* Wait all pixels to be send before procceeding to the next loop */
        if(iter % 2 == 0)
            MPI_Waitall(NUM_NEIGHBOURS, send_before_requests, MPI_STATUS_IGNORE);
        else
            MPI_Waitall(NUM_NEIGHBOURS, send_after_requests, MPI_STATUS_IGNORE);

        ///////////////////////////////////
        /* Convergence check with Reduce */
        ///////////////////////////////////

        #ifdef CHECK_CONVERGENCE 
        equality_flag = 0;

        // Check current image first
        for(i = 1; (i < my_height_incr_1) && (equality_flag == 0); i++){
            for(j = mult; j < my_width_incr_1; j++){
                if(my_image_before[i][j] != my_image_after[i][j]){
                    equality_flag = 1;
                    break;
                } // End if
            } // End for
        } // End for

        // Check if all processes reach convergence
        MPI_Allreduce(&equality_flag, &all_finished, 1, MPI_INT, MPI_LOR, my_cartesian_comm);

        if(my_rank == 0 && print_message == 0 && all_finished == 0){
            printf("Image convergence at %d iteration\n",iter);
            print_message = 1;
        }
        #endif

        //////////////////////////////
        /* End of Convergence check */
        //////////////////////////////

        /* In the next loop perform convolution to the new image  - swap images */
        tmp_ptr = my_image_before[0];

        my_image_before[0] = my_image_after[0];
        for(i = 1; i < my_height_incr_2; i++)
            my_image_before[i] = &(my_image_before[0][i*(my_width_incr_2)]);

        my_image_after[0] = tmp_ptr;
        for(i = 1; i < my_height_incr_2; i++)
            my_image_after[i] = &(my_image_after[0][i*(my_width_incr_2)]);
    } // End of iter

    /* Get time to calculate run time */
    double end = MPI_Wtime();
    double time_elapsed = end - start;
    double max_time, min_time;

    printImageAfter(my_image_before,my_height_incr_2,my_width_incr_2,my_rank);

    /* Print max run-time in parallel section */
    if(comm_size != 1){
        MPI_Reduce(&time_elapsed, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, my_cartesian_comm);
        MPI_Reduce(&time_elapsed, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, my_cartesian_comm);

    }
    else{
        max_time = time_elapsed;
        min_time = time_elapsed;
    }

    /* Print results */
    if(my_rank == 0)
        printf("\n[Parallel Convolution Completed]:\nType of Image: %d\nResolution: %d x %d\nSeed Given: %d\nNumber of Iterations: %d\nNumber of Processes: %d\nRun time: %.5lf seconds\nFastest process completed in: %.5lf seconds\n\n",
        my_args.image_type, my_args.image_width, my_args.image_height, my_args.image_seed, my_args.iterations, comm_size, max_time,min_time);

    /* Free memory */
    free(my_image_before[0]);
    free(my_image_before);

    free(my_image_after[0]);
    free(my_image_after);

    /* Free persistent requests objects */
    for(i = 0; i < NUM_NEIGHBOURS; i++){
        MPI_Request_free(&send_after_requests[i]);
        MPI_Request_free(&send_before_requests[i]);
        MPI_Request_free(&recv_after_requests[i]);
        MPI_Request_free(&recv_before_requests[i]);
    } // End for

    /* Deallocate data types */
    MPI_Type_free(&filter_type);
    MPI_Type_free(&filter_type1);
    MPI_Type_free(&args_type);
    MPI_Type_free(&column_type);

    MPI_Comm_free(&my_cartesian_comm);

    /* Terminate MPI execution */
    MPI_Finalize();

    return 0;
}

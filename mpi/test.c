    //printf("[%d][%d] Pass1\n",my_rank,getpid());
    
    /* Left upper process - active neighbours E, SE, S */
    if(my_rank == 0){

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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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
                                        my_image_before[1][j - 1] * my_args.filter[2][0]);

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
                //printf("rank %d\t src: %d\t tag: %d\n",my_rank,recv_stat.MPI_SOURCE,recv_stat.MPI_TAG);
            } // End for

            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < my_height_2; i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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
                                                my_image_before[i + 1][my_width - 1] * my_args.filter[2][0] +
                                                my_image_before[i][my_width - 1] * my_args.filter[2][0] +
                                                my_image_before[i - 1][my_width - 1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[i][my_width] < 0)
                    my_image_after[i][my_width] = 0;
                else if(my_image_after[i][my_width] > 255)
                    my_image_after[i][my_width] = 255;
            } // End for

            /* Right upper corner */
            my_image_after[1][my_width] = (int)(my_image_before[1][my_width] * my_args.filter[1][1] +
                                            my_image_before[2][my_width] * my_args.filter[2][1] +
                                            my_image_before[2][my_width - 1] * my_args.filter[2][0] +
                                            my_image_before[1][my_width - 1] * my_args.filter[2][0]);

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
                                        my_image_before[1][j - 1] * my_args.filter[2][0]);

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
                //printf("rank %d\t src: %d\t tag: %d\n",my_rank,recv_stat.MPI_SOURCE,recv_stat.MPI_TAG);
            } // End for

            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }
            
            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < my_height_2; i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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
                                                my_image_before[i + 1][my_width - 1] * my_args.filter[2][0] +
                                                my_image_before[i][my_width - 1] * my_args.filter[2][0] +
                                                my_image_before[i - 1][my_width - 1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[i][my_width] < 0)
                    my_image_after[i][my_width] = 0;
                else if(my_image_after[i][my_width] > 255)
                    my_image_after[i][my_width] = 255;
            } // End for

            /* Right lower corner */
            my_image_after[my_height][my_width] = (int)(my_image_before[my_height][my_width] * my_args.filter[1][1] +
                                                    my_image_before[my_height - 1][my_width] * my_args.filter[0][1] +
                                                    my_image_before[my_height][my_width - 1] * my_args.filter[1][0] +
                                                    my_image_before[my_height - 1][my_width - 1] * my_args.filter[0][0]);

            /* Truncated unexpected values */
            if(my_image_after[my_height][my_width] < 0)
                my_image_after[my_height][my_width] = 0;
            else if(my_image_after[my_height][my_width] > 255)
                my_image_after[my_height][my_width] = 255;

            /* Last line - except from left and right lower corners */
            for(j = 2; j < my_width; j++){
                my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                my_image_before[my_height - 1][j] * my_args.filter[0][1] +
                                                my_image_before[my_height - 1][j + 1] * my_args.filter[0][2] +
                                                my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                my_image_before[my_height][j - 1] * my_args.filter[2][0] +
                                                my_image_before[my_height - 1][j - 1] * my_args.filter[0][0]);

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
                //printf("rank %d\t src: %d\t tag: %d\n",my_rank,recv_stat.MPI_SOURCE,recv_stat.MPI_TAG);
            } // End for

            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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

            /* Left column - except from upper and lower left corners */
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
                                            my_image_before[my_height - 1][1] * my_args.filter[0][1] +
                                            my_image_before[my_height - 1][2] * my_args.filter[0][2] +
                                            my_image_before[my_height][2] * my_args.filter[1][2]);

            /* Truncated unexpected values */
            if(my_image_after[my_height][1] < 0)
                my_image_after[my_height][1] = 0;
            else if(my_image_after[my_height][1] > 255)
                my_image_after[my_height][1] = 255;

            /* Last line - execpt from left and right lower corners */
            for(j = 2; j < my_width; j++){
                my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                my_image_before[my_height - 1][j] * my_args.filter[0][1] +
                                                my_image_before[my_height - 1][j + 1] * my_args.filter[0][2] +
                                                my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                my_image_before[my_height][j - 1] * my_args.filter[2][0] +
                                                my_image_before[my_height - 1][j - 1] * my_args.filter[0][0]);
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
                //printf("rank %d\t src: %d\t tag: %d\n",my_rank,recv_stat.MPI_SOURCE,recv_stat.MPI_TAG);
            } // End for

            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];
    
            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
    } // End if d)
    /* First line processes - except from first and last process in this line - active neighbours E, SE , S, SW, W */
    else if(my_rank < procs_per_line_1){

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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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

            /* First line - execpt from left and right upper corners */
            for(j = 2; j < my_width; j++){
                my_image_after[1][j] = (int)(my_image_before[1][j] * my_args.filter[1][1] +
                                        my_image_before[1][j + 1] * my_args.filter[1][2] +
                                        my_image_before[2][j + 1] * my_args.filter[2][2] +
                                        my_image_before[2][j] * my_args.filter[2][1] +
                                        my_image_before[2][j - 1] * my_args.filter[2][0] +
                                        my_image_before[1][j - 1] * my_args.filter[2][0]);

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
                //printf("rank %d\t src: %d\t tag: %d\n",my_rank,recv_stat.MPI_SOURCE,recv_stat.MPI_TAG);
            } // End for

            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
    } // End if e)
    /* Right column processes - except from first and last process in this column - active neighbours N, S, SW, W, NW */
    else if(my_rank % procs_per_line == procs_per_line_1){

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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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
                                                my_image_before[i + 1][my_width - 1] * my_args.filter[2][0] +
                                                my_image_before[i][my_width - 1] * my_args.filter[2][0] +
                                                my_image_before[i - 1][my_width - 1] * my_args.filter[0][0]);

                /* Truncated unexpected values */
                if(my_image_after[i][my_width] < 0)
                    my_image_after[i][my_width] = 0;
                else if(my_image_after[i][my_width] > 255)
                    my_image_after[i][my_width] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            char fileName[10] = "";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
    } // End if f)
    /* Last line processes - except from first and last process in this line - active neighbours N, NE, E, W, NW */
    else if(my_rank > procs_per_line * procs_per_line_1){

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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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

            /* Last line - execpt from left and right lower corners */
            for(j = 2; j < my_width; j++){
                my_image_after[my_height][j] = (int)(my_image_before[my_height][j] * my_args.filter[1][1] +
                                                my_image_before[my_height - 1][j] * my_args.filter[0][1] +
                                                my_image_before[my_height - 1][j + 1] * my_args.filter[0][2] +
                                                my_image_before[my_height][j + 1] * my_args.filter[1][2] +
                                                my_image_before[my_height][j - 1] * my_args.filter[2][0] +
                                                my_image_before[my_height - 1][j - 1] * my_args.filter[0][0]);
                /* Truncated unexpected values */
                if(my_image_after[my_height][j] < 0)
                    my_image_after[my_height][j] = 0;
                else if(my_image_after[my_height][j] > 255)
                    my_image_after[my_height][j] = 255;
            } // End for

            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
    } // End if g)
    /* Left column processes - except from first and last process in this column - active neighbours N, NE, E, SE, S */
    else if(my_rank % procs_per_line == 0){

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
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
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

            /* Left column - except from upper and lower left corners */
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

            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
    } // End if h)
    /* Inner processes - all neighbours are active */
    else{

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
                    my_image_after[i][j] =  (int)(my_image_before[i][j] * my_args.filter[1][1] +
                                            my_image_before[i - 1][j] * my_args.filter[0][1] +
                                            my_image_before[i - 1][j + 1] * my_args.filter[0][2] +
                                            my_image_before[i][j + 1] * my_args.filter[1][2] +
                                            my_image_before[i + 1][j + 1] * my_args.filter[2][2] +
                                            my_image_before[i + 1][j] * my_args.filter[2][1] +
                                            my_image_before[i + 1][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i][j - 1] * my_args.filter[2][0] +
                                            my_image_before[i - 1][j - 1] * my_args.filter[0][0]);

                    /* Truncated unexpected values */
                    if(my_image_after[i][j] < 0)
                        my_image_after[i][j] = 0;
                    if(my_image_after[i][j] > 255)
                        my_image_after[i][j] = 255;
                } // End for
            } // End for


            /* Start receiving neighbours pixels/non-blocking */
            MPI_Startall(NUM_NEIGHBOURS, recv_requests);

            MPI_Status recv_stat;

            /* Keep receiving from all neighbours */
            for(k = 0; k < NUM_NEIGHBOURS; k++){
                MPI_Waitany(NUM_NEIGHBOURS, recv_requests, &index, &recv_stat);
                //printf("rank %d\t src: %d\t tag: %d\n",my_rank,recv_stat.MPI_SOURCE,recv_stat.MPI_TAG);
            } // End for
            
            char fileName[10]="";
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_after[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);

            /* Wait all pixles to be send before to procceeding to the next loop */
            MPI_Waitall(NUM_NEIGHBOURS, send_requests, MPI_STATUS_IGNORE);

            /* In the next loop perform convolution to the new image  - swapp images */
            tmp_ptr = my_image_before[0];

            my_image_before[0] = my_image_after[0];
            for(i = 1; i < my_height_2; i++)
                my_image_before[i] = &(my_image_before[0][i*(my_width_2)]);

            my_image_after[0] = tmp_ptr;
            for(i = 1; i < (my_height_2); i++)
                my_image_after[i] = &(my_image_after[0][i*(my_width_2)]);
        } // End of iter
    } // End else i)



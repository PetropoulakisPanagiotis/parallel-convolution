        //for(i = 1; i < my_height; i++)
          //  printf("rand %d i%d %d\n",my_rank,i, my_image_before[i][my_width + 1]);

        /*
        // Print array //
        char fileName[10];
        sprintf(fileName,"File%d",my_rank);

        FILE* my_file = fopen(fileName, "w");

        for(i = 0; i < my_height + 2; i++){
            for(j = 0; j < my_width + 2; j++){
                fprintf(my_file, "%d ", my_image_before[i][j]);
            }

            fprintf(my_file, "\n");
            fflush(my_file);
        }

        fclose(my_file);
*/


/* Save image before iretations, for check(can be removed) */
/////////////////////////////////////////////////////////////

    char fileName[10];
    sprintf(fileName,"File%dA",my_rank);

    FILE* my_file = fopen(fileName, "w");

    for(i = 0; i < my_height + 2; i++){
        for(j = 0; j < my_width + 2; j++){
            fprintf(my_file, "%d\t", my_image_before[i][j]);
        }
        fprintf(my_file, "\n");
    }

    fclose(my_file);

/* Save image after iteration, for check(can be removed) */
/////////////////////////////////////////////////////////////////////////

            char fileName[10];
            sprintf(fileName,"File%dB",my_rank);

            FILE* my_file = fopen(fileName, "w");

            for(i = 0; i < my_height + 2; i++){
                for(j = 0; j < my_width + 2; j++){
                    fprintf(my_file, "%d\t", my_image_before[i][j]);
                }
                fprintf(my_file, "\n");
            }

            fclose(my_file);



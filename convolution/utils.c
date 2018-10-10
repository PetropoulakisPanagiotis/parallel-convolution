/***********************************/
/* utils.c */
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
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#include "utils.h"

/*  Implementations of all functions that are defined in utils.h */

/* Read filter from the user */
/* Success: 0                */
/* Failure: -1               */
int read_filter(double filter[FILTER_SIZE][FILTER_SIZE]){
    char* num, *normalize; // Seperating values from input - normalize or not the filter
    char* line; // For getline
    size_t buff_size = 50; // Initial buffer size
    int total_sum = 0, line_sum; // Sum of filter values(for normalization) - Sum per line
    int i, j, error;


    /* Allocate memory for line buffer */
    line = malloc(sizeof(char) * buff_size);
    if(line == NULL)
        return -1;

    printf("Please enter the matrix of filter(%d x %d). Seperate lines with enter\n", FILTER_SIZE, FILTER_SIZE);

    /* Scan filter - Receive one line each time */
	for(i = 0; i < FILTER_SIZE; i++){
        line_sum = 0; // Sum of current line's integers

		error = getline(&line, &buff_size, stdin);
        if(error == -1){
            printf("An error occured. Try again later - read_filter(getline)");
            free(line);
            return -1;
        }

		line = strtok(line, "\n"); // Discard \n
        num = strtok(line, " \n\t"); // Get first number

        if(num == NULL){
           printf("No values given. Give line again\n");
            i--;
            continue;
        }

        j = 0; /* Count number of integers scanned from input */

        while(j < FILTER_SIZE){ // Get all integers

            /* Atoi on error returns 0, so we avoid the case when 0 is found */
            /* in order to know when 0 or invalid value was given            */
            if(!strcmp(num,"0"))
                filter[i][j] = 0;
            else{
                filter[i][j] = (double)atoi(num);

                /* Atoi error */
                if(filter[i][j] == 0){
                    printf("An error occured. Try again later - read_filter(atoi)");
                    free(line);
                    return -1;
                }

                if(filter[i][j] > FILTER_MAX_VALUE || filter[i][j] < FILTER_MIN_VALUE) // If true, invalid value was given
                    break;
            }

            line_sum += (int)filter[i][j];

			num = strtok(NULL," \n\t"); // Get the next number
			if(num == NULL)
				break;

            j++;
		} // End while

        /* Check if line given and all integers read were valid */
		if(j + 1 != FILTER_SIZE){
            printf("Invalid values given. Give line again\n");
			i--;
		}
        else
            total_sum += line_sum; // Line was valid, add it in total sum
	} // End for

    /* Avoid dividing with 0 - normalized filter will be the same */
    if(total_sum == 0)
        total_sum = 1;

    /* Normalize or not the filter */
    printf("If you want to normalize filter press y otherwise n:");

    /*  Read input */
    while(1){

        error = getline(&line, &buff_size, stdin);
        if(error == -1){
            printf("An error occured. Try again later - read_filter(getline_2)");
            free(line);
            return -1;
        }

        normalize = strtok(line, " \n\t");
        if(normalize == NULL || (strcmp(normalize, "y") && strcmp(normalize, "n") && strcmp(normalize, "yes") && strcmp(normalize, "no")))
            printf("Please enter a valid value for your answer(n or y)\n");
        else
            break;
    } // End while


    /* Normalize filter */
    if(!strcmp(normalize, "y") || !strcmp(normalize, "yes")){

        printf("NORMALIZED FILTER: \n");
        for(i = 0; i < FILTER_SIZE; i++){
            for(j = 0; j < FILTER_SIZE; j++){
                filter[i][j] = ceilf((filter[i][j] / (double)total_sum) * 100) / 100;
                printf("%.2f ",filter[i][j]);
            } // End for
            puts("");
        } // End for
    }

    free(line);

    return 0;
}

/* Read arguments for convolution */
/* Success: 0                     */
/* Failure: -1                    */
int read_user_input(Args_type* args, int procs_per_line){
    int error = 0;
    int i, input, stop;
    char buff[LINE_MAX];
    char *end;

    /* Check parameters */
    if(args == NULL){
        printf("An error occured. Try again later - read_user_input(NULL args)");
        return -1;
    }

    if(procs_per_line <= 0){
        printf("An error occured. Try again later - read_user_input(procs_per_line)");
        return -1;
    }

    printf("\nWelcome to parallel convolution program.\n");
    printf("----------------------------------------\n\n");

    /* Read input */
    for(i = 0; i < 5; i++){

        /* Print message */
        switch(i){
            case 0:
                printf("Enter the type of the image. 0 for black and white and 1 for multicolored:");
                break;

            case 1:
                printf("Enter the width of the image:");
                break;

            case 2:
                printf("Enter the height of the image:");
                break;

            case 3:
                printf("Enter the seed of the image(image will generated based in seed):");
                break;

            case 4:
                printf("Enter the number of iterations:");
                break;
        } // End switch

        stop = 0;

        /* Check imput */
        while(!stop){

            /* Read line */
            if(!fgets(buff, sizeof(buff), stdin)){
                printf("Please try again later. Fgets error(read_user_input)\n");
                return 1;
            }

            /* Empty line */
            if(buff[0] == '\n'){
                printf("Please enter a value\n\n");
                end = NULL; // Reset end
                i -= 1; // Repeat previous messages
                break;
            }

            /* Remove \n */
            buff[strlen(buff) - 1] = 0;

            errno = 0; //reset errno before call

            /* Convert line to integer */
            input = strtol(buff, &end, 10);
            if(input == 0 && errno != 0){
                printf("An error occured. Try again later - read_user_input(strtol)");
                return -1;
            }

            /* Check for errors - set parameters */
            switch(i){
                case 0:
                    if(end != buff + strlen(buff) || ((input != 0 && input != 1) || buff[0] == '\n')){
                        printf("Please enter valid value for image type(0 or 1):");
                        end = NULL; // Reset end
                    }
                    else{
                        args->image_type = input;
                        stop = 1;
                    }
                    break;

                case 1:
                    if(end != buff + strlen(buff) || input < procs_per_line || input > MAX_WIDTH){
                        printf("Please enter valid value for image width(equal or greater than %d and up to %d pixels):", procs_per_line, MAX_WIDTH);
                        end = NULL;
                    }
                    else{
                        args->image_width = input;
                        stop = 1;
                    }
                    break;

                case 2:
                    if(end != buff + strlen(buff) || input < procs_per_line || input > MAX_HEIGHT){
                        printf("Please enter valid value for image height(equal or greater than %d and up to %d pixels):", procs_per_line, MAX_HEIGHT);
                        end = NULL;
                    }
                    else{
                        args->image_height = input;
                        stop = 1;
                    }
                    break;

                case 3:
                    if(end != buff + strlen(buff) || input < 0 || input > MAX_SEED){
                        printf("Please enter valid value for seed(positive and up to %d):", MAX_SEED);
                        end = NULL;
                    }
                    else{
                        args->image_seed = input;
                        stop = 1;
                    }
                    break;
                case 4:
                    if(end != buff + strlen(buff) || input < 0 || input > MAX_ITERATIONS){
                        printf("Please enter valid value for iterations(positive and up to %d):", MAX_ITERATIONS);
                        end = NULL;
                    }
                    else{
                        args->iterations = input;
                        stop = 1;
                    }
                    break;
            } // End switch
        } // End while

        /* Reset end */
        end = NULL;
    } // End for

    /* Read filter */
    error = read_filter(args->filter);

    return error;
}

/* Debugging */
void printImage(int** my_image_before, int my_height_incr_2, int my_width_incr_2, int my_rank, int iter){
    char fileName[25];
    int i,j;

    sprintf(fileName,"File%d_%d", my_rank, iter);
    FILE* my_file = fopen(fileName, "w");

    for(i = 0; i < my_height_incr_2; i++){
        for(j = 0; j < my_width_incr_2; j++){
            fprintf(my_file, "%d\t", my_image_before[i][j]);
        }
        fprintf(my_file, "\n");
    }

    fclose(my_file);
}

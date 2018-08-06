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

#include "utils.h"

/*  Implementations of all functions that are defined in utils.h */

/* Read filter from the user */
/* Success: 0                */
/* Failure: -1               */
int read_filter(double filter[FILTER_SIZE][FILTER_SIZE]){
    char* num; // Seperating values from input
    size_t buff_size = 50; // For getline
    char* line; // For getline
    int i, j, error;
    int sum = 0; // sum of filter values for normalization

    /* Allocate memory for line buffer */
    line = malloc(sizeof(char) * buff_size);
    if(line == NULL)
        return -1;

    printf("Please enter the matrix of filter(%d x %d). Seperate lines with enter\n", FILTER_SIZE,FILTER_SIZE);

    /* Scan filter, receive one line each time */
	for(i = 0; i < FILTER_SIZE; i++){
        int line_sum = 0; // sum of current line's integers

		error = getline(&line, &buff_size,stdin);
        if(error == -1){
            free(line);
            return -1;
        }

		line = strtok(line,"\n"); /* Discard \n */
		num = strtok(line," \n\t"); /* Get first number */

        j = 0; /* Count number of integers scanned from input */

        while(j < FILTER_SIZE){ /* Get all integers */

            /* Atoi on error returns 0, so we avoid the case when 0 is found */
            /* in order to know when 0 or invalid value was given            */
            if(!strcmp(num,"0"))
                filter[i][j] = (double)0;
            else{
                filter[i][j] = (double)atoi(num);
                if(filter[i][j] == (double)0 || filter[i][j] > FILTER_MAX_VALUE || filter[i][j] < FILTER_MIN_VALUE) /* If true, invalid value was given */
                    break;
            }

            line_sum += (int)filter[i][j];

			num = strtok(NULL," \n\t"); /* Get the next number */
			if(num == NULL)
				break;

            j++;
		} /* End while */

        /* Check if line given and all integers read were valid */
		if(j + 1 != FILTER_SIZE){
            printf("Invalid values given. Give line again\n");
			i--;
		}
        else
            sum += line_sum; // line was valid, add sum
	} /* End for */

    printf("GIVEN FILTER: \n");
    for(int i = 0; i < FILTER_SIZE; i++){
        for(int j = 0; j < FILTER_SIZE; j++){
            printf("%d ",(int)filter[i][j]);
            if(sum != 0 && sum != 1) // WILL CHANGE LATER, JUST FOR PRINTING
                filter[i][j] /= (float)sum;
        }
        puts("");
    }

    printf("NORMALIZED FILTER: \n");
    for(int i = 0; i < FILTER_SIZE; i++){
        for(int j = 0; j < FILTER_SIZE; j++){
            printf("%f ",filter[i][j]);
        }
        puts("");
    }

    free(line);

    return 0;
}

/* Read parameters for the program */
/* Success: 0                      */
/* Failure: 1                      */
int read_user_input(Args_type* args, int procs_per_line){
    int error = 0;
    int i, input;
    char buff[LINE_MAX];
    char *end;

    /* Check pointer given */
    if(args == NULL){
        printf("Please try again later. NULL pointer(read_user_input)\n");
        return 1;
    }

    printf("Welcome to parallel convolution program.\n\n");
    printf("----------------------------------------\n\n");

    /* Read input */
    for(i = 0; i < 4; i++){

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
        } /* End switch */

        /* Check imput - read int */
        while(end != buff + strlen(buff)){

            /* Read line */
            if(!fgets(buff,sizeof(buff),stdin)){
                printf("Please try again later. Fgets error(read_user_input)\n");
                return 1;
            }

            /* Empty line */
            if(buff[0] == '\n'){
                printf("Please enter a value\n\n");
                end = NULL; /* Reset end */
                i -= 1; /* Repeat previous messages */
                break;
            }

            /* Remove \n */
            buff[strlen(buff) - 1] = 0;

            /* Convert line to integer */
            input = strtol(buff,&end,10);

            /* Check for errors - set parameters */
            switch(i){
                case 0:
                    if(end != buff + strlen(buff) || ((input != 0 && input != 1) || buff[0] == '\n')){
                        printf("Please enter valid value for image type(0 or 1):");
                        end = NULL; /* Reset end */
                    }
                    else
                        args->image_type = input;
                    break;

                case 1:
                    if(end != buff + strlen(buff) || input < procs_per_line || input > MAX_WIDTH){
                        printf("Please enter valid value for image width(equal or greater than %d and up to %d pixels):", procs_per_line, MAX_WIDTH);
                        end = NULL;
                    }
                    else
                        args->image_width = input;
                    break;

                case 2:
                    if(end != buff + strlen(buff) || input < procs_per_line || input > MAX_HEIGHT){
                        printf("Please enter valid value for image height(equal or greater than %d and up to %d pixels):", procs_per_line, MAX_HEIGHT);
                        end = NULL;
                    }
                    else
                        args->image_height = input;
                    break;

                case 3:
                    if(end != buff + strlen(buff) || input < 0 || input > MAX_SEED){
                        printf("Please enter valid value for seed(positive and up to %d):", MAX_SEED);
                        end = NULL;
                    }
                    else
                        args->image_seed = input;
                    break;
            } /* End switch */
        } // End while

        /* Reset end */
        end = NULL;
    } /* End for */

    /* Read filter */
    error = read_filter(args->filter);

    return error;
}

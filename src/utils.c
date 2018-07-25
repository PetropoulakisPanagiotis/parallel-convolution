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
#include "utils.h"

/*  Implementations of all functions that are defined in utils.h */

/* Allocate memory for filter */
/* Success: 0                 */
/* Failure: -1                */
int allocate_mem_filter(double*** filter){
    int i, j;

    /* Check given parameter */
    if(*filter == NULL){
        printf("Please try again later. NULL pointer(allocate_mem_filter)\n");
        return -1;
    }

    /* Create 2-d array/image (array of pointers to rows) */
    *filter = (double**)malloc(sizeof(double*) * FILTER_SIZE);
    if(*filter == NULL)
        return -1;

    for(i = 0; i < FILTER_SIZE; i++){ /* Create all rows */

        (*filter)[i] = (double*)malloc(sizeof(double) * FILTER_SIZE);
        if((*filter)[i] == NULL){ /* Error while allocating space */
            for(j = 0; j < i; j++)
                free((*filter)[i]);
            
            free(*filter);
            return -1;
        }
    } /* End for */

    return 0; 
}

/* De-allocate memory of filter */
/* Success: 0                   */
/* Failure: -1                  */
int free_mem_filter(double** filter){
    int i;

    /* Check parameters */
    if(filter == NULL){
        printf("Please try again later. NULL pointer(free_mem_filter)\n");
        return -1;
    }

    for(i = 0; i < FILTER_SIZE; i++)
        free(filter[i]);

    free(filter);

    return 0; 
}

/* Read filter from the user */
/* Success: 0                */
/* Failure: -1               */
int read_filter(double** filter){
    char* num; /* Seperating values from input */
    size_t buff_size = 50; /* For getline */
    char* line; /* For getline */
    int i, j, error;

    /* Allocate memory for line buffer */
    line = malloc(sizeof(char) * buff_size);
    if(line == NULL)
        return -1;

    printf("Please enter the matrix of filter(%d x %d). Seperate lines with enter\n",FILTER_SIZE,FILTER_SIZE);

    /* Scan filter, receive one line each time */
	for(i = 0; i < FILTER_SIZE; i++){

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
                if(filter[i][j] == 0) /* If true, invalid value was given */
                    break;
            }

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
	} /* End for */

    printf("GIVEN FILTER: \n");
    for(int i = 0; i < FILTER_SIZE; i++){
        for(int j = 0; j < FILTER_SIZE; j++){
            printf("%d ",(int)filter[i][j]);
        }
        puts("");
    }
    
    free(line);

    return 0;
}

/* Read parameters for the program */
/* Success: 0                      */
/* Failure: 1                      */
int read_user_input(int* image_type, int* image_width, int* image_height, int* image_seed, double** filter){
    int error = 0;
    int i = 0, input;
    char buff[LINE_MAX];
    char *end;

    /* Check parameters */
    if(image_type == NULL || image_width == NULL || image_height == NULL || image_seed == NULL || filter == NULL){
        printf("Please try again later. NULL pointer(read_user_input)\n");
        return 1;
    }

    printf("Welcome in parallel convolution program.\n\n");
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
                        *image_type = input;
                    break;

                case 1:
                    if(end != buff + strlen(buff) || input < 0 || input > MAX_WIDTH){
                        printf("Please enter valid value for image width(positive and up to %d pixels):",MAX_WIDTH);
                        end = NULL;
                    }
                    else
                        *image_width = input;
                    break;

                case 2:
                    if(end != buff + strlen(buff) || input < 0 || input > MAX_HEIGHT){
                        printf("Please enter valid value for image height(positive and up to %d pixels):",MAX_HEIGHT);
                        end = NULL;
                    }
                    else
                        *image_height = input;
                    break;

                case 3:
                    if(end != buff + strlen(buff) || input < 0 || input > MAX_SEED){
                        printf("Please enter valid value for seed(positive and up to %d):",MAX_SEED);
                        end = NULL;
                    }
                    else
                        *image_seed = input;
                    break;
            } /* End switch */
        } // End while

        /* Reset end */
        end = NULL;
    } /* End for */

    /* Read filter */
    error = read_filter(filter);

    return error;
}

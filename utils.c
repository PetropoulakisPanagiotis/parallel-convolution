#include "utils.h"

// Allocate memory for filter //
// Success: 0                 //
// Failure: 1                 //
int allocate_mem_filter(int*** filter){
    int i, j;

    // Check parameters //
    if(filter == NULL){
        printf("Please try again later. NULL pointer(allocate_mem_filter)\n");
        return 1;
    }

    // Create 2-d array/image //
    *filter = (int**)malloc(sizeof(int*) * FILTER_SIZE);
    if(*filter == NULL)
        return 1;

    for(i = 0; i< FILTER_SIZE; i++){
        (*filter[i]) = (int*)malloc(sizeof(int) * FILTER_SIZE);
        
        // Allocation failed, destroy filter //
        if((*filter[i]) == NULL){
            for(j = 0; j < i; j++)
                free((*filter[i]));
            free(*filter);
            return 1;
        }
    } // End for

    return 0;
}

// Clear memory of filter //
// Success: 0             //
// Failure: 1             //
int free_mem_filter(int** filter){
    int i;

    // Check parameters //
    if(filter == NULL){
        printf("Please try again later. NULL pointer(free_mem_filter)\n");
        return 1;
    }
    
    for(i = 0; i < FILTER_SIZE; i++)
        free(filter[i]);

    free(filter);

    return 0;
}

// Read filter from the user //
// Success: 0                //
// Failure: 1                //
int read_filter(char* buff, int buff_size, int** filter){
    int i, count, error = 0;
    char* current_num_str;
    int current_num;

    printf("Please enter the matrix of filter(3 x 3). Seperate lines with enter\n"); 

    // Read lines of filter //
    for(i = 0; i < FILTER_SIZE; i++){
        
        // Read current line //
        if(!fgets(buff,buff_size,stdin)){
            printf("Please try again later. Fgets error(read_filter)\n");
            return 1;
        }

        // Receive 3 integers. Convert line to integers //
        count = 0;
        while(1){
            if(count == 3)
                break;

            // Get each integer string//
            current_num_str = strtok(buff, " ");
            if(token == NULL)
                error = 1;
          
            if(error != 1){
                // Convert string to integer //
                current_num = strtol(current_num_str,&buff[sizeof(current_num_str) - 1],10);
                
                // Check if string is an integers //
                if(buff[sizeof(current_num_str)] == current_num_str)
                    error = 1;
                else
                    filter[i][count] = current_num; // Valid input - Copy number into matrix
            }

            // Check if error occured in current line //
            if(error == 1){
                printf("Please re-enter current line of matrix with valid values\n");
                // Read new line //
                if(!fgets(buff,buff_size,stdin)){
                    printf("Please try again later. Fgets error - inner(read_filter)\n");
                    return 1;
                }
                count = 0;
                continue;
            }

            count += 1;
        } // End while

    } // End for

    return 0;
}

// Read parameters for the program //
// Success: 0                      //
// Failure: 1                      //
int read_user_input(int* image_type, int* image_width, int* image_height, int* image_seed, int** filter){
    int error = 0;
    int i = 0, input;
    char buff[LINE_MAX];
    char *end;

    // Check parameters //
    if(image_type == NULL || image_width == NULL || image_height == NULL || image_seed == NULL || filter == NULL){
        printf("Please try again later. NULL pointer(read_user_input)\n");
        return 1;
    }

    printf("Welcome in parallel convolution program.\n\n");
    printf("----------------------------------------\n\n");
    
    // Read input //
    for(i = 0; i < 4; i++){

        // Print message //
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
        } // End switch
        
        // Check imput - read int //
        while(end != buff + strlen(buff)){
            if(!fgets(buff,sizeof(buff),stdin)){
                printf("Please try again later. Fgets error(read_user_input)\n");
                return 1;
            }
           
            // Empty line //
            if(buff[0] == '\n'){
                printf("Please enter a value\n\n");
                end = NULL; // Reset end 
                i -= 1; // Repeat previous messages
                break;
            }

            // Remove \n //
            buff[strlen(buff) - 1] = 0;

            // Convert line to integer //
            input = strtol(buff,&end,10);

            // Check for errors - set parameters //
            switch(i){
                case 0:
                    if(end != buff + strlen(buff) || (input != 0 && input != 1 || buff[0] == '\n')){
                        printf("Please enter valid value for image type(0 or 1):");
                        end = NULL; // Reset end 
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
            } // End switch
        } // End while

        // Reset end //
        end = NULL;
    } // End for

    // Read filter //
    error = read_filter(buff,LINE_MAX,filter);

    return error;
}



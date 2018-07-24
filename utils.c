#include "utils.h"

// Allocate memory for filter //
// Success: 0                 //
// Failure: 1                 //
int allocate_mem_filter(int*** filter){
    int i, j;

    *filter = (int**)malloc(sizeof(int*) * FILTER_SIZE);
    if(*filter == NULL)
        return 1;

    for(i = 0; i< FILTER_SIZE; i++){
        (*filter[i]) = (int*)malloc(sizeof(int) * FILTER_SIZE);
        if((*filter[i]) == NULL){
            for(j = 0; j < i; j++)
                free((*filter[i]));
            return 1;
        }
    } // End for

    return 0;
}

// Clear memory of filter //
void free_mem_filter(int** filter){
    int i;

    for(i = 0; i < FILTER_SIZE; i++)
        free(filter[i]);

    free(filter);
}

// Read parameters for the program //
// Success: 0                      //
// Failure: 1                      //
void read_user_input(int* image_type, int* image_width, int* image_height, int* image_seed){
    int error;
    int i = 0, input;
    char buff[LINE_MAX];
    char *end;

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
                printf("An error occured(read input). Please try again later\n");
                exit(1);
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
}

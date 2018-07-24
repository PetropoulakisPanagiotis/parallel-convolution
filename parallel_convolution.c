#include <stdio.h>
#include "utils.h"

int main(void){
    int image_type, image_width, image_height, image_seed;
    int **filter, k, l;
    int error;

    // Create filter //
    error = allocate_mem_filter(&filter);
    if(error != 0){
        printf("Please try again later(filter allocation)\n");
        return 1;
    }

    read_user_input(&image_type,&image_width,&image_height,&image_seed);

    free_mem_filter(filter);

    return 0;
}


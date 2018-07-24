#include <stdio.h>
#include "utils.h"

int main(void){
    int image_type, image_width, image_height, image_seed;
    int **filter;
    int error;

    // Create filter //
    error = allocate_mem_filter(&filter);
    if(error != 0)
        return error;
    

    // Read parameters of program //
    error = read_user_input(&image_type,&image_width,&image_height,&image_seed);
    if(error != 0)
        return error;

    error = free_mem_filter(filter);
    if(error != 0)
        return error;

    return 0;
}


/***********************************/
/* utils.h */
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
#pragma once

/* Definitions */
#define MAX_WIDTH 1000 // Max width of pictures
#define MAX_HEIGHT 1000 // Max Height of pictures
#define MAX_SEED 50 // For randomly generated pictures
#define FILTER_SIZE 3 // 3 x 3 -> Must not be changed
#define FILTER_MAX_VALUE 10 // Max value for filter

/*  Header file for all variant functions and structs used
 *  to complete the Parallel Convolution application.
 */


 /* Arguments struct for each mpi process */
 typedef struct Args_type{
     int image_type, image_width, image_height, image_seed;
     double filter[FILTER_SIZE][FILTER_SIZE];
     int width_per_process, width_remaining;
     int height_per_process, height_remaining;
 }Args_type;

/*  Gets the filter of convolution. Given an already allocated array    */
/*  scans from stdin the filter line by line. When the line is given    */
/*  all integers are seperated and are put in filter. In case of        */
/*  error, the line is rescanned.                                       */
int read_filter(double[FILTER_SIZE][FILTER_SIZE]);

/*  NOT FULLY CHECKED. LOOKS OKAY   */
int read_user_input(int*,int*,int*,int*,double[FILTER_SIZE][FILTER_SIZE]);

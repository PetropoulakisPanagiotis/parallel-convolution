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
#define MAX_WIDTH 3500 // Max width of pictures
#define MAX_HEIGHT 3500 // Max Height of pictures
#define MAX_SEED 50 // For randomly generated pictures
#define FILTER_SIZE 3 // 3 x 3 -> Must not be changed
#define FILTER_MAX_VALUE 10 // Max value for filter
#define FILTER_MIN_VALUE -10
#define MAX_ITERATIONS 200 

/* Neighbours */
#define N 0
#define NE 1
#define E 2
#define SE 3
#define S 4
#define SW 5
#define W 6
#define NW 7

/*  Header file for all variant functions and structs used
 *  to complete the Parallel Convolution application.
 */


 /* Arguments struct for each mpi process */
 typedef struct Args_type{
     int image_type, image_width, image_height, image_seed;
     double filter[FILTER_SIZE][FILTER_SIZE];
     int width_per_process, width_remaining;
     int height_per_process, height_remaining;
     int iterations;
 }Args_type;

/*  Gets the filter of convolution. Given an already allocated array    */
/*  scans from stdin the filter line by line. When the line is given    */
/*  all integers are seperated and are put in filter. In case of        */
/*  error, the line is rescanned.                                       */
int read_filter(double[FILTER_SIZE][FILTER_SIZE]);

/*  NOT FULLY CHECKED. LOOKS OKAY   */
int read_user_input(Args_type*, int procs_per_line);

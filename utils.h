#pragma once 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Defines //

#define MAX_WIDTH 1000
#define MAX_HEIGHT 1000
#define MAX_SEED 50
#define FILTER_SIZE 3 // 3 x 3

// Prototypes functions //
int allocate_mem_filter(int***);
void free_mem_filter(int**);

void read_user_input(int*,int*,int*,int*);

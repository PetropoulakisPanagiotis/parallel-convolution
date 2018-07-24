#pragma once 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Defines //

#define MAX_WIDTH 1000
#define MAX_HEIGHT 1000
#define MAX_SEED 50
#define FILTER_SIZE 3 // 3 x 3 -> Must not changed
#define FILTER_MAX_VALUE 10 // Max integer in filter

// Prototypes functions //
int allocate_mem_filter(int***);
void free_mem_filter(int**);

int read_filter(char*,int,int*);
int read_user_input(int*,int*,int*,int*,int**);

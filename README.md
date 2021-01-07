[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# Parallel Convolution
<p align="center">
<img src="https://i1.wp.com/jeanvitor.com/wp-content/uploads/2017/07/6zX2c.png?w=364&ssl=1">
</p>

Parallel convolution performs n convolutions to an image using MPI and OpenMP for parallelism

## How It Works
The image is partitioned into square blocks and each process is "mapped" to a specific block. At first, each process convolutes only the inner pixels with a given kernel, and then, each process exchanges with its neighbours its outer pixels to perform a convolution to the remaining pixels. This process is repeated n times. 
<br />
Notes:
* You can find some plots inside the measurements folder 
* Filter size = 3*3
* All processes are called workers. Process 0 sends initially the input image to other workers, and in the end, the same process prints some statistics.
* In plots, the pre and post-processing time are excluded.
* Max neighbours per process = 8 
* The input image must be square

## Key Features
* Persistent Communication
* Virtual Topology - Cartesian 
* Derived Data Types - Vector, Struct, Contiguous
* Non-Blocking Communication
* Dynamic Receiving - MPI_Status, MPI_Waitany, use of MPI_PROC_NULL 
* Static Scheduling with OpenMP

## Requirements
* MPI-3 
* OpenMP

## Compilation and Excecution
```
$ cd parallel-convolution/convolution
$ make
$ mpiexec -n 4 ./convolution
```

## Contributing
* Introduce parallel I/O to read a real input image

## Helpful Links: 
1. http://matlabtricks.com/post-5/3x3-convolution-kernels-with-online-demo
2. http://setosa.io/ev/image-kernels/
3. https://lodev.org/cgtutor/filtering.html
4. https://www.openmp.org/
5. https://www.mpich.org/static/downloads/3.2.1/mpich-3.2.1-userguide.pdf

## Authors
* Petropoulakis Panagiotis petropoulakispanagiotis@gmail.com
* Andreas Charalambous and.charalampous@gmail.com

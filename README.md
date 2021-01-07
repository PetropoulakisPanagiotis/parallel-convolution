[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# Parallel Convolution
<p align="center">
<img src="https://i1.wp.com/jeanvitor.com/wp-content/uploads/2017/07/6zX2c.png?w=364&ssl=1">
</p>

Parallel convolution performs n convolutions to an image using MPI and OpenMP for parallelism

## How It Works
Partition image into block, so as each process convolutes its part using a given kernel. In the beginning, all processes convolute the "inner" pixels and then the "outer" pixels should be transmited to the neigbros processes to perform convolution to the "outer" pixels. 
<br />
Notes:
* Filter = 3*3
* Given filter can be normalized
* All processes are workers. Process 0 just sends the user input in the other processes. After that process 0 is a worker. In the end process 0 will print some results.
* Timers only includes computation and messages(pre/post-procesing is excluded)
* Max heighbours per proces = 8 
* Given number of processes must be a perfect square (For image division)
* In the files _reduce.c every process checks for the convergence of the image. More specifically a generated image can convergence in the <br />
60 loop out of 100 loops. In this scenario process 0 will print a message and the remaining loops will continue.

## Features
* Persistent Communication
* Virtual Topology - Cartesian 
* Derived Data Types - Vector, Struct, Contiguous
* Non - Blocking Communication
* Dynamic Receiving - MPI_Sttus, MPI_Waitany, use of MPI_PROC_NULL 
* Static scheduling with OpenMp

## Requirements
* MPI-3 
* OpenMP

## Compilation/Run
1. make
2. mpiexec -n 4 ./convolution <br />
Note: Run make file inside folders  

## Contributing
* Introduce parallel I/O: Handle a real image <br />
Note: The user will enter the image path, the filter and the number of iterations

## Helpful Links: 
1. http://matlabtricks.com/post-5/3x3-convolution-kernels-with-online-demo
2. http://setosa.io/ev/image-kernels/
3. https://lodev.org/cgtutor/filtering.html
4. https://www.openmp.org/
5. https://www.mpich.org/static/downloads/3.2.1/mpich-3.2.1-userguide.pdf

## Authors
* Petropoulakis Panagiotis petropoulakispanagiotis@gmail.com
* Andreas Charalambous and.charalampous@gmail.com

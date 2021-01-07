[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# Parallel Convolution
<p align="center">
<img src="https://i1.wp.com/jeanvitor.com/wp-content/uploads/2017/07/6zX2c.png?w=364&ssl=1">
</p>

Parallel convolution performs n convolutions to an image using MPI and OpenMP for parallelism

## How It Works
Partition image into blocks, so as each process convolutes its own part using a given kernel. In the beginning, all processes convolute the "inner" pixels, and then, the "outer" pixels should be transmitted to the neighbours processes. 
<br />
Notes:
* You can find some plots that reside inside the measurements folder 
* Filter = 3*3
* All processes are workers. Process 0 just sends the user input in the other processes. After that process 0 is a worker. In the end process 0 will print some results.
* Timers only includes computation and messages(pre and post-processing are excluded)
* Max neighbours per process = 8 
* The input number of processes must be a perfect square

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
$ mpiexec -n 4 ./convolution <br />
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

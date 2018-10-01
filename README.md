[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# Parallel Convolution with random image
<img src="https://i1.wp.com/jeanvitor.com/wp-content/uploads/2017/07/6zX2c.png?w=364&ssl=1"> <br />
A Random image is generated and then convolution is performed with a given filter. Convolution can be performed many times(based in the user-input)  <br /> 

## How It Works
Type of data partition is block, so every process has it's own portion of the original image. In the beginning, all processes convolute <br /> inner pixels. After that processes should exchange their outer pixels via messages and then perform convolution to outer pixels.
<br /> 
Notes:
* Filter = 3 X 3 (can not be changed)
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
* MPI-3 <br />
* OpenMp <br />

## Compilation/Run
1. make
2. mpiexec -n 4 ./filter <br />
Note: Run make file inside folders  

## Results - MPI
![alt text](https://github.com/PetropoulakisPanagiotis/parallel-convolution/blob/master/results/mpi-bw.png)
![alt text](https://github.com/PetropoulakisPanagiotis/parallel-convolution/blob/master/results/mpi-reduce-bw.png)
![alt text](https://github.com/PetropoulakisPanagiotis/parallel-convolution/blob/master/results/mpi-rgb.png)
![alt text](https://github.com/PetropoulakisPanagiotis/parallel-convolution/blob/master/results/mpi-reduce-rgb.png)

* Input
* Resolution: 1920 x 2520
* Seed: 3
* Iterations: 150
* Filter: Sharpen

## Machines statistics
* I5-6500 cpu @ 3.20 GHz
* 4 CPU per machine
* RAM: 33MHz - 15 GB per machine

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

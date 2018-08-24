[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# Parallel Convolution with random image
![alt text](data:image/png;base64,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)
A Random image is generated and then convolution is performed with a given filter. Convolution can be performed many times(based in user-input)  <br /> 
Notes:
* Filter = 3 X 3 (can not be changed)
* Given filter will be normalized
* All processes are workers. Process 0 just sends user input in the other processes. After that process 0 is a worker. In the end <br />
  process 0 will print some results
* Timers only includes computation and messages(pre/post-procesing is excluded)
* Every pixel has 8 heighbours 
* Given number of processes must be a perfect square (For image division)

## Prerequisites
* MPI-3 <br />
Instructions: https://www.mpich.org/static/downloads/3.2.1/mpich-3.2.1-userguide.pdf

## Compilation/Run
1. make
2. mpiexec -n 4 ./filter
Note: Run make file inside src folder  

## Helpful Links: 
1. http://matlabtricks.com/post-5/3x3-convolution-kernels-with-online-demo
2. http://setosa.io/ev/image-kernels/
3. https://docs.gimp.org/en/plug-in-convmatrix.html

## Authors
* Petropoulakis Panagiotis petropoulakispanagiotis@gmail.com
* Andreas Charalambous and.charalampous@gmail.com

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
# Parallel Convolution with random image
![alt text](https://i1.wp.com/jeanvitor.com/wp-content/uploads/2017/07/6zX2c.png?w=364&ssl=1) <br />
A Random image is generated and then convolution is performed with a given filter. Convolution can be performed many times(based in user-input)  <br /> 

## How It Works
A Random image is generated and then convolution is performed with a given filter. Convolution can be performed many times(based in user-input)  <br /> 
Notes:
* Filter = 3 X 3 (can not be changed)
* Given filter will be normalized
* All processes are workers. Process 0 just sends user input in the other processes. After that process 0 is a worker. In the end <br />
  process 0 will print some results
* Timers only includes computation and messages(pre/post-procesing is excluded)
* Every pixel has 8 heighbours 
* Given number of processes must be a perfect square (For image division)

## Requirements
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

# Parallel Convolution

Program generates random image and then perform convolution with a given filter(x times) <br /> 
Notes:
* Filter = 3 X 3
* Given filter will be normalized
* All processes are workers. Process 0 just send user input in the other processes. After that process 0 is a worker. In the end <br />
  process 0 print some results
* Timers only includes computation and messages(pre/post-procesing is excluded)
* Every pixel has 8 heighbours. 
* Given number of processes must be a perfect square

Helpful Links:
* http://matlabtricks.com/post-5/3x3-convolution-kernels-with-online-demo
* http://setosa.io/ev/image-kernels/
* https://docs.gimp.org/en/plug-in-convmatrix.html

# Compilation
make

# Run
mpicc -n 4 ./filter

# Data division
![alt text](https://raw.githubusercontent.com/PetropoulakisPanagioti/parallel_convolution/master/images/1.png)

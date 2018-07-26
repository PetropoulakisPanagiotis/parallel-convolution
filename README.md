# Parallel Convolution

Program generates random image and then perform convolution with a given filter(x times) <br /> 
Notes:
* Filter = 3 X 3 

Helpful Links:
* http://matlabtricks.com/post-5/3x3-convolution-kernels-with-online-demo
* http://setosa.io/ev/image-kernels/
* https://docs.gimp.org/en/plug-in-convmatrix.html

# Compilation
make

# Run
mpicc -n 4 ./filter

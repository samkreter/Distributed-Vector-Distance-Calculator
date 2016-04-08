Vector L2 Norm Over a Cluster
=============================


Running
-------

1. create a directory in the main project folder called _build.

2. cd into the folder and execute ` cmake .. `

3. next execute ` make ` to compile the project

4. To run the program

        mpirun -n [number of spawns] ./main [value for k]

5. The top k results will be stored in results.csv in the _build directory
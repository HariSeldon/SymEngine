SymEngine: Symbolic Execution of OpenCL Kernels
===============================================

SymEngine symbolically executes an OpenCL kernel so to extract symbolic representations of the memory accesses of the kernel.
SymEngine relies on Scalar Evolution for the computation of the addresses. 

SymEngine uses 3 config files.
1. hw_config.yaml contains information about the hardware configuration of the considered GPU.
This information include: number of local memory banks, width of a local memory bank, size in Bytes of a cache line and number of threads per warp.
2. kernel_arg_config.yaml contains a collection of kernel names with a list of numbers corresponding to the values of known integer kernel arguments. 
These are typically buffer sizes. 
The matching between the nubers and the kernel arguments is positional: the first number in the list corresponds to the first integer argument in the kernel. 
3. opencl_config.yaml contains the NDRange configuration used to symbolically execute the kernel.
SymEngine requires to know the size of a local group, the total number of groups in the NDRange space, what warp to simulate and to which group it belongs. 
The information about the size of the NDRange from opencl_config.yaml is only used if SymEngine if the command lines localSize[XYZ] and numberOfGroups[XYZ] are not given.

SymEngine usually executes only the specified warp.
This behaviour can be changed using the --full-simulation option. 
This triggers the symulation of all the threads in the selected work group, giving in output the accumulated result. 
This feature is useful to take into account control flow statements. 

Validation of the tool is given in the pdf file named "symexe_vs_hwcounters.pdf" .
The output of SymEngine is compared against hardware profiler counters collected from an Nvidia GTX 480.
Deviations between the prediction and the actual hardware are usually due to control flow or loop bounds not being model correctly.
Benchmarks control-flow free (like mt (matrix transposition) or blacksholes) are modelled correctly.


If you want SymEngine dump LLVM-IR with metadata containing the result of the computation replace /dev/null with - in symEngine.sh

Examples
--------

To try SymEngine execute the following command from the /scripts directory.
(Remember to set the content of the opencl_config.yaml configuration file to represent a 2D kernel.)

./symEngine.sh ../kernels/mt.cl mt
"symEngine.sh" is a script that takes in input an OpenCL-C file and a kernel name.
The expected output is: 1 load transaction and 32 store transactions.
Since the load is coalesced while the store is perfectly uncoalesced.
To try with a fully coalesced version of the same benchmark.
./symEngine.sh ../kernels/mtLocal.cl mtLocal 
In this case the expected output is 1 transaction for both the global load and global store.
This because local memory is used to stage data temporarely. 
SymEngine also shows the number of conflicts for the local memory banks.

To test the --full-simulation option try to run:
./symEngine.sh ../kernels/reduce.cl reduce
(Remember to set the content of the opencl_config.yaml configuration file to represent a 1D kernel.)
Notice that the total number of store transactions is 1. This is because only thread number 0 in the group stores to main memory.

Prerequisites
-------------

SymEngine has been tested with LLVM 3.6 and clang 3.6

For any questions please contact [Alberto Magni][email/alberto]

[email/alberto][mailto:alberto.magni86@gmail.com]

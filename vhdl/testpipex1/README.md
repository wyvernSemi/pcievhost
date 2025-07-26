# VHDL Test Environment

The VHDL demonstration test bench in this directory uses the same user source files as the Verilog test bench, which are located in the `verilog/testpipex1` directory, and the make files make references to those C and C++ files. The two test folders are:

* `verilog/test/usercode`
* `verilog/test/usercodeEnum`

To select from the default of `usercode` the make command line can specify `USRCDIR=usercodeEnum`, without the need to specify the path the the verilog test location.
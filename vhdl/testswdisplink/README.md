# testswdisplink Test Bench

Top level VHDL test bench for verifying the wide (parallel) and serial _pcieVHost_ software displink functionality

## Compiling and Running Test for Parallel Software Link Display

The default test bench uses the `test.vhd` file defining a top level `test`. Two _pcieVHost_ components are instantiated at nodes 0 and 1 (for RC and EP respectively) and the test is configuired for a 2 lane link, with full 8b10b encoding. The diagram below shows the layout for this test bench.

<p align=center><img src="images/pcieswdisplinkpar.png" width=1000></p>

Compilation between the simulators is very similar, but the correct make file must be selected for each to get the simulator specific settings. (A `makefile.common` file houses the commmon settings and is included by the simulator specific make files.). The examples below for the supported simulators show compiling and running in batch mode. Other modes are available and `make [-f <makefile>] help` displays the options.

### Questa

```
    make run
```

### GHDL

```
    make -f makefile.ghdl run
```

### NVC

```
    make -f makefile.nvc run
```


## Compiling and Running Test for Serial Software Link Display

The serial link display test bench uses the same `test.vhd` top level file but requires a generic to be defined. Two _pcieVHost_ serial components are instantiated at nodes 0 and 1 (for RC and EP respectively) with the serial link input5 and output signals, and the serial signals sent to two `PcieSwDispLinkSer` modules. As for the parallel test, the settings are configuired for a 2 lane link, with full 8b10b encoding. The diagram below shows the layout for this test bench.

<p align=center><img src="images/pcieswdisplinkser.png" width=1000></p>

For the serial link display test the compilation between the simulators require some more specific settings on the command line, using the appropriate make file. A different top level test module must be selected by setting the `PCIE_TOP` make variable, and this is common between all the simulators. A Verilog definition, `SERIALTEST` must be specified to change some internal settings, and the exact method varies between simulators. A `USRVLOGFLAGS` make file variable is used to add the approriate command line flag for the simulator. The examples below show compiling and running in batch mode for the supported simulators with the make file variable setting required. Other modes are available and `make [-f <makefile>] help` displays the options.

### Questa

```
    make SERIALISER=1 run
```
### GHDL

```
    make -f makefile.ghdl SERIALISER=1 run
```

### NVC

```
    make -f makefile.nvc SERIALISER=1 run
```


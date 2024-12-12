# pcievhost
PCIe (1.0a to 2.0) Virtual host model for Verilog and SystemVerilog logic simulation environments.

The _pcievhost_ model generates PCIe Physical, Data Link and Transaction Layer traffic for up to 16 lanes, controlled from user C program, via a comprehensive API. It has configurable internal memory and configuration space models, and will auto-generate completions (configurably), with flow control, ACKs, and NAKS etc. The protocol itself is modelled in C and is integrated with a logic simulation using the [_VProc_](https://github.com/wyvernSemi/vproc) virtual processor. The diagram below shows the structure of the model which ultimately generates a stream of 8b10b encoded symbols, and processes the resturned symbols.

<p align="center">
<img src="https://github.com/user-attachments/assets/3b387f4f-8ca6-4ac9-b169-c6f94e77ecb7" width=640>
</p>

_pcievhost_ is bundled with verilog pcie link traffic display modules and an example test harness. The model has been tested with ModelSim/Questa, Vivado xsim and Verilator at the present time, though easily adpated for other simulators. The _pcievhost_ model can also be configured to act as an endpoint via a parameter and with simple running user code&mdash;the model itself automatically generating responses to transactions. The diagram below shows the example test bench structure.

<p align="center">
<img src="https://github.com/user-attachments/assets/cc504204-4308-4b87-9afd-5013b26aa468" width=640>
</p>

More information can be found in the documentation <code>doc/pcieVHost.pdf</code>

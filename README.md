# pcievhost
PCIe (1.0a to 2.0) Virtual host model for VHDL, Verilog and SystemVerilog logic simulation environments.

The _pcievhost_ model generates PCIe Physical, Data Link and Transaction Layer traffic for up to 16 lanes, controlled from a user C program, via a comprehensive API. It has configurable internal memory and configuration space models, and will auto-generate completions (configurably), with flow control, ACKs, and NAKS etc. The protocol itself is modelled in C and is integrated with a logic simulation using the [_VProc_](https://github.com/wyvernSemi/vproc) virtual processor. Below is a list of features of the model:

* All lane widths up to 16
* Internal memory space accessed with incoming write/read requests (can be disabled)
* Auto-generation of read completions (can be disabled)
* Auto-generation of 'unsupported' completions (can be disabled)
* Auto-generation of Acks/Naks (can be disabled)
* Auto-generation of Flow control (can be disabled)
* Auto-generation of Skip OS (can be disabled)
* User generation of all TLP types
  * Memory Reads/Writes
  * Read completions
  * Config Reads/Writes
  * IO Reads/Writes
  * Messages
* User generation of all DLLP types
  * Acks/Naks
  * Flow control
  * Power management
  * Vendor
* User generation of all training sequences
* User generation of all ordered sets
* User generation of idle
* 8b10b encoding and decoding (can be disabled)
* Scrambling and Descrambling (can be disabled)
* Proper throttling on received flow control
* Lane reversal
* Lane Inversion
* Serial input/output support
* Programmable FC delay (via Rx packet consumption rates)
* Programmable Ack/Nak delay
* LTSSM (partial implementation)

The diagram below shows the structure of the model which ultimately generates a stream of 8b10b encoded symbols, and processes the returned symbols.

<p align="center">
<img src="https://github.com/user-attachments/assets/32a4c6d9-e71f-4ece-89e4-3c49cc1c7c76" width=740>
</p>

_pcievhost_ is bundled with verilog pcie link traffic display modules and an example test harness. The model has been tested with Questa (Verilog and VHDL), Vivado xsim, Verilator, NVC and GHDL at the present time, though easily adpated for other simulators. The _pcievhost_ model can also be configured to act as an _**endpoint**_ via a parameter and with simple running user code&mdash;the model itself automatically generating responses to transactions. The diagram below shows the example test bench structure.

<p align="center">
<img src="https://github.com/user-attachments/assets/7701ffa3-f556-4006-a16e-46ec2942c87a" width=800>
</p>

More information can be found in the documentation <code>doc/pcieVHost.pdf</code>

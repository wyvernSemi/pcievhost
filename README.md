# pcievhost
PCIe (1.0a to 2.0) Virtual host model for verilog.

Generates PCIe Physical, Data Link and Transaction Layer traffic for up to 16 lanes, controlled from user C program, via an API. Has configurable internal memory and configuration space models, and will auto-generate completions (configurably), with flow control, ACKs, and NAKS etc. It is also bundled with verilog pcie link traffic display modules, and an example test harness.

This branch uses SystemVerilog and is targetted at the ALDEC simulators, Riviera-PRO and Active-HDL.

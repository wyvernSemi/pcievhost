# pcievhost
PCIe (1.0a to 2.0) Virtual host model for verilog.

Generates PCIe Physical, Data Link and Transaction Layer traffic for up to 16 lanes, controlled from user C program, via an API. Has configurable internal memory and configuration space models, and will auto-generate completions (configurably), with flow control, ACKs, and NAKS etc.

Is bundled with verilog pcie link traffic display modules, and an example test harness. Tested for ModeSim only at the present time, though easily adpated for VCS, NC-Verulog and Icarus (and has previously been running on these in the past).

# pcievhost
PCIe (1.0a to 2.0) Virtual host model for verilog.

Generates PCIe Physical, Data Link and Transaction Layer traffic for up to 16 lanes, controlled from user C program, via an API. Has configurable internal memory and configuration space models, and will auto-generate completions (configurably), with flow control, ACKs, and NAKS etc.

pcievhost is bundled with verilog pcie link traffic display modules and an example test harness. Tested for ModelSim/Questa only at the present time, though easily adpated for VCS, NC-Verilog and Icarus (and has previously been running on these in the past).
<p align="center">
<img src="https://github.com/wyvernSemi/pcievhost/assets/21970031/6e21c9da-dd50-4872-b385-fe67845dd16b" width=640>
</p>
More informaton can be found in the documentation <code>doc/pcieVHost.pdf</code>

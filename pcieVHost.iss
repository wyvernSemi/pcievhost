
[Setup]
AppName=pcieVHost
AppVerName=PCIEVHOST_1_0_0
DefaultDirName={src}\pcieVHost
DisableProgramGroupPage=yes
OutputBaseFilename=setup_pcievhost_1_0_0

[Dirs]
Name: "{app}\doc"
Name: "{app}\lib\obj"
Name: "{app}\src"
Name: "{app}\verilog"

[Files]
Source:"pcieVHost.iss";                         DestDir:"{app}"
                                                
Source:"doc\index.html";                        DestDir:"{app}\doc"; DestName: "pcieVHost.html"
Source:"doc\images\*.png";                      DestDir:"{app}\doc\images"
Source:"doc\images\*.jpg";                      DestDir:"{app}\doc\images"
                                                
Source:"lib\makefile";                          DestDir:"{app}\lib"
                                                 
Source:"src\*.c";                               DestDir:"{app}\src"
Source:"src\*.h";                               DestDir:"{app}\src"
                                                
Source:"verilog\headers\headers.vc";            DestDir:"{app}\verilog\headers"
Source:"verilog\headers\*.v";                   DestDir:"{app}\verilog\headers"; Excludes:"*pcie_vhost_map.v"
Source:"verilog\lib\lib.vc";                    DestDir:"{app}\verilog\lib"
Source:"verilog\lib\*.v";                       DestDir:"{app}\verilog\lib"
Source:"verilog\PcieDispLink\PcieDispLink.vc";  DestDir:"{app}\verilog\PcieDispLink"
Source:"verilog\PcieDispLink\*.v";              DestDir:"{app}\verilog\PcieDispLink"
Source:"verilog\pcieVHost\pcieVHost.vc";        DestDir:"{app}\verilog\pcieVHost"
Source:"verilog\pcieVHost\*.v";                 DestDir:"{app}\verilog\pcieVHost"

Source:"verilog\test\test.vc";                  DestDir:"{app}\verilog\test"
Source:"verilog\test\test.v";                   DestDir:"{app}\verilog\test"
Source:"verilog\test\ContDisps.v";              DestDir:"{app}\verilog\test"
Source:"verilog\test\usercode\*.c";             DestDir:"{app}\verilog\test\usercode"
Source:"verilog\test\hex\ContDisps.hex";        DestDir:"{app}\verilog\test\hex"


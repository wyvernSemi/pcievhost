//=============================================================
// 
// Copyright (c) 2026 Simon Southwell. All rights reserved.
//
// Date: 26th Feb 2026
//
// This file is part of the pcieVHost package.
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

`ifdef VPROC_SV
`include "allheaders.v"
`endif

`WsTimeScale

module clkmux (
  input  aresetn,
  input  sel,
  input  clka,
  input  clkb,
  output clkout
);

wire clkagated;
wire clkbgated;

reg selclka;
reg selclkb;
reg selclka_p1;
reg selclkb_p1;

assign clkagated = clka & selclka;
assign clkbgated = clkb & selclkb;

assign clkout    = clkagated | clkbgated;

always @(posedge clka or negedge aresetn)
begin
  if (aresetn == 1'b0)
    selclka_p1 <= 1'b0;
  else
    selclka_p1 <=  sel & ~selclkb; 
end

always @(negedge clka or negedge aresetn)
begin
    if (aresetn == 1'b0)
      selclka  <= 1'b0;
    else
      selclka    <= selclka_p1;
end

always @(posedge clkb or negedge aresetn)
begin
  if (aresetn == 1'b0)
    selclkb_p1 <= 1'b0;
  else
    selclkb_p1 <= ~sel & ~selclka;
end

always @(negedge clkb or negedge aresetn)
begin
  if (aresetn == 1'b0)
    selclkb    <= 1'b0;
  else
    selclkb    <= selclkb_p1;
end

endmodule

`ifdef TEST

// ==============================================================
// Local test
// ==============================================================

module test ();

wire clkout;

reg aresetn;
reg clka, clkb;
reg sel;

initial
begin
  clka = 1'b0;
  clkb = 1'b0;
  sel  = 1'b0;  
  fork
    forever #2500 clka = ~clka;
    forever #1000 clkb = ~clkb;
  join
end

  clkmux clkmux_i
  (
    .aresetn  (aresetn),
    .sel      (sel),
    .clka     (clka),
    .clkb     (clkb),
    .clkout   (clkout)
  );
  
initial
begin
  aresetn        = 1'b0;
  #5000  aresetn = 1'b1;
  #11000 sel     = 1'b1;
  #27000 sel     = 1'b0;
  #43000 sel     = 1'b1;
  #17000 sel     = 1'b0;
  #1000 $stop;
end

endmodule

`endif
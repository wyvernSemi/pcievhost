onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /test/Clk
add wave -noupdate /test/notReset
add wave -noupdate -radix unsigned /test/Count
add wave -noupdate /test/DisableScrambleDown
add wave -noupdate /test/DisableScrambleUp
add wave -noupdate -radix hexadecimal /test/DispDataIn
add wave -noupdate -radix hexadecimal /test/DispDataOut
add wave -noupdate /test/DispVal
add wave -noupdate -radix hexadecimal /test/DownLink
add wave -noupdate /test/InvertTxPolarityDown
add wave -noupdate /test/InvertTxPolarityUp
add wave -noupdate -radix hexadecimal /test/LinkDown0
add wave -noupdate -radix hexadecimal /test/LinkDown1
add wave -noupdate -radix hexadecimal /test/LinkDown2
add wave -noupdate -radix hexadecimal /test/LinkDown3
add wave -noupdate -radix hexadecimal /test/LinkDown4
add wave -noupdate -radix hexadecimal /test/LinkDown5
add wave -noupdate -radix hexadecimal /test/LinkDown6
add wave -noupdate -radix hexadecimal /test/LinkDown7
add wave -noupdate -radix hexadecimal /test/LinkDown8
add wave -noupdate -radix hexadecimal /test/LinkDown9
add wave -noupdate -radix hexadecimal /test/LinkDown10
add wave -noupdate -radix hexadecimal /test/LinkDown11
add wave -noupdate -radix hexadecimal /test/LinkDown12
add wave -noupdate -radix hexadecimal /test/LinkDown13
add wave -noupdate -radix hexadecimal /test/LinkDown14
add wave -noupdate -radix hexadecimal /test/LinkDown15
add wave -noupdate -radix hexadecimal /test/LinkUp0
add wave -noupdate -radix hexadecimal /test/LinkUp1
add wave -noupdate -radix hexadecimal /test/LinkUp2
add wave -noupdate -radix hexadecimal /test/LinkUp3
add wave -noupdate -radix hexadecimal /test/LinkUp4
add wave -noupdate -radix hexadecimal /test/LinkUp5
add wave -noupdate -radix hexadecimal /test/LinkUp6
add wave -noupdate -radix hexadecimal /test/LinkUp7
add wave -noupdate -radix hexadecimal /test/LinkUp8
add wave -noupdate -radix hexadecimal /test/LinkUp9
add wave -noupdate -radix hexadecimal /test/LinkUp10
add wave -noupdate -radix hexadecimal /test/LinkUp11
add wave -noupdate -radix hexadecimal /test/LinkUp12
add wave -noupdate -radix hexadecimal /test/LinkUp13
add wave -noupdate -radix hexadecimal /test/LinkUp14
add wave -noupdate -radix hexadecimal /test/LinkUp15
add wave -noupdate -radix unsigned /test/LinkWidth
add wave -noupdate -radix unsigned /test/NodeNumDown
add wave -noupdate -radix unsigned /test/NodeNumUp
add wave -noupdate /test/UpLink
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {0 ps} 0}
quietly wave cursor active 0
configure wave -namecolwidth 150
configure wave -valuecolwidth 100
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ns
update
WaveRestoreZoom {0 ps} {3687601 ps}

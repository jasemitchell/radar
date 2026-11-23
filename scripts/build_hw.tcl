# Rebuild the Vivado project, bitstream and XSA from source.
#
# Usage, from the repo root:
#   vivado -mode batch -source scripts/build_hw.tcl
#   vivado -mode batch -source scripts/build_hw.tcl -tclargs 8   ;# 8 parallel jobs
#
# Needs Vivado 2026.1 (hw/bd/system.tcl checks the version) and the PYNQ-Z2
# board files. Everything is written to build/vivado/; nothing in hw/ is changed.

set repo  [file normalize [file join [file dirname [info script]] ..]]
set build [file join $repo build vivado]
set jobs  [expr {[llength $argv] > 0 ? [lindex $argv 0] : 4}]

create_project radar $build -part xc7z020clg400-1 -force
set_property board_part tul.com.tw:pynq-z2:part0:1.0 [current_project]

# The block design references ADC_formatter as an RTL module, so it must be
# in the project before system.tcl is sourced.
add_files -norecurse [file join $repo hw rtl ADC_formatter.sv]
update_compile_order -fileset sources_1

source [file join $repo hw bd system.tcl]
if {[llength [get_bd_cells -quiet]] == 0} {
    error "Block design was not created; see the messages from system.tcl above."
}

set bd [get_files radar.bd]
generate_target all $bd
add_files -norecurse [make_wrapper -files $bd -top]
set_property top radar_wrapper [current_fileset]
update_compile_order -fileset sources_1

launch_runs impl_1 -to_step write_bitstream -jobs $jobs
wait_on_run impl_1
if {[get_property PROGRESS [get_runs impl_1]] ne "100%"} {
    error "Implementation failed; see $build/radar.runs/impl_1/runme.log"
}

set xsa [file join $build radar.xsa]
write_hw_platform -fixed -include_bit -force -file $xsa
puts "Done: $xsa"

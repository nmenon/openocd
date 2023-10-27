## Using this test script for quick CI:
#../src/openocd  -f ./test-c.tcl
adapter serial MC110001
gdb port 3335
source [find board/ti_mspm0_launchpad.cfg]

# init before running this script - this allows us to provide this as -f option at command line.
init
mspm0_board_reset
sleep 1000
mspm0_factory_reset
sleep 1000
flash info 0
# Does'nt seem to work on my board for some reason
# mspm0_mass_erase
flash protect 0 0 last off
flash erase_sector 0 0 last
program /home/nmenon/tmp/mspm0_images/out_of_box_LP_MSPM0C1104_nortos_ticlang.out
# Does'nt seem to work on my board for some reason
# mspm0_start_bootloader
sleep 1000
mspm0_board_reset
shutdown

# PIN-FI

This is an updated version of the PINFI provided by DSL lab @ UBC.

## Building
1. Install Intel Pin
2. Copy the tool directory (pin-fi) to <pin root>/source/tools
3. Edit the 'utils.h' to select the fault targets
  - FI_SRC_REG: source registers
  - FI_DST_REG: destination registers
  - FI_DST_MEM: destination memory location
3. Chdir to that and do 'make', the obj-intel64 folder with the
pintools' shared libraries is created

## Usage
1. Check the faultinject.py script. In a nutshell, first one needs
to run the instcount.so pintool to get the dynamic instruction count.
Then, run the faultinjection.so tool


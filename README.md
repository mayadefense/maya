# Maya: Obfuscating Power Side Channels with Formal Control

This software accompanies the ISCA (IEEE/ACM International Symposium on Computer Architecture) 2021 paper "Using Formal Control to Obfuscate Power Side Channels" by Pothukuchi et al. ([Paper](https://iacoma.cs.uiuc.edu/iacoma-papers/isca21_1.pdf)), and is used to obfuscate power side channels on a system. 

## Requirements

This code reads power values through the [Intel RAPL interface](https://www.kernel.org/doc/html/latest/power/powercap/powercap.html), uses dynamic voltage-frequency scaling through the [Linux CPUFreq governors](https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt) and idle cycle injection using the [Intel powerclamp interface](https://www.kernel.org/doc/Documentation/thermal/intel_powerclamp.txt). So, on systems that support these features (which include most x86_linux systems), it should work out of the box (provided these interfaces are enabled - see below). 

For other systems, the appropriate sensors and inputs (also called knobs or actuators) can be updated in the Sensors.h/cpp and Inputs.h/cpp files. It is easy to include such extensions because the code has been developed to support it.

Maya changes the processor's frequency and this requires root privilege (`sudo`).

## Setting up the System

### Allow Maya to change the processor's frequency from software

1. Check that the power driver is *acpi-cpufreq*

    Processor power management is handled by one of two drivers: *acpi-cpufreq* and *intel_pstate*. *intel_pstate* is the default driver these days but it doesn't allow users to change frequency. Check if the system has the directory `/sys/devices/system/cpu/intel_pstate`. If so, you should switch to *acpi-cpufreq* - it is easy (and it is also easy to switch back!). If you disable *intel_pstate*, the system autoamtically uses *acpi-cpufreq*. Follow the next steps if you need to switch to *acpi-cpufreq* or skip them if you already enabled it.

2. To disable the default *intel-pstate* driver, you need to edit `/etc/default/grub`. In this file, add `intel_pstate=disable` for the `GRUB_CMDLINE_LINUX_DEFAULT` option. For example, 
    ```bash
    GRUB_CMDLINE_LINUX_DEFAULT="<other stuff you may or may not have> intel_pstate=disable"
    ```
3. Recompile grub. You can either use `sudo update-grub` or `grub2-mkconfig -o "$(readlink -e /etc/grub2.cfg)"`. 

4. Reboot. That's it!

4. With the *acpi-cpufreq* driver, Maya can change the processor's frequency values using either the *userspace* governor (preferred) or the *performance* governor. To enable the *userspace* governor (or to enable the *performance* governor if *userspace* isn't available), you can use the `SetGovernor.sh` file in the `Scripts` directory. Since you need to change the governor, the code requires sudo access. Simply type `sudo bash SetGovernor.sh` from that directory.

## Maya's controller

Maya modifies the processor's settings using a robust control theory based controller. The controller's files are in the `Controller` directory, and the default controller is named `mayaRobust`. This controller should work well (i.e., it can keep power close to the target given to it) for most systems. If that doesn't happen, there are two solutions:

1. A simple solution is to calibrate the scaling factors. The controller operates on normalized values of power and the inputs. Run Maya in system identification mode with a test application, and record the maximum and minimum values of power and CPU frequency. Use them to adjust the scaling factors in the `Controller/mayaRobust_scaleYMeasDown.txt` and `Controller/mayaRobust_scaleInputsUp.txt` files. 
    * After running a test application and measuring its power, the entry in the `Controller/mayaRobust_scaleYMeasDown.txt` file can be updated to `2/(maxPowerValue - idlePowerValue)`. 
    
    * For the inputs, the `Controller/mayaRobust_scaleInputsUp.txt` file has the scaling values for the three inputs. They are given by `(maxInputValue - minInputValue)/2` for each input. You will only need to change the first value which corresponds to CPU frequency (it is measured in MHz). 

2. If the simple solution doesn't work, you might have to re-design a controller for your system. You can follow the instructions in the ISCA paper and the [technical report](https://iacoma.cs.uiuc.edu/iacoma-papers/isca21_1_tr.pdf) for this.

## Compiling Maya and the Balloon application

There are two configurations (aka `CONF`s) for the software: Debug (with verbose debug information) and Release. Simply type `make CONF=<Debug|Release>` to build the `CONF` of choice. You can also edit the default configuration using the `DEFAULTCONF` variable in the Makefile.

The Maya executable is placed in the Dist/\<CONF\>/ directory. The `make` process also builds an executable for the Balloon application needed for changing the power consumption (please see the ISCA paper above). The Balloon executable is also placed in the same directory.

## Using Maya

1. Begin by launching the Balloon executable:
```bash
./Balloon <number of cores in the system> &
```

2. Launch Maya with the desired options. The general syntax is:
```bash
sudo LD_LIBRARY_PATH=<path to lib64>/:\$LD_LIBRARY_PATH ./Maya --mode <Baseline|Sysid|Mask> [--idips <inputs for system identification>] [--mask <Constant|Uniform|Gauss|Sine|GaussSine|Preset> --ctldir <path to the directory where the files for the robust controller are stored> --ctlfile <the name of the controller which is used as a prefix for all its files>] > <log file> 2>&1 &
```
Note that you need to specify the `LD_LIBRARY_PATH` explicitly because the variable is cleared in sudo mode. The path you specify is the path to the lib64 library for the gcc/g++ compiler you use.

Once Maya is launched, it will print the time, power, and values of the inputs to the standard output. You can also redirect it to a log file.

Examples:
```bash
# Prints the values of power and the inputs - doesn't change power
sudo LD_LIBRARY_PATH=<path to lib64>/:\$LD_LIBRARY_PATH ./Maya --mode Baseline > /dev/null 2>&1 & 

# Perform system identification by changing the inputs named CPUFreq, IdlePct and PBalloon randomly
sudo LD_LIBRARY_PATH=<path to lib64>/:\$LD_LIBRARY_PATH ./Maya --mode Sysid --idips CPUFreq IdlePct PBalloon > /dev/null 2>&1 & 

# Run Maya with the Gaussian Sinusoid mask generator. The robust controller files are in the ../../Controller directory and the files are prefixed with the name mayaRobust
sudo LD_LIBRARY_PATH=<path to lib64>/:\$LD_LIBRARY_PATH ./Maya --mode Mask --mask GaussSine --ctldir ../../Controller --ctlfile mayaRobust > /dev/null 2>&1 & 
```

## Stopping Maya

Maya will continue to run indefinitely once launched. To stop it, press `ctrl C`. Maya has a `sigkill` handler (see Manager.cpp) that will terminate the program gracefully.

## Maya in paired mode

To launch Maya with a specific application, you can use the Launch.sh script in the Scripts directory. You can modify the script to add the application you want to run. Edit this script to specify the command line for your apps as:
```bash
su <userid> -c "cmdline for your app" > $OUTFILE
```

An example command line for the script to record the output is:
```bash
sudo -E --preserve-env=PATH env "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" bash ./Launch.sh --rundir "../Dist/Release/" --options "--mode Baseline" --logdir "<logdir>" --tag "<name>" --apps "<appname>"
```
The Launch.sh script takes care of preparing the system, launching Balloon, Maya and the application you specify. Finally, it wil terminate Balloon and Maya after the application completes execution. The script is also launched with `sudo` permissions. The `--tag` and `--logdir` parameters are optional. If you specify a tag name for the execution and a log directory, the script will record the output of Maya in `$LOGFILE` in the specified directory, and the output of the application will be in `$OUTFILE`. Make sure that the log directory is accessible by the script (note that it is running with root privilege).

You can print a timestamp (`date +%s.%N`) when the applications starts and exits in the `Launch.sh` file so that you can extract the relevant power monitoring data from the log file using these timestamps.

If needed, the script can be killed with `ctrl C`.

## Citing this work

If you have used the software for your research publication, please cite the [ISCA paper](https://iacoma.cs.uiuc.edu/iacoma-papers/isca21_1.pdf).

## Contact

If you have questions about the code, create an issue in the repo, or contact [Raghavendra Pradyumna Pothukuchi](https://www.cs.yale.edu/homes/raghav/).

## License

[UIUC/NCSA](https://choosealicense.com/licenses/ncsa/)
/*
 * ================================================================================
 * Copyright 2021 University of Illinois Board of Trustees. All Rights Reserved.
 * Licensed under the terms of the University of Illinois/NCSA Open Source License 
 * (the "License"). You may not use this file except in compliance with the License. 
 * The License is included in the distribution as License.txt file.
 *
 * Software distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and limitations 
 * under the License. 
 * ================================================================================
 */

/* 
 * File:   Inputs.h
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#ifndef INPUTS_H
#define INPUTS_H

#include "Sensors.h"
#include <vector>
#include <string>


/*
 * Declare all the inputs (also called actuators or knobs) you need here. 
 * The base class for any input is the "Input" class (The Input class is itself 
 * derived from the Sensor class because it also has to read the current input value from the system). 
 * Any new input inherits this class and updates the 
 * writeToSystem() and readFromSystem() functions. The former function is used to apply a given value to
 * the appropriate system counters/files. The latter is described in Sensors.h.
 * A new input class must also define the allowed values for that input, and the 
 * min, max and mid values of this range.
 * 
 * There are three inputs defined here: CPUFreq, Power. A few other sensors: CPU Temperature,  
 * Performance (Throughput, in Billions of Instructions Per Second (BIPS))are 
 * commented but can be enabled for other purposes if desired.
 */
//Children must fill allowedValues, and define the apply, read functions

class Input : public Sensor {
public:

    Input(std::string iname);

    std::shared_ptr<InputPort> in;

    void updateValueToSystem();

    void setRandomValue(); //set the input to a random value among the allowed values
    void setMaxValue(); //set the input to its maximum value
    void setMinValue(); //set the input to its minimum values
    void setMidValue(); //set the input to its mid value

    virtual void reset();
    Vector measureWriteLatency();

protected:
    double sanitizeValue(double);

    void updateMinMaxMid();
    virtual void writeToSystem();
    void prepareValueToBeWritten(Vector);

    std::vector<double> allowedValues; //populate in constructor
    double minVal, maxVal, midVal; //populate in constructor
    double requestedWriteValue, actualWriteValue; //these may be 
};

/* Reading frequency is easy - read the scaling_cur_freq file in the 
 * cpufreq directory. There are two ways to write frequency. 
 * One is to use the Linux's userspace power governor and write to the 
 * scaling_setspeed file (scaling file) for all the cores. Another is to use Linux's performance 
 * power governor and make the min and max frequencies the value we want to set. 
 * Then, the governor will enforce that frequency. If the userpsace governor is 
 * available, we will use it. Otherwise, we use the latter approach.
 */

class CPUFrequency : public Input {
public:
    CPUFrequency(std::string name, std::vector<uint32_t> coreList);
    void reset() override;

protected:
    void writeToSystem() override;
    void readFromSystem() override;

private:
    std::vector<uint32_t> coreIds;
    std::string freqFileNamePrefix = "/sys/devices/system/cpu/cpu",
            freqRFileNamePostfix = "/cpufreq/scaling_cur_freq";
    std::vector<std::string> freqRFileName;
    std::string freqWFileNamePostfix1 = "/cpufreq/scaling_setspeed",
            freqWFileNamePostfix2Min = "/cpufreq/scaling_min_freq";
    std::vector<std::string> freqWFileName, freqWMinFileName, freqWMaxFileName;

    bool writeScalingFile;
};

/* Use the Intel Powerclamp interface.
 * See https://www.kernel.org/doc/Documentation/thermal/intel_powerclamp.txt
 */
class IdleInject : public Input {
public:
    IdleInject(std::string name);
protected:
    void writeToSystem() override;
    void readFromSystem() override;
    void reset() override;
    std::string dirName = "/sys/class/thermal", devicetypePostfix = "/type",
            pclampSetFileName, pclampMaxFileName, pclampSetFileNamePostfix = "/cur_state",
            pclampMaxFileNamePostfix = "/max_state";
};

/*The power balloon is an application we create. See README. 
 * The value of the balloon is set through /dev/shm/powerBalloon.txt and the maximum 
 * level of the balloon is present in /dev/shm/powerBalloonMax.txt
 */

class PowerBalloon : public Input {
public:
    PowerBalloon(std::string name);
protected:
    void writeToSystem() override;
    void readFromSystem() override;
    void reset() override;
    std::string pbFileName = "/dev/shm/powerBalloon.txt", pbMaxFileName = "/dev/shm/powerBalloonMax.txt";
};

/*
class NumCores : public Input {
public:
    NumCores(std::string name);
protected:
    void writeToSystem() override;
    void readFromSystem() override;

private:
    uint32_t numCores;
    std::string onlineCoreFileName = "/sys/devices/system/cpu/online";
    std::string presentCoreFileName = "/sys/devices/system/cpu/present";
    std::string coreStatusFileNamePrefix = "/sys/devices/system/cpu/cpu";
    std::string coreStatusFileNamePostfix = "/online";
};

class DummySrc {
public:
    void genValues();
    std::shared_ptr<OutputPort> op;
    DummySrc(std::string name);
    DummySrc(std::string name, std::initializer_list<std::string> portNames);
};
*/
#endif /* INPUTS_H */

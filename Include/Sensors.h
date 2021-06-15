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
 * File:   Sensors.h
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

/*
 * Declare all the sensors you need here. The base class for any sensor is the 
 * "Sensor" class. Any new sensor inherits this class and updates the 
 * readFromSystem() function. This function is used to read the sensor value from 
 * the appropriate system counters/files.
 * 
 * There are two sensors defined here: Time, Power. A few other sensors: CPU Temperature,  
 * Performance (Throughput, in Billions of Instructions Per Second (BIPS))are 
 * commented but can be enabled for other purposes if desired.
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "Abstractions.h"
#include "MathSupport.h"
#include <string>
#include <chrono>
#include <memory>
#include <vector>
#include <linux/perf_event.h>
#include <time.h>

class Sensor {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using NanoSec = std::chrono::nanoseconds;
    using MicroSec = std::chrono::microseconds;
    using MilliSec = std::chrono::milliseconds;
    using Sec = std::chrono::seconds;

    Sensor(std::string sname, std::initializer_list<std::string> pnames);
    Sensor(std::string sname);
    virtual void updateValuesFromSystem();
    std::string getName();

    std::shared_ptr<OutputPort> out;
    Vector measureReadLatency(); //measure the delay of reading values from system

protected:
    virtual void readFromSystem();
    std::string name;
    Vector values, prevValues; //current values and previous values of sensors
    uint32_t width; //number of values, default is 1
    TimePoint sampleTime, prevSampleTime;
};

class Time : public Sensor {
public:
    Time(std::string name);
protected:
    void readFromSystem() override;
private:
    struct timespec rawTime;

};

class CPUPowerSensor : public Sensor {
public:
    CPUPowerSensor(std::string name);
protected:
    void readFromSystem() override;
private:
    std::string coreEnergyDirName = "/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/",
            pkgEnergyDirName1 = "/sys/class/powercap/intel-rapl/intel-rapl:0/",
            pkgEnergyDirName2 = "/sys/class/powercap/intel-rapl/intel-rapl:1/",
            energyFilePrefix = "energy_uj";
    std::vector<std::string> energyFileNames;
    double energyCtr;
};

/*

//Hottest core temperature - one value
class CPUTempSensor : public Sensor {
public:
    CPUTempSensor(std::string name);
protected:
    void readFromSystem() override;
private:
    std::vector<std::string> coretempDirNames = {"/sys/devices/platform/coretemp.0/hwmon/hwmon0/",
    "/sys/devices/platform/coretemp.0/hwmon/hwmon1/",
    "/sys/devices/platform/coretemp.1/hwmon/hwmon1/"};
    std::vector<std::string> tempFileNames;
    Vector coreTemps; //individual core temperatures
};

class DRAMPowerSensor : public Sensor {
public:
    DRAMPowerSensor(std::string name);
protected:
    void readFromSystem() override;
private:
    std::string energyFileName = "/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:1/energy_uj";
    double energyCtr;
};
 
//For help with Linux Perf Counters:
//http://web.eece.maine.edu/~vweaver/projects/perf_events/perf_event_open.html
class PerfStatCounters {
public:
    PerfStatCounters(uint32_t coreId, std::initializer_list<perf_type_id> typeIds, std::initializer_list<perf_hw_id> ctrNames);
    void createCounterFds(uint32_t coreId, std::initializer_list<perf_type_id> typeIds, std::initializer_list<perf_hw_id> ctrNames);
    // void createCounterFds(uint32_t coreId, std::initializer_list<perf_type_id> typeIds, 
    //std::initializer_list<perf_hw_cache_id> cacheIds, std::initializer_list<perf_hw_cache_op_id> operationTypes, 
    //std::initializer_list<perf_hw_cache_op_result_id> ctrNames);
    void enable();
    void reenable();
    void disable();
    void updateCounters();
    Vector getValues();
    Vector getDeltaValues();
    double getValue(uint32_t ctrNum);
private:
    std::vector<int> fds;
    std::vector<uint64_t> values, prevValues;
};

class CorePerfSensor : public Sensor {
public:
    CorePerfSensor(std::string name, uint32_t coreId);
    virtual ~CorePerfSensor();

protected:
    void readFromSystem() override;

private:
    void handleReactivation();
    void handleShutDown();
    uint32_t coreId;
    std::unique_ptr<PerfStatCounters> instCtr, cacheCtr;
    bool shutDown;
    double coreBips, coreMpki;
};

class CPUPerfSensor : public Sensor {
public:
    CPUPerfSensor(std::string name, std::vector<uint32_t> coreIds);
    virtual ~CPUPerfSensor();

protected:
    void readFromSystem() override;

private:
    void handleReactivation(uint32_t coreId);
    void handleShutDown(uint32_t coreId);
    std::vector<uint32_t> coreIds;
    std::vector<std::unique_ptr<PerfStatCounters>> instCtr;
    std::vector<bool> shutDown; //keep track of which cores are shutdown, because
    //the counters must be re-enabled upon re-activation
    Vector coreBips;
};

class Dummy {
public:
    Vector readInputs();
    std::shared_ptr<InputPort> inp;
    Dummy(std::string name);
    Dummy(std::string name, std::initializer_list<std::string> portNames);
};

*/
#endif /* SENSORS_H */

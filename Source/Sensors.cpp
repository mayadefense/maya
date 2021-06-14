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
 * File:   Sensors.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */


#include "Sensors.h"
#include "debug.h"
#include "SystemStatus.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <cmath>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <dirent.h>
#include <vector>

/*coreStatus is used to track the on/off status of cores - necessary for performance 
 *monitoring
 */
extern SystemStatus coreStatus;

Sensor::Sensor(std::string sname) :
name(sname),
width(1),
out(std::make_shared<OutputPort>(sname, std::initializer_list<std::string>({sname}))) {
    values = Vector(width);
    prevValues = values;
#ifdef DEBUG
    std::cout << "sensor width (def) " << width << " and " << values.size() << std::endl;
#endif
    prevSampleTime = sampleTime = Clock::now();
}

Sensor::Sensor(std::string sname, std::initializer_list<std::string> pNames) :
name(sname),
width(pNames.size()),
out(std::make_shared<OutputPort>(sname, pNames)) {
    values = Vector(width);
    prevValues = values;
#ifdef DEBUG
    std::cout << "sensor width " << width << " and " << values.size() << std::endl;
#endif
    prevSampleTime = sampleTime = Clock::now();
}

std::string Sensor::getName() {
    return name;
}

void Sensor::updateValuesFromSystem() {
    prevValues = values;
    readFromSystem();
    out->updateValuesToPort(values);
}

void Sensor::readFromSystem() {

}

Vector Sensor::measureReadLatency() {
    Vector latencyResults(1);
    auto init = Clock::now();
    updateValuesFromSystem();
    auto end = Clock::now();
    latencyResults[0] = std::chrono::duration_cast<MicroSec>(end - init).count();
#ifdef DEBUG
    std::cout << " Read Latency for " << name << " " << latencyResults[0] << " us" << std::endl;
#endif
    return latencyResults;
}

Time::Time(std::string name) : Sensor(name) {
    readFromSystem();
}

void Time::readFromSystem() {
    clock_gettime(CLOCK_REALTIME, &rawTime);
    values[0] = (double) rawTime.tv_sec + ((double) rawTime.tv_nsec)*1e-9;
#ifdef DEBUG
    std::cout << "Time: " << rawTime.tv_sec << "." << rawTime.tv_nsec << " " << std::fixed << values[0] << std::endl;
    ;
#endif
}

CPUPowerSensor::CPUPowerSensor(std::string name) : Sensor(name),
energyCtr(0) {
    values[0] = 0.0;
    std::string fileName;
    std::string raplName;
    std::ifstream raplFile;

    raplFile.open(coreEnergyDirName + "name");
    raplFile >> raplName;
    raplFile.close();

    if (raplName.find("core") != std::string::npos) {
        //we have rapl for all cores
        energyFileNames.push_back(coreEnergyDirName + energyFilePrefix);
#ifdef DEBUG
       	std::cout << "Pushing " << coreEnergyDirName+energyFilePrefix << std::endl;
#endif
    } else {
        //we have rapl for each of the two packages
        energyFileNames.push_back(pkgEnergyDirName1 + energyFilePrefix);
        energyFileNames.push_back(pkgEnergyDirName2 + energyFilePrefix);
#ifdef DEBUG
        std::cout << "Pushing " << pkgEnergyDirName1+energyFilePrefix << std::endl;
        	std::cout << "Pushing " << pkgEnergyDirName2+energyFilePrefix << std::endl;
#endif
    }
}

void CPUPowerSensor::readFromSystem() {
    double ctrValue = 0.0, tmp = 0.0;
    std::ifstream powerFile;

    for (auto& energyFileName : energyFileNames) {
        powerFile.open(energyFileName);
        powerFile >> tmp;
        powerFile.close();
        ctrValue = ctrValue + tmp;
    }
    double newEnergy = ctrValue - energyCtr;
    energyCtr = ctrValue;

    sampleTime = Clock::now();
    auto deltaTime = std::chrono::duration_cast<MicroSec>(sampleTime - prevSampleTime).count();
    prevSampleTime = sampleTime;
    if (deltaTime > 0) {
        values[0] = newEnergy / (double) deltaTime;
    } else {
        values[0] = 0.0;
    }
#ifdef DEBUG
    std::cout << "deltaEnergy is " << newEnergy << " elapsed time is " <<
            deltaTime << " power is " << values[0] << std::endl;
#endif
}
/*

CPUTempSensor::CPUTempSensor(std::string name) :
Sensor(name) {
    DIR * coretempDir;
    struct dirent * coretempDirEntry;
    uint32_t dirOpenFailure = 0;
    //We must identify all the sensor files to read temperature from, among the 
    //possible directories where such files might be stored. 
    //To do this, we first go through each candidate directory name
    for (auto& coretempDirName : coretempDirNames) {
        //Open the corresponding directory that has this name
        if ((coretempDir = opendir(coretempDirName.c_str())) != NULL) {
            //Go through each file in this directory
            while ((coretempDirEntry = readdir(coretempDir)) != NULL) {
                //Get the name of the file
                std::string tempfileName(coretempDirEntry->d_name);
                //If the file is of the type "temp*_input" in it (i.e., it has the 
                //word "input" in it), add to the list of sensor files we have to 
                //read from. We won't add temp1_input because it is the average of 
                //all other temp*_input files.
                if (tempfileName.find("input") != std::string::npos && tempfileName.compare("temp1_input") != 0) {
                    tempFileNames.push_back(coretempDirName + tempfileName);
#ifdef DEBUG
                    std::cout << "found " << coretempDirName + tempfileName << std::endl;
#endif
                }
                closedir(coretempDir);
            }
        } else {
            //Directory doesn't exist!
            dirOpenFailure += 1;
        }
    }
    if (dirOpenFailure == coretempDirNames.size()) {
        std::cout << "Cannot open any of directories listed! " << std::endl;
        std::exit(EXIT_FAILURE);
    }

    coreTemps = Vector(tempFileNames.size());
}

void CPUTempSensor::readFromSystem() {
    double newValue;
    values[0] = 0.0;
    std::ifstream tempFile;
    auto i = 0;
    for (auto& tempFileName : tempFileNames) {
#ifdef DEBUG
        std::cout << "Reading from " << tempFileName << ": " << newValue << std::endl;
#endif
        tempFile.open(tempFileName);
        tempFile >> newValue; //in mC
        tempFile.close();
        if (newValue > values[0]) {
            values[0] = newValue;
        }
        coreTemps[i] = newValue;
        i++;
    }
    values[0] = values[0] / 1000.0; //hotspot temperature: the output of this sensor
    coreTemps = coreTemps / 1000.0; //all core temperatures
#ifdef DEBUG
    std::cout << "Core temperatures are: " << coreTemps;
#endif
}

DRAMPowerSensor::DRAMPowerSensor(std::string name) : Sensor(name), energyCtr(0) {
    values[0] = 0.0;
}

void DRAMPowerSensor::readFromSystem() {
    double ctrValue;
    std::ifstream powerFile(energyFileName);
    powerFile >> ctrValue;
    double newEnergy = ctrValue - energyCtr;
    energyCtr = ctrValue;

    sampleTime = Clock::now();
    auto deltaTime = std::chrono::duration_cast<MicroSec>(sampleTime - prevSampleTime).count();
    prevSampleTime = sampleTime;
    if (deltaTime > 0) {
        values[0] = newEnergy / (double) deltaTime;
    } else {
        values[0] = 0.0;
    }
#ifdef DEBUG
    std::cout << "deltaEnergy is " << newEnergy << " elapsed time is " <<
            deltaTime << " DRAM power is " << values[0] << std::endl;
#endif
}

PerfStatCounters::PerfStatCounters(uint32_t coreId, std::initializer_list<perf_type_id> typeIds, std::initializer_list<perf_hw_id> ctrNames) {
    if (typeIds.size() != ctrNames.size()) {
        std::cout << " Number of counter types and names don't match" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int numCounters = ctrNames.size();
    for (int i = 0; i < numCounters; i++) {
        values.push_back(0);
        fds.push_back(-1);
    }
    createCounterFds(coreId, typeIds, ctrNames);

    prevValues = values;
}

void PerfStatCounters::createCounterFds(uint32_t coreId, std::initializer_list<perf_type_id> typeIds, std::initializer_list<perf_hw_id> ctrNames) {

    if (typeIds.size() != ctrNames.size()) {
        std::cout << " Number of counter types and names don't match" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::vector<perf_type_id> typeIdList(typeIds);
    std::vector<perf_hw_id> ctrNameList(ctrNames);

    int numCounters = ctrNames.size();

    struct perf_event_attr eventAttr;
    memset(&eventAttr, 0, sizeof (struct perf_event_attr));
    eventAttr.size = sizeof (struct perf_event_attr);

    for (int i = 0; i < numCounters; i++) {
        eventAttr.type = typeIdList[i];
        eventAttr.config = ctrNameList[i];
        if (i == 0) {
            //the first counter in a group is disabled by default
            eventAttr.disabled = 1;
            fds[i] = syscall(__NR_perf_event_open, &eventAttr, -1, coreId, -1, 0);
        } else {
            //specify fds[0] as the leader for the remaining counters
            eventAttr.disabled = 0;
            fds[i] = syscall(__NR_perf_event_open, &eventAttr, -1, coreId, fds[0], 0);
        }
        if (numCounters == 1) {
            eventAttr.disabled = 0;
        }

#ifdef DEBUG
        std::cout << "File descriptor is " << fds[i] << std::endl;
#endif
        if (fds[i] == -1) {
            std::cout << "Cannot create perf counter collection for core " << coreId << std::endl;
            std::cout <<"Note that all cores must be on when the counters are first setup" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}

void PerfStatCounters::enable() {
    for (auto& fd : fds) {
        ioctl(fd, PERF_EVENT_IOC_RESET);
        ioctl(fd, PERF_EVENT_IOC_ENABLE);
    }
}

void PerfStatCounters::reenable() {
    for (auto& fd : fds) {
        ioctl(fd, PERF_EVENT_IOC_ENABLE);
    }
}

void PerfStatCounters::disable() {
    auto i = 0;
    for (auto& fd : fds) {
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        close(fd);
#ifdef DEBUG
        std::cout << "Closing fds " << fd << std::endl;
#endif
        fd = -1;
        values[i] = 0;
        prevValues[i] = 0;
        i++;
    }
}

void PerfStatCounters::updateCounters() {
    prevValues = values;
    uint64_t value;
    int numBytesRead = -1, i = 0;
    for (auto& fd : fds) {
        numBytesRead = read(fd, &value, sizeof (value));
        if (numBytesRead != sizeof (value)) {
            std::cout << "Cannot read perf counter for core " << std::endl;
            std::cout <<"Note that a cores must be on when the counters are read" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        values[i] = value;
        i++;
    }
}

Vector PerfStatCounters::getValues() {
    Vector result(values);
    return result;
}

Vector PerfStatCounters::getDeltaValues() {
    Vector curr(values), prev(prevValues);
    return curr - prev;
}

double PerfStatCounters::getValue(uint32_t ctrNum) {
    return (double) values[ctrNum];
}

CPUPerfSensor::CPUPerfSensor(std::string name, std::vector<uint32_t> coreIds_) :
coreIds(coreIds_),
Sensor(name,{name + "_BIPS"}) {

    coreBips = Vector(coreIds.size());
    sampleTime = Clock::now();
    prevSampleTime = sampleTime;

    for (auto& coreId : coreIds) {
        instCtr.push_back(std::make_unique<PerfStatCounters>(coreId,
                std::initializer_list<perf_type_id> ({PERF_TYPE_HARDWARE}),
        std::initializer_list<perf_hw_id>({PERF_COUNT_HW_INSTRUCTIONS})));
        shutDown.push_back(false);
    }

    for (auto& ctr : instCtr) {
        ctr->enable();
    }

    readFromSystem();
#ifdef DEBUG
    std::cout << "First performance value is " << values << std::endl;
#endif
}

CPUPerfSensor::~CPUPerfSensor() {
    for (auto& ctr : instCtr) {
        ctr->disable();
    }
}

void CPUPerfSensor::handleShutDown(uint32_t coreId) {
#ifdef DEBUG
    std::cout << "Shutting down counters on core " << coreId << std::endl;
#endif
    instCtr[coreId]->disable();
    shutDown[coreId] = true;
}

void CPUPerfSensor::handleReactivation(uint32_t coreId) {
#ifdef DEBUG
    std::cout << "Reactivating counters on core restart " << coreId << std::endl;
#endif

    instCtr[coreId]->createCounterFds(coreId,{PERF_TYPE_HARDWARE},
    {
        PERF_COUNT_HW_INSTRUCTIONS
    });

    sampleTime = Clock::now();
    prevSampleTime = sampleTime;
    instCtr[coreId]->reenable();
    shutDown[coreId] = false;
}

void CPUPerfSensor::readFromSystem() {
    size_t numBytesRead;
    uint64_t ctrValue;
    sampleTime = Clock::now();
    auto deltaTime = (double) std::chrono::duration_cast<NanoSec>(sampleTime - prevSampleTime).count();
    prevSampleTime = sampleTime;

    Vector totalNewInst(1);
    Vector perCoreNewInstVals;

    for (auto& coreId : coreIds) {
        if (coreStatus.getUnitStatus(coreId) == false && shutDown[coreId] == false) {
            //core was just shutdown
            handleShutDown(coreId);
            continue;
        } else if (coreStatus.getUnitStatus(coreId) == false && shutDown[coreId] == true) {
            //core is off
            continue;
        } else if (coreStatus.getUnitStatus(coreId) == true && shutDown[coreId] == true) {
            //core was just turned on
            handleReactivation(coreId);
        }
        instCtr[coreId]->updateCounters();

        perCoreNewInstVals = instCtr[coreId]->getDeltaValues();

        totalNewInst = totalNewInst + perCoreNewInstVals;

        coreBips[coreId] = perCoreNewInstVals[0] / deltaTime;

#ifdef DEBUG
        std::cout << "------Core " << coreId << std::endl;
        std::cout << "Instructions " << perCoreNewInstVals[0] << " Time " << deltaTime <<
                " BIPS is " << coreBips[coreId] << std::endl;
#endif      
    }
    values[0] = totalNewInst[0] / deltaTime;

#ifdef DEBUG
    std::cout << "Instructions " << totalNewInst[0] << " Time " << deltaTime <<
            " BIPS is " << values[0] << std::endl;
#endif
}


Dummy::Dummy(std::string name) :
inp(std::make_shared<InputPort>(name)) {

}

Dummy::Dummy(std::string name, std::initializer_list<std::string> portNames) :
inp(std::make_shared<InputPort>(name, portNames)) {

}

Vector Dummy::readInputs() {
    return inp->updateValuesFromPort();
}

CorePerfSensor::CorePerfSensor(std::string name, uint32_t coreId_) :
coreId(coreId_),
Sensor(name,{name + std::to_string(coreId_) + "_BIPS", name + std::to_string(coreId_) + "_MPKI"}) {

    sampleTime = Clock::now();
    prevSampleTime = sampleTime;


    instCtr = std::move(std::make_unique<PerfStatCounters>(coreId,
            std::initializer_list<perf_type_id> ({PERF_TYPE_HARDWARE}),
    std::initializer_list<perf_hw_id>({PERF_COUNT_HW_INSTRUCTIONS})));
    cacheCtr = std::move(std::make_unique<PerfStatCounters>(coreId, std::initializer_list<perf_type_id> ({PERF_TYPE_HARDWARE, PERF_TYPE_HARDWARE}),
    std::initializer_list<perf_hw_id>({PERF_COUNT_HW_CACHE_REFERENCES, PERF_COUNT_HW_CACHE_MISSES})));
    shutDown = false;

    instCtr->enable();
    cacheCtr->enable();

    readFromSystem();
#ifdef DEBUG
    std::cout << "First performance value of " << name << values << std::endl;
#endif
}

CorePerfSensor::~CorePerfSensor() {
    instCtr->disable();
    cacheCtr->disable();
}

void CorePerfSensor::handleShutDown() {
#ifdef DEBUG
    std::cout << "Shutting down counters on core " << coreId << std::endl;
#endif
    instCtr->disable();
    cacheCtr->disable();
    shutDown = true;
}

void CorePerfSensor::handleReactivation() {
#ifdef DEBUG
    std::cout << "Reactivating counters on core restart " << coreId << std::endl;
#endif

    instCtr->createCounterFds(coreId,{PERF_TYPE_HARDWARE},
    {
        PERF_COUNT_HW_INSTRUCTIONS
    });
    cacheCtr->createCounterFds(coreId,{PERF_TYPE_HARDWARE, PERF_TYPE_HARDWARE},
    {
        PERF_COUNT_HW_CACHE_REFERENCES, PERF_COUNT_HW_CACHE_MISSES
    });

    sampleTime = Clock::now();
    prevSampleTime = sampleTime;
    instCtr->reenable();
    cacheCtr->reenable();
    shutDown = false;
}

void CorePerfSensor::readFromSystem() {
    size_t numBytesRead;
    uint64_t ctrValue;
    sampleTime = Clock::now();
    auto deltaTime = (double) std::chrono::duration_cast<NanoSec>(sampleTime - prevSampleTime).count();
    prevSampleTime = sampleTime;

    Vector perCoreNewInstVals, perCoreNewCacheCtrVals;
    coreBips = 0.0;
    coreMpki = 0.0;
    if (coreStatus.getUnitStatus(coreId) == false && shutDown == false) {
        handleShutDown();
    } else if (coreStatus.getUnitStatus(coreId) == false && shutDown == true) {
    } else if (coreStatus.getUnitStatus(coreId) == true && shutDown == true) {
        handleReactivation();
    } else {
        instCtr->updateCounters();
        cacheCtr->updateCounters();

        perCoreNewInstVals = instCtr->getDeltaValues();
        perCoreNewCacheCtrVals = cacheCtr->getDeltaValues();

        //totalNewInst = totalNewInst + perCoreNewInstVals;
        //totalNewCacheCtrVals = totalNewCacheCtrVals + (cacheCtr[coreId]->getDeltaValues());

        coreBips = perCoreNewInstVals[0] / deltaTime;
        coreMpki = perCoreNewCacheCtrVals[1]*1000.0 / perCoreNewInstVals[0];
    }
    values[0] = coreBips;
    values[1] = coreMpki;
#ifdef DEBUG
    std::cout << "------Core " << coreId << std::endl;
    std::cout << "Instructions " << perCoreNewInstVals[0] << " Time " << deltaTime <<
            " BIPS is " << values[0] << std::endl;
    std::cout << "Accesses: " << perCoreNewCacheCtrVals[0] << "Misses: " << perCoreNewCacheCtrVals[1] <<
            " MPKI " << values[1] << std::endl;
#endif      
}
*/

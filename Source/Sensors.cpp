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

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
 * File:   Inputs.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#include "Inputs.h"
#include "debug.h"
#include "Sensors.h"
#include "SystemStatus.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <dirent.h>
#include <time.h> 


Input::Input(std::string iname) : Sensor(iname),
in(std::make_shared<InputPort>(iname, std::initializer_list<std::string>({iname}))),
requestedWriteValue(0.0),
actualWriteValue(0.0) {

}

double Input::sanitizeValue(double val) {
    if (allowedValues.size() == 0) {
#ifdef DEBUG
        std::cout << "No range of allowed values " << std::endl;
#endif
        return val;
    }
    return *std::min_element(allowedValues.begin(),
            allowedValues.end(),
            [&val](double x, double y) {
                return std::abs(x - val) < std::abs(y - val);
            });
}

void Input::prepareValueToBeWritten(Vector newValues) {
    requestedWriteValue = newValues[0];
    actualWriteValue = sanitizeValue(requestedWriteValue);
}

void Input::updateValueToSystem() {
    if (in->areValuesUnread() == false) {
#ifdef DEBUG
        std::cout << "Didn't receive any new values for " << name << std::endl;
#endif
        return;
    }

    prepareValueToBeWritten(in->updateValuesFromPort());

#ifdef DEBUG
    std::cout << " Asked to write " << requestedWriteValue << " writing " << actualWriteValue <<
            " for " << name << std::endl;
#endif

    writeToSystem();
}

void Input::writeToSystem() {

}

Vector Input::measureWriteLatency() {
    Vector latencyResults(2);
    prepareValueToBeWritten(Vector({maxVal}));
    writeToSystem();
    updateValuesFromSystem();

    prepareValueToBeWritten(Vector({minVal}));
    auto begin = Clock::now();
    writeToSystem();
    auto end = Clock::now();
    latencyResults[0] = std::chrono::duration_cast<MicroSec>(end - begin).count();
    updateValuesFromSystem();

    prepareValueToBeWritten(Vector({maxVal}));
    begin = Clock::now();
    writeToSystem();
    end = Clock::now();
    latencyResults[1] = std::chrono::duration_cast<MicroSec>(end - begin).count();
    updateValuesFromSystem();
#ifdef DEBUG
    std::cout << " Write Latency (max-min) for " << name << " " << latencyResults[0] << " us" << std::endl;
    std::cout << " Write Latency (min-max) for " << name << " " << latencyResults[1] << " us" << std::endl;
#endif
    return latencyResults;
}

void Input::updateMinMaxMid() {
    if (allowedValues.size() == 0) {
        std::cout << "No range of allowed values " << std::endl;
        std::exit(EXIT_FAILURE);
    }

    minVal = *std::min_element(allowedValues.begin(), allowedValues.end());
    maxVal = *std::max_element(allowedValues.begin(), allowedValues.end());
    midVal = (minVal + maxVal) / 2.0;

#ifdef DEBUG
    std::cout << " Min val is " << minVal << " Max val is " << maxVal << " Mid val is " << midVal << std::endl;
#endif
}

void Input::setRandomValue() {
#ifdef DEBUG
    std::cout << "Setting random value for " << name << std::endl;
#endif
    auto id = rand() % (allowedValues.size());
    in->receiveValues(Vector({allowedValues[id]}));
}

void Input::setMaxValue() {
#ifdef DEBUG
    std::cout << "Setting max value for " << name << std::endl;
#endif
    in->receiveValues(Vector({maxVal}));
}

void Input::setMinValue() {
#ifdef DEBUG
    std::cout << "Setting min value for " << name << std::endl;
#endif
    in->receiveValues(Vector({minVal}));
}

void Input::setMidValue() {
#ifdef DEBUG
    std::cout << "Setting mid value for " << name << std::endl;
#endif
    in->receiveValues(Vector({midVal}));
}

void Input::reset() {
#ifdef DEBUG
    std::cout << "Reset called for " << name << std::endl;
#endif
    setMaxValue();
}

CPUFrequency::CPUFrequency(std::string name) :
Input(name) {
    //Find number of cores
    std::string coreStatusString;
    std::ifstream coreFile(presentCPUCoreFileName);
    coreFile >> coreStatusString;
    auto delimPos = coreStatusString.find("-");
    uint32_t startCore = -1, endCore = -1, numCores = 1;
    if (delimPos != std::string::npos) {
        startCore = std::stoul(coreStatusString.substr(0, delimPos));
        endCore = std::stoul(coreStatusString.substr(delimPos + 1));
        numCores = endCore - startCore + 1;
    }
    for (auto i = 0; i < numCores; i++){
        coreIds.push_back(i);
    }

    for (auto& coreId : coreIds) {
        auto fileName = freqFileNamePrefix;

        //populate freqRFileName
        fileName = fileName.append(std::to_string(coreId)).append(freqRFileNamePostfix);
        freqRFileName.push_back(fileName);
#ifdef DEBUG
        std::cout << "Creating frequency files for " << coreId << " " << fileName << std::endl;
#endif
        //populate freqWFileName
        fileName = freqFileNamePrefix;
        fileName = fileName.append(std::to_string(coreId)).append(freqWFileNamePostfix1);
        freqWFileName.push_back(fileName);

        //populate freqWFileNameMin
        fileName = freqFileNamePrefix;
        fileName = fileName.append(std::to_string(coreId)).append(freqWFileNamePostfix2Min);
        freqWMinFileName.push_back(fileName);

        //populate freqWFileNameMax
        fileName.replace(fileName.find("min"), 3, "max");
        freqWMaxFileName.push_back(fileName);

    }

    //find min frequency
    minVal = 0, maxVal = 0;
    auto tmpFreqFileName = freqRFileName[0];
    tmpFreqFileName.replace(tmpFreqFileName.find("scaling_cur"), 11, "cpuinfo_min");
    std::ifstream freqFile(tmpFreqFileName);
#ifdef DEBUG
    std::cout << tmpFreqFileName << std::endl;
#endif
    freqFile >> minVal;
    freqFile.close();

    //find max frequency
    tmpFreqFileName.replace(tmpFreqFileName.find("min"), 3, "max");
    freqFile.open(tmpFreqFileName);
#ifdef DEBUG
    std::cout << tmpFreqFileName << std::endl;
#endif
    freqFile >> maxVal;
    freqFile.close();
#ifdef DEBUG
    std::cout << "minVal : " << minVal << " maxVal : " << maxVal << std::endl;
#endif

    //find allowedValues
    tmpFreqFileName = freqFileNamePrefix;
    tmpFreqFileName = tmpFreqFileName.append(std::to_string(coreIds[0])).append("/cpufreq/scaling_available_frequencies");
#ifdef DEBUG
    std::cout << tmpFreqFileName << std::endl;
#endif

    freqFile.open(tmpFreqFileName);
    if (freqFile) {
        double val;
        while (freqFile >> val) {
            allowedValues.push_back(val);
        }
        freqFile.close();
    } else {
        for (double val = minVal; val <= maxVal + 1; val += 200000) {
            allowedValues.push_back(val);
        }
    }
#ifdef DEBUG
    std::cout << "Frequency values are: ";
    for (auto&& val : allowedValues) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
#endif
    updateMinMaxMid();

    //Determine which write method to use
    tmpFreqFileName = freqFileNamePrefix;
    tmpFreqFileName = tmpFreqFileName.append(std::to_string(coreIds[0])).append("/cpufreq/scaling_governor");
    freqFile.open(tmpFreqFileName);
    std::string governorName;
    freqFile >> governorName;
    freqFile.close();
    if (governorName.compare("userspace") == 0) {
        writeScalingFile = true;
    } else {
        writeScalingFile = false;
    }

#ifdef DEBUG
    std::cout << "Write method is " << writeScalingFile?"userspace governor":"performance governor" << std::endl;
#endif

    updateValuesFromSystem();
}

void CPUFrequency::reset() {
#ifdef DEBUG
    std::cout << "Resetting " << name << std::endl;
#endif

    if (!writeScalingFile) {
        std::ofstream freqFile;

        uint64_t newValue = (uint64_t) maxVal;

        for (auto& fileName : freqWMaxFileName) {
#ifdef DEBUG
            std::cout << "Writing " << newValue << " to " << fileName << std::endl;
#endif
            freqFile.open(fileName);
            freqFile << newValue;
            freqFile.close();
        }

        newValue = (uint64_t) minVal;
        for (auto& fileName : freqWMinFileName) {
#ifdef DEBUG
            std::cout << "Writing " << newValue << " to " << fileName << std::endl;
#endif
            freqFile.open(fileName);
            freqFile << newValue;
            freqFile.close();
        }
    }
}

void CPUFrequency::writeToSystem() {
#ifdef DEBUG
    std::cout << "Writing to " << name << " with value " << actualWriteValue << " and current values is " <<
            values << std::endl;
#endif
    readFromSystem();
    double value = values[0];
    uint64_t newValue = (uint64_t) actualWriteValue;
    if (newValue == value) {
        return;
    }

    std::ofstream freqFile;
    if (writeScalingFile) {
        for (auto& fileName : freqWFileName) {
#ifdef DEBUG
            std::cout << "Writing " << newValue << " to " << fileName << std::endl;
#endif
            freqFile.open(fileName);
            freqFile << newValue;
            freqFile.close();
        }
    } else {
        if (newValue > value) {

            for (auto& fileName : freqWMaxFileName) {
#ifdef DEBUG
                std::cout << "Writing " << newValue << " to " << fileName << std::endl;
#endif
                freqFile.open(fileName);
                freqFile << newValue;
                freqFile.close();
            }

            for (auto& fileName : freqWMinFileName) {
#ifdef DEBUG
                std::cout << "Writing " << newValue << " to " << fileName << std::endl;
#endif
                freqFile.open(fileName);
                freqFile << newValue;
                freqFile.close();
            }
        } else {

            for (auto& fileName : freqWMinFileName) {
#ifdef DEBUG
                std::cout << "Writing " << newValue << " to " << fileName << std::endl;
#endif
                freqFile.open(fileName);
                freqFile << newValue;
                freqFile.close();
            }

            for (auto& fileName : freqWMaxFileName) {
#ifdef DEBUG
                std::cout << "Writing " << newValue << " to " << fileName << std::endl;
#endif
                freqFile.open(fileName);
                freqFile << newValue;
                freqFile.close();
            }
        }
    }
}

void CPUFrequency::readFromSystem() {
    values[0] = 0;
    double newValue;
    std::ifstream freqFile;
    for (auto& fileName : freqRFileName) {
        freqFile.open(fileName);
        freqFile >> newValue;
        freqFile.close();
#ifdef DEBUG
        std::cout << "Reading " << newValue << " from " << fileName << std::endl;
#endif
        if (newValue > values[0]) {
            values[0] = newValue;
        }
    }
    if (actualWriteValue != 0 && values[0] != actualWriteValue) {
#ifdef DEBUG
        std::cout << "Supposedly " << actualWriteValue << std::endl;
#endif
        in->receiveValues(Vector({actualWriteValue}));
    }
}

IdleInject::IdleInject(std::string name) : Input(name) {
    DIR* dir;
    struct dirent * dEntry;
    std::string deviceTypeFileName, deviceType;
    std::ifstream file;
    bool foundpClamp = false;

    if ((dir = opendir(dirName.c_str())) != NULL) {
        //Check all the entries and find the intel powerclamp file
        while ((dEntry = readdir(dir)) != NULL) {
            std::string deviceDirName(dEntry->d_name);
            deviceTypeFileName = dirName + "/" + deviceDirName + devicetypePostfix;
            file.open(deviceTypeFileName);
            file >> deviceType;
            file.close();
#ifdef DEBUG
            std::cout << "Checking " << deviceTypeFileName << " with entry " << deviceType << std::endl;
#endif
            if (deviceType.compare("intel_powerclamp") == 0) {
                pclampSetFileName = dirName + "/" + deviceDirName + pclampSetFileNamePostfix;
                pclampMaxFileName = dirName + "/" + deviceDirName + pclampMaxFileNamePostfix;
#ifdef DEBUG
                std::cout << "IdleCycleInject: " << pclampSetFileName << std::endl;
#endif
                foundpClamp = true;
            }
        }
        closedir(dir);
    }

    if (!foundpClamp) {
        std::cout << "Intel PowerClamp does not exist! " << std::endl;
        std::exit(EXIT_FAILURE);
    }

    uint32_t mVal;
    file.open(pclampMaxFileName);
    file >> mVal;
    file.close();
    for (int i = 0; i <= mVal; i = i + 4) {
        allowedValues.push_back(i);
    }
    updateMinMaxMid();
    setMinValue();
    updateValuesFromSystem();
}

void IdleInject::writeToSystem() {
#ifdef DEBUG
    std::cout << " Writing " << actualWriteValue << " to " << pclampSetFileName << std::endl;
#endif

    uint32_t val = (uint32_t) values[0];
    uint32_t newValue = (uint32_t) actualWriteValue;
    if (newValue == val) {
        return;
    }

    std::ofstream file(pclampSetFileName);
    file << actualWriteValue;
    values[0] = actualWriteValue; //here, we are not reading the value from the 
    //system because you can write any value to this file and when you read you simply 
    //get it back. However, it might not get applied. So, we keep track of the 
    //actual values we write.
}

void IdleInject::readFromSystem() {
    int32_t val;
    std::ifstream file(pclampSetFileName);
    file >> val;
    if (val == -1) {
        values[0] = 0;
    }
    //here, we are not reading the value from the 
    //system because you can write any value to this file and when you read you simply 
    //get it back. However, it might not get applied. So, we keep track of the 
    //actual values we write.
}

void IdleInject::reset() {
    std::ofstream file(pclampSetFileName);
    file << 0;
}

PowerBalloon::PowerBalloon(std::string name) : Input(name) {
    uint32_t maxLevel;
    std::ifstream pbFile(pbMaxFileName);
    if (!pbFile) {
        std::cout << pbMaxFileName << " does not exist! " << std::endl;
        std::exit(EXIT_FAILURE);
    }
    pbFile >> maxLevel;
    pbFile.close();
#ifdef DEBUG
    std::cout << " Reading max" << maxLevel << " from " << pbMaxFileName << std::endl;
#endif
    for (uint32_t i = 0; i <= maxLevel; i = i + 2) {
        allowedValues.push_back(i);
    }
    updateMinMaxMid();
    setMinValue();
    updateValuesFromSystem();
}

void PowerBalloon::readFromSystem() {
    uint32_t tmp;
    std::ifstream pbFile(pbFileName);
    pbFile >> tmp;
#ifdef DEBUG
    std::cout << " Reading " << tmp << " from " << pbFileName << std::endl;
#endif
    values[0] = tmp;
}

void PowerBalloon::writeToSystem() {
#ifdef DEBUG
    std::cout << " Writing " << actualWriteValue << " to " << pbFileName << std::endl;
#endif
    if ((uint32_t) values[0] == (uint32_t) actualWriteValue) {
        return;
    }
    std::ofstream pbFile(pbFileName);
    pbFile << (uint32_t) actualWriteValue << "\n";
}

void PowerBalloon::reset() {
    setMinValue();
}

/*

NumCores::NumCores(std::string name) : Input(name) {
    std::string coreStatusString;
    std::ifstream coreFile(presentCoreFileName);
    coreFile >> coreStatusString;
    auto delimPos = coreStatusString.find("-");
    uint32_t startCore = -1, endCore = -1;
    if (delimPos != std::string::npos) {
        startCore = std::stoul(coreStatusString.substr(0, delimPos));
        endCore = std::stoul(coreStatusString.substr(delimPos + 1));
        numCores = endCore - startCore + 1;
    } else {
        numCores = 1;
    }
#ifdef DEBUG
    std::cout << "StartCore is " << startCore << " endCore is " << endCore << std::endl;
#endif

    std::vector<bool> newCoreActivity(numCores, false);
    for (auto core = startCore; core <= endCore; core++) {
        newCoreActivity[core] = true;
        allowedValues.push_back(core + 1);
    }
    updateMinMaxMid();
    coreStatus.setTotalUnits(numCores);
    coreStatus.setUnitStatus(newCoreActivity);
    setMaxValue();
    updateValuesFromSystem();
}

void NumCores::writeToSystem() {
    readFromSystem();
    int32_t deltaCores = (uint32_t) actualWriteValue - (uint32_t) values[0];
#ifdef DEBUG
    std::cout << "Writing to " << name << " with value " << actualWriteValue << " and current values is " <<
            values << std::endl;
    std::cout << "Delta cores is " << deltaCores << std::endl;
#endif
    if (deltaCores == 0) {
        return;
    }
    std::ofstream coreStatusFile;

    for (auto core = 1; core < numCores && deltaCores != 0; core++) {
        std::string coreStatusFileName(coreStatusFileNamePrefix);
        if (deltaCores > 0) {

            if (coreStatus.getUnitStatus(core) == false) {
                coreStatusFileName = coreStatusFileName.append(std::to_string(core)).append(coreStatusFileNamePostfix);
#ifdef DEBUG

                std::cout << " Writing to " << coreStatusFileName << std::endl;
#endif
                coreStatusFile.open(coreStatusFileName);
                coreStatusFile << 1;
                coreStatusFile.close();
                coreStatus.setUnitStatus(core, true);
                deltaCores--;
            }
        } else {
            if (coreStatus.getUnitStatus(core) == true) {
                coreStatusFileName = coreStatusFileName.append(std::to_string(core)).append(coreStatusFileNamePostfix);
#ifdef DEBUG

                std::cout << " Writing to " << coreStatusFileName << std::endl;
#endif
                coreStatusFile.open(coreStatusFileName);
                coreStatusFile << 0;
                coreStatusFile.close();
                coreStatus.setUnitStatus(core, false);
                deltaCores++;
            }
        }
    }
}

void NumCores::readFromSystem() {
    std::string coreStatusString;
    std::ifstream onlineFile(onlineCoreFileName);
    onlineFile >> coreStatusString;

#ifdef DEBUG
    std::cout << "Core status string is " << coreStatusString << std::endl;
#endif
    //use strtok, delimiter is ','
    values[0] = 0;
    std::vector<bool> newCoreActivity(numCores, false);

    char *coreStatusCString = new char[coreStatusString.length() + 1];
    std::strcpy(coreStatusCString, coreStatusString.c_str());
    auto token = strtok(coreStatusCString, ",");
    while (token != nullptr) {
        //token is of the form <a> i.e. core a alone or <a-b> i.e.
        //cores a to b both inclusive.
        std::string tokenStr(token);
        auto delimPos = tokenStr.find("-");
        if (delimPos != std::string::npos) {
            uint32_t startCore = -1, endCore = -1;
            startCore = std::stoul(tokenStr.substr(0, delimPos));
            endCore = std::stoul(tokenStr.substr(delimPos + 1));
            for (auto core = startCore; core <= endCore; core++) {
                newCoreActivity[core] = true;
                values[0]++;
            }
        } else {
            newCoreActivity[std::stoul(tokenStr)] = true;
            values[0]++;
        }
        token = strtok(NULL, ",");
    }
    coreStatus.setUnitStatus(newCoreActivity);
#ifdef DEBUG
    coreStatus.print();
#endif
}

DummySrc::DummySrc(std::string name) :
op(std::make_shared<OutputPort>(name)) {

}

DummySrc::DummySrc(std::string name, std::initializer_list<std::string> portNames) :
op(std::make_shared<OutputPort>(name, portNames)) {

}

void DummySrc::genValues() {
    op->updateValuesToPort(Vector({5}));
}
 */
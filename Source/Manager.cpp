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
 * File:   Manager.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#include "Manager.h"
#include "debug.h"

#include <iomanip>
#include <signal.h>
#include <cstring>
#include <thread>

std::atomic<bool> stopRunning(false); //signal flag

void receivedSigInt(int) {
    stopRunning.store(true);
}

void Manager::setupSigKillHandler() {
    struct sigaction sa; //signal handling structure
    memset(&sa, 0, sizeof (sa)); //set entries to 0
    sa.sa_handler = receivedSigInt; //change the signal handler to receivedSig()
    sigfillset(&sa.sa_mask); //block all signals (SIGTERM can't be blocked)
    sigaction(SIGINT, &sa, NULL); //bind sa with SIGINT signal.
}

Manager::Manager(uint32_t samplingIntervalMS, Mode mode) :
samplingIntervalMS(samplingIntervalMS),
mode(mode) {
    setupSigKillHandler();
}

void Manager::addInput(std::unique_ptr<Input> newInput) {
    if (newInput == nullptr) {
        std::cout << "Cannot add Null pointer as input" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    //Check if an input with the same name is added twice
    auto newInputPinNames = newInput->out->getPinNames();
    for (auto& input : inputList) {
        auto pinNames = input->out->getPinNames();
        for (auto& pinName : pinNames) {
            if (std::find(newInputPinNames.begin(), newInputPinNames.end(), pinName) != newInputPinNames.end()) {
                std::cout << "Cannot add two inputs with same name: " << pinName << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
    }
#ifdef DEBUG
    std::cout << "Adding " << newInput->getName() << " with index " << inputList.size() - 1;
#endif
    inputList.push_back(std::move(newInput));
}

void Manager::addSensor(std::unique_ptr<Sensor> newSensor) {
    if (newSensor == nullptr) {
        std::cout << "Cannot add Null pointer as sensor" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    auto newSensorPinNames = newSensor->out->getPinNames();

    //Check if a sensor with the same name is added twice
    for (auto& sensor : sensorList) {
        auto pinNames = sensor->out->getPinNames();
        for (auto& pinName : pinNames) {
            if (std::find(newSensorPinNames.begin(), newSensorPinNames.end(), pinName) != newSensorPinNames.end()) {
                std::cout << "Cannot add two sensors with same name: " << pinName << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
    }
    sensorList.push_back(std::move(newSensor));
}

void Manager::addSysIdParams(std::vector<std::string> sysidList_,
        std::initializer_list<uint32_t> minHoldTime,
        std::initializer_list<uint32_t> maxHoldTime,
        std::initializer_list<uint32_t> initHoldTime) {
#ifdef DEBUG
    for (auto& item : sysidList_) {
        std::cout << "Asked id for " << item << std::endl;
    }
#endif
    sysidInputNameList = sysidList_;
    holdPeriods = std::vector<uint32_t>(initHoldTime);
    minHoldPeriods = std::vector<uint32_t>(minHoldTime);
    maxHoldPeriods = std::vector<uint32_t>(maxHoldTime);

    auto numSysidInputs = sysidInputNameList.size();
    holdCounters = std::vector<uint32_t>(numSysidInputs, 0);

    if (holdPeriods.empty()) {
        holdPeriods = std::vector<uint32_t>(numSysidInputs, defaultMinHoldPeriod + 1);
    } else if (numSysidInputs != holdPeriods.size()) {
        std::cout << "Incorrect number of hold periods specified " << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (minHoldPeriods.empty()) {
        minHoldPeriods = std::vector<uint32_t>(numSysidInputs, defaultMinHoldPeriod);
    } else if (numSysidInputs != minHoldPeriods.size()) {
        std::cout << "Incorrect number of min hold periods specified " << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (maxHoldPeriods.empty()) {
        maxHoldPeriods = std::vector<uint32_t>(numSysidInputs, defaultMaxHoldperiod);
    } else if (numSysidInputs != maxHoldPeriods.size()) {
        std::cout << "Incorrect number of max hold periods specified " << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void Manager::addController(std::string name, std::initializer_list<std::string> opNames,
        std::initializer_list<std::string> ipNames, ControllerType ctlType,
        std::string dirPath, std::string fileName, uint32_t smplInt) {
    std::unique_ptr<Controller> controller;
    if (ctlType == ControllerType::Dummy) {
        controller = std::make_unique<Controller>(name, smplInt);
    } else if (ctlType == ControllerType::SSV) {
        controller = std::make_unique<RobustController>(name, fileName, m, smplInt);
    }
    //set width of ports in controller to take in outputs, curr inputs, curr targets and set new inputs
    //opNames, ipNames could be pins or ports
    for (auto& opName : opNames) {
        auto srcPort = sensorList[getSensorIndexInList(opName)]->out;
        std::vector<std::string> pinNames;
        if (isNameSensorPort(opName)) {
            pinNames = srcPort->getPinNames();
        } else if (isNameSensorPin(opName)) {
            pinNames.push_back(opName);
        }
        controller->outputVals->addPin(pinNames);
        controller->outputTargetVals->addPin(pinNames);
        controller->currOutputTargetVals->addPin(pinNames);
        sysReadWires.push_back(std::make_unique<Wire>(srcPort, pinNames, controller->outputVals, pinNames));
    }

    for (auto& ipName : ipNames) {
        auto srcPort = inputList[getInputIndexInList(ipName)]->out;
        auto destPort = inputList[getInputIndexInList(ipName)]->in;
        std::vector<std::string> srcPinNames, destPinNames;
        if (isNameInputPort(ipName)) {
            srcPinNames = srcPort->getPinNames();
            destPinNames = destPort->getPinNames();
        } else if (isNameInputPin(ipName)) {
            srcPinNames.push_back(ipName);
            destPinNames.push_back(ipName);
        }
        controller->currInputVals->addPin(srcPinNames);
        sysReadWires.push_back(std::make_unique<Wire>(srcPort, srcPinNames, controller->currInputVals, srcPinNames));

        controller->newInputVals->addPin(destPinNames);
        sysWriteWires.push_back(std::make_unique<Wire>(controller->newInputVals, destPinNames, destPort, destPinNames));
    }
    controllerList.push_back(std::move(controller));
}

void Manager::addMaskGenerator(std::string name, std::string controllerName, MaskGenType maskType,
        std::string dirPath, std::string fileName, uint32_t smplInt, bool randomProp) {
    //Create  planner
    std::unique_ptr<Planner> planner;
    if (maskType == MaskGenType::Constant) {
        planner = std::make_unique<Planner>(name, fileName, m, smplInt);
    } else if (maskType == MaskGenType::Gauss) {
        planner = std::make_unique<MaskGenerator>(name, dirPath, fileName, smplInt, SignalType::Normal, randomProp);
    } else if (maskType == MaskGenType::Sine) {
        planner = std::make_unique<MaskGenerator>(name, dirPath, fileName, smplInt, SignalType::Sine, randomProp);
    } else if (maskType == MaskGenType::GaussSine) {
        planner = std::make_unique<MaskGenerator>(name, dirPath, fileName, smplInt, SignalType::GaussSine, randomProp);
    } else if (maskType == MaskGenType::Uniform) {
        planner = std::make_unique<MaskGenerator>(name, dirPath, fileName, smplInt, SignalType::Uniform, randomProp);
    } else if (maskType == MaskGenType::Preset) {
        planner = std::make_unique<Planner>(name, dirPath, fileName, smplInt, true);
    }

    //Find the corresponding controller
    uint32_t ctlListIndex = 0;
    bool foundCtl = false;
    for (auto& controller : controllerList) {
        if (controllerName.compare(controller->getName()) == 0) {
            foundCtl = true;
            break;
        }
        ctlListIndex++;
    }
    if (!foundCtl) {
        std::cout << "Incorrect controller name to attach for mask generators " << std::endl;
        std::exit(EXIT_FAILURE);
    }

    //set width of ports in planner to take in outputs, curr inputs, and set new targets
    std::vector<std::string> pinNames = controllerList[ctlListIndex]->outputVals->getPinNames();
    for (auto& opName : pinNames) {
        auto srcPort = sensorList[getSensorIndexInList(opName)]->out;
        planner->currOutputVals->addPin(opName);
        sysReadWires.push_back(std::make_unique<Wire>(srcPort, opName, planner->currOutputVals, opName));
        planner->newOutputTargetVals->addPin(opName);
    }
    auto destPort = controllerList[ctlListIndex]->outputTargetVals;
    blockWires.push_back(std::make_unique<Wire>(planner->newOutputTargetVals, destPort));

    pinNames = controllerList[ctlListIndex]->currInputVals->getPinNames();
    for (auto& ipName : pinNames) {
        auto srcPort = inputList[getInputIndexInList(ipName)]->out;
        planner->currInputVals->addPin(ipName);
        sysReadWires.push_back(std::make_unique<Wire>(srcPort, ipName, planner->currInputVals, ipName));
    }
    plannerList.push_back(std::move(planner));
}

NameType Manager::getInputNameType(std::string name) {
    for (auto& ip : inputList) {
        if (name.compare(ip->getName()) == 0) {
            return NameType::Port;
        }
    }
    for (auto& ip : inputList) {
        auto pinNames = ip->out->getPinNames();
        for (auto& pinName : pinNames) {
            if (name.compare(pinName) == 0) {
                return NameType::Pin;
            }
        }
    }
    return NameType::Invalid;
}

NameType Manager::getSensorNameType(std::string name) {
    for (auto& op : sensorList) {
        if (name.compare(op->getName()) == 0) {
            return NameType::Port;
        }
    }
    for (auto& op : sensorList) {
        auto pinNames = op->out->getPinNames();
        for (auto& pinName : pinNames) {
            if (name.compare(pinName) == 0) {
                return NameType::Pin;
            }
        }
    }
    return NameType::Invalid;
}

bool Manager::isNameInputPin(std::string name) {
    for (auto& ip : inputList) {
        auto ipPinNames = ip->out->getPinNames();
        for (auto& ipPinName : ipPinNames) {
            if (name.compare(ipPinName) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool Manager::isNameSensorPin(std::string name) {
    for (auto& op : sensorList) {
        auto opPinNames = op->out->getPinNames();
        for (auto& opPinName : opPinNames) {
            if (name.compare(opPinName) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool Manager::isNameInputPort(std::string name) {
    for (auto& ip : inputList) {
        if (name.compare(ip->getName()) == 0) {
            return true;
        }
    }
    return false;
}

bool Manager::isNameSensorPort(std::string name) {
    for (auto& op : sensorList) {
        if (name.compare(op->getName()) == 0) {
            return true;
        }
    }
    return false;
}

uint32_t Manager::getInputIndexInList(std::string name) {
    uint32_t i = 0;
    for (auto& ip : inputList) {
        if (name.compare(ip->getName()) == 0) {
#ifdef DEBUG
            std::cout << "Found input port for name " << name << std::endl;
#endif
            return i;
        } else {
            auto ipPinNames = ip->out->getPinNames();
            for (auto& ipPinName : ipPinNames) {
                if (name.compare(ipPinName) == 0) {
#ifdef DEBUG
                    std::cout << "Found pin for name " << name << std::endl;
#endif
                    return i;
                }
            }
        }
        i++;
    }
    std::cout << "Cannot find non-existing input name " << name << std::endl;
    std::exit(EXIT_FAILURE);
}

uint32_t Manager::getSensorIndexInList(std::string name) {
    uint32_t i = 0;
    for (auto& op : sensorList) {
        if (name.compare(op->getName()) == 0) {
#ifdef DEBUG
            std::cout << "Found sensor port for name " << name << std::endl;
#endif
            return i;
        } else {
            auto opPinNames = op->out->getPinNames();
            for (auto& opPinName : opPinNames) {
                if (name.compare(opPinName) == 0) {
#ifdef DEBUG
                    std::cout << "Found pin for name " << name << std::endl;
#endif
                    return i;
                }
            }
        }
        i++;
    }
    std::cout << "Cannot find non-existing sensor name " << name << std::endl;
    std::exit(EXIT_FAILURE);
}

void Manager::updateValuesFromSystem() {
    Vector values;
    for (auto& sensor : sensorList) {
        sensor->updateValuesFromSystem();
        values = sensor->out->transmitValues();
#ifdef DEBUG
        std::cout << sensor->getName() << " " << values;
#endif
    }
    for (auto& input : inputList) {
        input->updateValuesFromSystem();
        values = input->out->transmitValues();
#ifdef DEBUG
        std::cout << input->getName() << " " << values;
#endif
    }
}

void Manager::updateValuesToSystem() {
    for (auto& input : inputList) {
        input->updateValueToSystem();
    }
}

void Manager::run() {
    completeInit();
    //run once to initialize readings
    updateValuesFromSystem();
    updateValuesToSystem();
    std::this_thread::sleep_for(std::chrono::milliseconds(samplingIntervalMS));
    //continue loop
    while (!stopRunning.load()) {
#ifdef DEBUG
        std::cout << "-------------------------------------------Round--------------------------------------" << std::endl;
#endif
        updateValuesFromSystem();
        displayValues();
        transferSysReadings();
        switch (mode) {
            case Mode::Sysid:
                runSysid();
                break;
            case Mode::Control:
                transferBlockWires();
                runControl();
                break;
        }
        transferSysWrites();
        updateValuesToSystem();
#ifdef DEBUG
        std::cout << "--------------------------------------------------------------------------------------" << std::endl;
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(samplingIntervalMS));
    }
    resetInputs();
#ifdef DEBUG
    std::cout << "Ending" << std::endl;
#endif
}

void Manager::transferBlockWires() {
    for (auto& wire : blockWires) {
        wire->transfer();
    }
}

void Manager::transferSysReadings() {
    for (auto& wire : sysReadWires) {
        wire->transfer();
    }
}

void Manager::transferSysWrites() {
    for (auto& wire : sysWriteWires) {
        wire->transfer();
    }
}

void Manager::runControl() {
#ifdef DEBUG
    std::cout << "Running planners" << std::endl;
#endif
    for (auto& planner : plannerList) {
#ifdef DEBUG
        std::cout << "Running planner " << planner->getName() << std::endl;
#endif
        planner->run();
    }


#ifdef DEBUG
    std::cout << "Running controllers" << std::endl;
#endif
    for (auto& controller : controllerList) {
#ifdef DEBUG
        std::cout << "Running controller " << controller->getName() << std::endl;
#endif
        controller->run();
    }
}

void Manager::runSysid() {
    auto i = 0;
    for (auto& holdCounter : holdCounters) {
        holdCounter++;
        if (holdCounter == holdPeriods[i]) {
            inputList[inputIndicesForSysid[i]]->setRandomValue();
            holdCounter = 0;
            holdPeriods[i] = minHoldPeriods[i] + rand() % (maxHoldPeriods[i] - minHoldPeriods[i] + 1);
#ifdef DEBUG
            std::cout << "New hold period for input " << inputList[inputIndicesForSysid[i]]->getName()
                    << " is " << holdPeriods[i] << std::endl;
#endif
        }
        i++;
    }
}

void Manager::resetInputs() {
    for (auto& input : inputList) {
        input->reset();
    }
}

void Manager::displayHeader() {
    std::vector<std::string> names;
    for (auto& sensor : sensorList) {
        names = sensor->out->getPinNames();
        for (auto& name : names) {
            std::cout << name << " ";
        }
    }
    for (auto& input : inputList) {
        names = input->out->getPinNames();
        for (auto& name : names) {
            std::cout << name << " ";
        }
    }

    if (mode == Mode::Control) {
        for (auto& ctl : controllerList) {
            auto targetNames = ctl->currOutputTargetVals->getPinNames();
            for (auto& tName : targetNames) {
                std::cout << "Target@" << tName << " ";
            }
        }
    }
    std::cout << std::endl;
}

void Manager::displayValues() {
    Vector values;
    for (auto& sensor : sensorList) {
        values = sensor->out->transmitValues();
        for (auto& value : values) {
            std::cout << std::setprecision(3) << std::fixed << value << " ";
        }
    }
    for (auto& input : inputList) {
        values = input->out->transmitValues();
        for (auto& value : values) {
            std::cout << std::setprecision(2) << std::fixed << value << " ";
        }
    }

    if (mode == Mode::Control) {
        for (auto& ctl : controllerList) {
            auto targetValues = ctl->currOutputTargetVals->transmitValues();
            for (auto& tValue : targetValues) {
                std::cout << std::setprecision(2) << std::fixed << tValue << " ";
            }
        }
    }
    std::cout << std::endl;
}

void Manager::completeInit() {
    if (mode == Mode::Sysid) {
        for (auto& name : sysidInputNameList) {
            inputIndicesForSysid.push_back(getInputIndexInList(name));
        }
        Vector latencyResults;

        for (auto& input : inputList) {
#ifdef DEBUG
            latencyResults = input->measureWriteLatency();
            std::cout << " Write Latency (max-min) for " << input->getName() << " " << latencyResults[0] << " us" << std::endl;
            std::cout << " Write Latency (min-max) for " << input->getName() << " " << latencyResults[1] << " us" << std::endl;
#endif
            input->setMinValue();
        }
    }
    //Initialize numcores and cpu frequency to maximum
    /*
    for (auto& input : inputList) {
        auto name = input->getName();
        if (name.compare("NumCores") == 0) {
            input->setMaxValue();
        } else if (name.compare("CPUFreq") == 0) {
            input->setMaxValue();
        }
    }
    */
    displayHeader();
}


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
 * File:   Manager.h
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#ifndef MANAGER_H
#define MANAGER_H

#include "Sensors.h"
#include "Inputs.h"
#include "Controller.h"
#include "Planner.h"

#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <map>

enum class Mode {
    Baseline,
    Sysid,
    Mask,
    Invalid
};

enum class NameType {
    Port,
    Pin,
    Invalid
};

enum class ControllerType {
    SSV,
    Dummy
};

enum class MaskGenType {
    Constant,
    Uniform,
    Gauss,
    Sine,
    GaussSine,
    Preset // use a precomputed target from a file
};

/*
 * This class is like the orchestrator for the entire processes.
 * The Manager object is used to hold the controllers, planners, sensors, inputs 
 * and their connections. These blocks are wired automatically and values are processed 
 * accordingly.
 */
class Manager {
public:
    void addSensor(std::unique_ptr<Sensor> newSensor);
    void addInput(std::unique_ptr<Input> newInput); //add an input
    void addSysIdParams(std::vector<std::string> sysidList_ = {},
    std::initializer_list<uint32_t> minHoldTime = {}, std::initializer_list<uint32_t> maxHoldTime = {},
    std::initializer_list<uint32_t> initHoldTime = {});
    void addController(std::string name, std::initializer_list<std::string> opNames,
            std::initializer_list<std::string> ipNames, ControllerType ctlType = ControllerType::Dummy,
            std::string dirPath = "", std::string fileName = "", uint32_t smplInt = 1);
    void addMaskGenerator(std::string name, std::string controllerName, 
        MaskGenType maskType = MaskGenType::Constant, std::string dirPath ="", 
        std::string fileName ="", uint32_t smplInt = 1, bool randomizeMaskProps = false);
    void run();
    Manager(uint32_t samplingIntervalMS, Mode mode);

private:
    void updateValuesFromSystem(); //read values from system into sensor modules
    void updateValuesToSystem(); //write values from input modules to system
    void transferBlockWires(); //transfer values on wires between components
    void transferSysReadings();
    void transferSysWrites();

    void displayValues();
    void displayHeader();

    void runSysid();
    void runControl();

    void resetInputs();

    void setupSigKillHandler(); //when sigkill is issued, shutdown all counters properly

    bool isNameSensorPin(std::string name);
    bool isNameInputPin(std::string name);
    bool isNameSensorPort(std::string name);
    bool isNameInputPort(std::string name);

    NameType getSensorNameType(std::string name);
    NameType getInputNameType(std::string name);

    uint32_t getInputIndexInList(std::string name);
    uint32_t getSensorIndexInList(std::string name);

    void completeInit();

    Mode mode;
    uint32_t samplingIntervalMS;
    std::vector<std::unique_ptr < Sensor>> sensorList;
    std::vector<std::unique_ptr < Input>> inputList;
    std::vector<std::unique_ptr <Controller>> controllerList;
    std::vector<std::unique_ptr <Planner>> plannerList;
    std::vector<std::unique_ptr <Wire>> sysReadWires, sysWriteWires, blockWires;

    std::vector<std::vector<std::unique_ptr < Input>>::size_type> inputIndicesForSysid;
    std::vector<std::string> sysidInputNameList;
    std::vector<uint32_t> holdPeriods, minHoldPeriods, maxHoldPeriods, holdCounters;
    uint32_t defaultMinHoldPeriod = 2, defaultMaxHoldperiod = 20; //2, 20 for freq, 2, 10 for freq, numcores
};
#endif /* MANAGER_H */

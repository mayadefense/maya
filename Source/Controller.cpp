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
 * File:   Controller.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */



#include "Controller.h"
#include "debug.h"
#include "Abstractions.h"
#include "Sensors.h"
#include <fstream>

void Controller::run() {
    bool run = false;
    if (cycles == samplingInterval) {
        cycles = 1;
        run = true;
    } else {
        cycles++;
    }
    auto newValues = computeNewInputs(run);

#ifdef DEBUG
    std::vector<std::string> inputNames = newInputVals->getPinNames();
    std::cout << "Controller setting values: " << newValues << " for ";
    for (auto& inputName : inputNames) {
        std::cout << inputName << " ";
    }
    std::cout << std::endl;
#endif
    newInputVals->updateValuesToPort(newValues);
    currOutputTargetVals->updateValuesToPort(outputTargetVals->updateValuesFromPort());
}

Vector Controller::computeNewInputs(bool run) {
#ifdef DEBUG
    std::cout << "------Controller------" << std::endl;
#endif

    if (run) {
        auto currOpVals = outputVals->updateValuesFromPort();
        auto currIpVals = currInputVals->updateValuesFromPort();
        auto newIpVals = currInputVals->updateValuesFromPort() - 5;
#ifdef DEBUG
        std::cout << "currOps " << currOpVals << "currIps " << currIpVals;
        std::cout << "newIps " << newIpVals;
#endif
        return newIpVals;
    } else {
#ifdef DEBUG
        std::cout << "Skipping" << std::endl;
#endif
        auto currIpVals = currInputVals->updateValuesFromPort();
        return currIpVals;
    }
}

void Controller::reset() {

}

std::string Controller::getName() {
    return name;
}

Controller::Controller(std::string name, uint32_t smplInt) :
name(name),
newInputVals(std::make_shared<OutputPort>("newInputVals")),
currOutputTargetVals(std::make_shared<OutputPort>("currOutputTargetVals")),
currInputVals(std::make_shared<InputPort>("currInputVals")),
outputVals(std::make_shared<InputPort>("outputVals")),
outputTargetVals(std::make_shared<InputPort>("outputTargetVals")),
samplingInterval(smplInt),
cycles(smplInt) {
#ifdef DEBUG
    std::cout << "Creating controller " << name << std::endl;
#endif

}

RobustController::RobustController(std::string name, std::string dirPath, std::string ctlFileName, uint32_t smplInt) :
Controller(name, smplInt) {
    std::ifstream file;
    uint32_t dimension, numInputs, numMeasurements;
    std::string fileNamePrefix = dirPath + "/"+ ctlFileName;
    file.open(fileNamePrefix + "_dimension.txt");
    if (!file) {
        std::cerr << "Unable to open " << fileNamePrefix << "_dimension.txt" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    file >> dimension;
    file.close();

    file.open(fileNamePrefix + "_numInputs.txt");
    if (!file) {
        std::cerr << "Unable to open " << fileNamePrefix << "_numInputs.txt" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    file >> numInputs;
    file.close();

    file.open(fileNamePrefix + "_numYmeas.txt");
    if (!file) {
        std::cerr << "Unable to open " << fileNamePrefix << "_numYmeas.txt" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    file >> numMeasurements;
    file.close();

    state = Vector(dimension);

    A = Matrix(dimension, dimension);
    B = Matrix(dimension, numMeasurements);
    C = Matrix(numInputs, dimension);
    D = Matrix(numInputs, numMeasurements);

    A.from_file(fileNamePrefix + "_A.txt");
#ifdef DEBUG
    std::cout << "A\n" << A;
#endif
    B.from_file(fileNamePrefix + "_B.txt");
#ifdef DEBUG
    std::cout << "B\n" << B;
#endif
    C.from_file(fileNamePrefix + "_C.txt");
#ifdef DEBUG
    std::cout << "C\n" << C;
#endif
    D.from_file(fileNamePrefix + "_D.txt");
#ifdef DEBUG
    std::cout << "D\n" << D;
#endif


    inputDenormalizeScales.from_file(fileNamePrefix + "_scaleInputsUp.txt");
    outputNormalizeScales.from_file(fileNamePrefix + "_scaleYmeasDown.txt");

#ifdef DEBUG
    std::cout << "inputDenormalizationScales\n" << inputDenormalizeScales;
#endif
#ifdef DEBUG
    std::cout << "outputNormalizationScales\n" << outputNormalizeScales;
#endif

}

Vector RobustController::computeNewInputs(bool run) {
#ifdef DEBUG
    std::cout << "------Robust Controller: " << name << "------" << std::endl;
#endif

    auto currIpVals = currInputVals->updateValuesFromPort();
    auto currTargets = outputTargetVals->updateValuesFromPort();
    auto currOpVals = outputVals->updateValuesFromPort();
    if (run) {
        deltaOutputs = currTargets - currOpVals;
        auto normalizedDeltaOutputs = (deltaOutputs) * outputNormalizeScales;

        auto newState = A * state + B*normalizedDeltaOutputs;
        auto newNormalizedIps = C * state + D*normalizedDeltaOutputs;
        auto newIpVals = (newNormalizedIps * inputDenormalizeScales) + currIpVals;

#ifdef DEBUG
        std::cout << "currIpVals " << currIpVals << "currOpVals " << currOpVals <<
                "currTargets " << currTargets << "deltaOutputs " << deltaOutputs <<
                " normalizedDeltaOutputs " << normalizedDeltaOutputs <<
                " newNormalizedIps " << newNormalizedIps << " newState " << newState;
#endif

        state = newState;
        return newIpVals;
    } else {
#ifdef DEBUG
        std::cout << "Skipping" << std::endl;
#endif
        return currIpVals;
    }
}





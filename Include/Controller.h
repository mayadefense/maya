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
 * File:   Controller.h
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Abstractions.h"
#include "MathSupport.h"
#include <string>

/* Any controller to change inputs and meet targets can be derived from the general 
 * Controller class. Such controllers must re-define the computeNewInputs() function.
 */
class Controller {
public:
    Controller(std::string name, uint32_t smplInt = 1);
    std::string getName();
    void run();
    virtual void reset();
    std::shared_ptr<OutputPort> newInputVals, currOutputTargetVals;
    std::shared_ptr<InputPort> currInputVals, outputVals, outputTargetVals;
protected:
    virtual Vector computeNewInputs(bool run);
    std::string name;
    uint32_t samplingInterval, cycles;
};

//A Robust controller is a control theory controller. See README
class RobustController : public Controller {
public:
    RobustController(std::string name, std::string dirPath, std::string ctlFileName, uint32_t smplInt = 1);
    Vector computeNewInputs(bool run) override;
private:
    Matrix A, B, C, D;
    Vector state, deltaOutputs;

    Vector inputDenormalizeScales, outputNormalizeScales;
    //std::string dirPath = "/home/pothuku2/Research/visakha/code/Matlab/Controllers/";
};
#endif /* CONTROLLER_H */

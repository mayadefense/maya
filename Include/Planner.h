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
 * File:   Planner.h
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#ifndef PLANNER_H
#define PLANNER_H

#include "Abstractions.h"
#include "MathSupport.h"
#include <random>
#include <tuple>
#include <utility>
#include <string>

extern const uint32_t samplingIntervalMS;

//The different distributions
enum class SignalType {
    Normal,
    Uniform,
    Sine,
    GaussSine,
};

/*Parameters for the different distributions.
 * Normal: mu=param1,sigma=param2
 * Uniform: min=param1, max=param2
 * Sinusoid: param1+param3*sin(2*pi*param2*time); param1 = offset, param2 = freq, param3 = amplitude
 * GaussSine: param1+param3*sin(2*pi*param2*time) + Normal (0,param4)
 */
enum class Param {
    One,
    Two,
    Three,
    Four
};

//The Planner is any module that specifies a target for the controller.
//The MaskGenerator inherits this class and uses a SignalGenerator to produce different waveforms.
class Planner {
public:

    virtual void reset();
    void run();
    std::string getName();

    Planner(std::string name, std::string dirPath, std::string fileName, uint32_t smplInt=1, bool usePreset = false);

    std::shared_ptr<OutputPort> newOutputTargetVals;
    std::shared_ptr<InputPort> currInputVals, currOutputVals;

protected:
    virtual Vector computeNewTargets(bool run);
    std::string name, fileName, dirPath;
    Vector targets, outputs, maxLimits, minLimits;
    uint32_t periodInSamples, cycles;
    
    //use targets that were precomputed.
    bool usePresetTarget; 
    Matrix presetTargets;
    uint64_t presetTargetCounter;
};

class SignalGenerator {
public:
    SignalGenerator(SignalType sig, double minval, double maxval, double p1, double p2, double p3, double p4 );

    double getSignalValue();
    
    void enableRandomizedParam(Param p, std::pair<double, double> range); //make a parameter be selected randomly from a range
    void setParamRange(Param p, std::pair<double, double> range); //change the range from which a parameter is selected 
    void selectNewValForParam(Param p); //randomly select a new value for a parameter from its range, and apply it
    void setParam(Param p, double newVal); //apply value to param
    std::pair <double, double> getParamRange(Param p);
    Vector getParams();  
private:
    std::pair<double, double> sanitizeParamRanges(Param p, std::pair<double, double> range); //verify that the ranges are legal for the signal chosen
    void sanitizeParamValues(); //Make sure param values are properly set (e.g., values don;t go beyond maxVal)

    SignalType sigType;
    double param1, param2, param3, param4; //Signal parameters. See above on how they are used
    double minVal, maxVal; //signal genreated is always in [minVal,maxVal]

    //Distributions to generate signals
    std::normal_distribution<double> normalDist; 
    std::uniform_real_distribution<> uniformDist; 
    double time, sineSamplingFreq, minSineCycles; 
    
    //Each parameter can be varied randomly. So, maintian a distribution for each parameters
    bool randomizeParam1, randomizeParam2, randomizeParam3, randomizeParam4;
    std::uniform_real_distribution<> param1Dist, param2Dist, param3Dist, param4Dist; 
};

class MaskGenerator : public Planner {
public:
    MaskGenerator(std::string name, std::string dirPath, std::string fileName, 
            uint32_t smplInt = 1, SignalType sig = SignalType::Normal, bool randomProp = false);
protected:
    Vector computeNewTargets(bool run) override;
    
    SignalType signalType;
    std::vector<std::shared_ptr<SignalGenerator>> signalDists; //one signalDist for each output

    bool randomizeMaskProps;
    uint32_t maskPropHoldCounter, maskPropHoldPeriod;
    //bool randomizeMean, randomizeVariance;
    //uint32_t constVariancePeriod, constVarianceCounter, constMeanPeriod, constMeanCounter;
    //uint32_t uniformSigWaitPeriod, uniformSigWaitCounter;
private:
    bool shouldMaskPropChange();
    //bool shouldMeanChange();
    //bool shouldVarianceChange();
};

#endif /* PLANNER_H */

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
 * File:   Planner.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#include <fstream>
#include "Planner.h"
#include "debug.h"
#include "Inputs.h"
#define _USE_MATH_DEFINES
#include <cmath>

std::random_device randomDevice;
std::mt19937 randomGen(randomDevice());
std::uniform_int_distribution<> signalPropHoldDist(12, 125);

Planner::Planner(std::string name, std::string dirPath, std::string fileName, uint32_t smplInt, bool usePreset) :
name(name),
dirPath(dirPath),
fileName(fileName),
newOutputTargetVals(std::make_shared<OutputPort>("newOutputTargetVals")),
currInputVals(std::make_shared<InputPort>("currInputVals")),
currOutputVals(std::make_shared<InputPort>("currOutputVals")),
periodInSamples(smplInt),
cycles(smplInt),
usePresetTarget(usePreset),
presetTargetCounter(0) {
#ifdef DEBUG
    std::cout << "Creating planner " << name << std::endl;
#endif
    std::string fileNamePrefix = dirPath + "/" + fileName;

    std::ifstream file;
    uint64_t presetTargetLen = 0;

    maxLimits.from_file(fileNamePrefix + "_maxLimits.txt");
    minLimits.from_file(fileNamePrefix + "_minLimits.txt");
    targets.from_file(fileNamePrefix + "_targets.txt");
    if (usePresetTarget) {
        file.open(fileNamePrefix + "_presetlen.txt");
        if (!file) {
            std::cerr << "Unable to open " << fileNamePrefix << "_presetlen.txt" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        file >> presetTargetLen;
        file.close();
        presetTargets = Matrix(presetTargetLen, targets.size());
        presetTargets.from_file(fileNamePrefix + "_presets.txt");
    }

    //std::cout << minLimits << " " << maxLimits << " " << targets << std::endl;

}

std::string Planner::getName() {
    return name;
}

void Planner::reset() {
    std::string fileNamePrefix = dirPath + "/" + fileName;
    targets.from_file(fileNamePrefix + "_targets.txt");
    presetTargetCounter = 0;
}

void Planner::run() {
    bool run = false;
    if (cycles == periodInSamples) {
        run = true;
        cycles = 1;
    } else {
        cycles++;
    }

    auto newValues = computeNewTargets(run);

#ifdef DEBUG
    std::vector<std::string> outputNames = newOutputTargetVals->getPinNames();
    std::cout << "Planner setting targets: " << newValues << " for ";
    for (auto& outputName : outputNames) {
        std::cout << outputName << " ";
    }
    std::cout << std::endl;
#endif
    newOutputTargetVals->updateValuesToPort(newValues);
}

Vector Planner::computeNewTargets(bool run) {
#ifdef DEBUG
    std::cout << "---------Planner---------" << std::endl;
#endif

    outputs = currOutputVals->updateValuesFromPort();
    auto currIpVals = currInputVals->updateValuesFromPort();

    if (usePresetTarget) {
        targets = Vector(presetTargets[presetTargetCounter], targets.size());
#ifdef DEBUG
        std::cout << targets << " " << presetTargetCounter << std::endl;
#endif
        presetTargetCounter++;
        if (presetTargetCounter == presetTargets.row()) {
            presetTargetCounter = 0;
        }
    }

#ifdef DEBUG
    std::cout << "currOps " << outputs << "currIps " << currIpVals
            << "currTargets " << targets << "newTargets " << targets;
#endif
    return targets;
}

SignalGenerator::SignalGenerator(SignalType sig, double minval, double maxval, double p1, double p2, double p3, double p4) :
sigType(sig),
minVal(minval),
maxVal(maxval),
param1(p1),
param2(p2),
param3(p3),
param4(p4),
time(0.0),
minSineCycles(4.0),
randomizeParam1(false),
randomizeParam2(false),
randomizeParam3(false),
randomizeParam4(false) {
    if (minVal > maxVal) {
        std::cout << "Min " << minVal << " should be smaller than Max " << maxVal << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* The fastest sine we can produce has 1/3 the frequency of the sampling frequency (i.e., 1/sampling interval).
     * Nyquist criterion says that a sinusoid's sampling frequency must be at least twice that of the sinusoid.
     * For better fidelity, we are limiting the sinusoid's frequency to only 1/3 sampling frequency instead of 1/2
     * Additioanlly, we will limit the maximum frequency used by a sinusoid (param2) even further, 
     * because we want to see at least minSineCycles cycles of the sinusoid.
     */
    sineSamplingFreq = 1000.0 / (3 * (double) samplingIntervalMS);

    sanitizeParamValues();

    //Create the required distributions. Note that Sine isn't a distribution
    if (sigType == SignalType::Normal) {
        normalDist = std::normal_distribution<double>(param1, param2);
    } else if (sigType == SignalType::GaussSine) {
        normalDist = std::normal_distribution<double>(0, param4);
    } else if (sigType == SignalType::Uniform) {
        uniformDist.param(std::uniform_real_distribution<>::param_type(param1, param2));
    }
}

void SignalGenerator::sanitizeParamValues() {
    //Make sure minVal < param1 < maxVal. This is true for all signals
    param1 = std::max(param1, minVal);
    param1 = std::min(param1, maxVal);

    //Enforce signal specific rules
    if (sigType == SignalType::Sine || sigType == SignalType::GaussSine) {
        //param2 is frequency
        /* Make sure minFreq < param2 < maxFreq 
         * minFreq depends on what is the maximum duration that the signal properties can be unchanged.
         * This is given by signalPropHoldDist.max()
         * maxFreq is sineSamplingFreq / minSineCycles
         */
        param2 = std::min(param2, sineSamplingFreq / minSineCycles);
        param2 = std::max(param2, sineSamplingFreq / (double) signalPropHoldDist.max());

        //param1 is offset and param3 is amplitude. 
        /* Make sure that the maximum value of the sinusoid (i.e., param1+param3) 
         * does not go above maxVal, and the minimum value of the sinusoid 
         * (i.e., param1 - param3) does not go below minVal).
         * Adjust param3 accordingly to enforce this.
         */
        if (param1 + param3 > maxVal && param1 - param3 < minVal) {
            param3 = std::min(maxVal - param1, param1 - minVal);
        } else if (param1 + param3 > maxVal) {
            param3 = maxVal - param1;
        } else if (param1 - param3 < minVal) {
            param3 = param1 - minVal;
        }
    } else if (sigType == SignalType::Uniform) {
        //param2 is the upper limit for a uniform distribution.
        //Make sure that minVal < param1 < param2 < maxVal
        param2 = std::max(param2, param1);
        param2 = std::min(param2, maxVal);
        if (param2 == param1) {
            param1 = minVal;
            param2 = maxVal;
        }
    }
}

double SignalGenerator::getSignalValue() {
    double newValue = 0.0;
    if (sigType == SignalType::Normal) {
#ifdef DEBUG
        std::cout << "Sampling Normal dist with " << normalDist.mean() << "  " << normalDist.stddev() << std::endl;
#endif
        newValue = normalDist(randomGen);
#ifdef DEBUG
        std::cout << "Returning Normal " << newValue << std::endl;
#endif
    } else if (sigType == SignalType::Sine || sigType == SignalType::GaussSine) {
        newValue = param1 + (param3 * sin(2.0 * M_PI * param2 * time));
        time = time + (1.0 / sineSamplingFreq);
        if (sigType == SignalType::GaussSine) {
            newValue += normalDist(randomGen);
        }
    } else if (sigType == SignalType::Uniform) {
        newValue = uniformDist(randomGen);
    }
    //Ensure minVal < newValue < maxVal
    newValue = std::min(newValue, maxVal);
    newValue = std::max(newValue, minVal);

    return newValue;
}

void SignalGenerator::enableRandomizedParam(Param p, std::pair<double, double> range) {
    switch (p) {
        case Param::One:
            randomizeParam1 = true;
            break;
        case Param::Two:
            randomizeParam2 = true;
            break;
        case Param::Three:
            randomizeParam3 = true;
            break;
        case Param::Four:
            randomizeParam4 = true;

            break;
    }
    setParamRange(p, range);
}

void SignalGenerator::setParamRange(Param p, std::pair<double, double> range) {
    std::pair<double, double> newRange = sanitizeParamRanges(p, range);

#ifdef DEBUG
    std::cout << "Setting range " << newRange.first << " " << newRange.second;
#endif
    if (p == Param::One) {
#ifdef DEBUG
        std::cout << " for param1" << std::endl;
#endif
        param1Dist.param(std::uniform_real_distribution<>::param_type(newRange.first, newRange.second));
    } else if (p == Param::Two) {
#ifdef DEBUG
        std::cout << " for param2" << std::endl;
#endif
        param2Dist.param(std::uniform_real_distribution<>::param_type(newRange.first, newRange.second));
    } else if (p == Param::Three) {
#ifdef DEBUG
        std::cout << " for param3" << std::endl;
#endif
        param3Dist.param(std::uniform_real_distribution<>::param_type(newRange.first, newRange.second));
    } else if (p == Param::Four) {
#ifdef DEBUG
        std::cout << " for param4" << std::endl;
#endif
        param4Dist.param(std::uniform_real_distribution<>::param_type(newRange.first, newRange.second));
    }
}

std::pair<double, double> SignalGenerator::sanitizeParamRanges(Param p, std::pair<double, double> range) {
    auto range_min = range.first, range_max = range.second;

    //basic check: range_min < range_max
    if (range_min > range_max) {
        std::cout << "Min " << range_min << " should be smaller than Max " << range_max << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (p == Param::One || (p == Param::Two && sigType == SignalType::Uniform) || p == Param::Three) {
        /* In this case, p is either:
         * Param One: {min value for uniform dist, mean of normal dist, offset of sinusoid (in Sine or GaussSine)} or
         * Param Two in Uniform: {max value of uniform dist} or
         * Param Three: {amplitude of sinusoid (Sine or GaussSine)
         */
        //The condition to check is: minVal < range_min,range_max < maxVal
        range_min = std::max(range_min, minVal);
        range_min = std::min(range_min, maxVal);

        range_max = std::max(range_max, minVal);
        range_max = std::min(range_max, maxVal);
    } else if (p == Param::Two && (sigType == SignalType::Sine || sigType == SignalType::GaussSine)) {
        //In this case, p is Param Two i.e., the frequency parameter in Sine or GaussSine
        //condition to check is: minFreq < range_min,range_max < maxFreq

        auto minFreq = sineSamplingFreq / (double) signalPropHoldDist.max();
        auto maxFreq = sineSamplingFreq / minSineCycles;
        range_min = std::max(range_min, minFreq);
        range_min = std::min(range_min, maxFreq);

        range_max = std::max(range_max, minFreq);
        range_max = std::min(range_max, maxFreq);
    }

    //return sanitized ranges
    return std::make_pair(range_min, range_max);
}

void SignalGenerator::selectNewValForParam(Param p) {
    double val;
    if (p == Param::One) {
        val = param1Dist(randomGen);
    } else if (p == Param::Two) {
        val = param2Dist(randomGen);
    } else if (p == Param::Three) {
        val = param3Dist(randomGen);
    } else if (p == Param::Four) {
        val = param4Dist(randomGen);
    }
    setParam(p, val);
}

void SignalGenerator::setParam(Param p, double val) {
    if (p == Param::One) {
        param1 = val;
    } else if (p == Param::Two) {
        param2 = val;
    } else if (p == Param::Three) {
        param3 = val;
    } else if (p == Param::Four) {
        param4 = val;
    }
    sanitizeParamValues();

    //Adjust the signal distributions
    if (sigType == SignalType::Normal) {
        normalDist.param(std::normal_distribution<double>::param_type(param1, param2));
    } else if (sigType == SignalType::GaussSine) {
        normalDist.param(std::normal_distribution<double>::param_type(normalDist.mean(), param4));
    } else if (sigType == SignalType::Uniform) {
        uniformDist.param(std::uniform_real_distribution<>::param_type(param1, param2));
    }
}

std::pair<double, double> SignalGenerator::getParamRange(Param p) {
    if (p == Param::One) {
        if (randomizeParam1) {
            return std::make_pair(param1Dist.min(), param1Dist.max());
        } else {
            return std::make_pair(param1, param1);
        }
    } else if (p == Param::Two) {
        if (randomizeParam2) {
            return std::make_pair(param2Dist.min(), param2Dist.max());
        } else {
            return std::make_pair(param2, param2);
        }
    } else if (p == Param::Three) {
        if (randomizeParam3) {
            return std::make_pair(param3Dist.min(), param3Dist.max());
        } else {
            return std::make_pair(param3, param3);
        }
    } else if (p == Param::Four) {
        if (randomizeParam4) {
            return std::make_pair(param4Dist.min(), param4Dist.max());
        } else {
            return std::make_pair(param4, param4);
        }
    }
}

Vector SignalGenerator::getParams() {
    return Vector({param1, param2, param3, param4});
}

MaskGenerator::MaskGenerator(std::string name, std::string dirPath, std::string fileName, uint32_t smplInt,
        SignalType sigType, bool randomMProp) :
Planner(name, dirPath, fileName, smplInt),
signalType(sigType),
randomizeMaskProps(randomMProp),
maskPropHoldCounter(0),
maskPropHoldPeriod(0) {
    //for a Uniform mask, a new target is not chosen at every invocation because, a given 
    //target is held constant for a period of time. This is a piecewise constant target, 
    //and not a uniformly random target despite its name. So, a new value is sampled 
    //only at maskPropHoldPeriods. Note that when randomizing the parameters for this signal, 
    //the parameters are additionally updated at this period.
    //To get a uniformly random mask, simply make the SignalType::Uniform change at 
    //every invocation instead of only at maskPropHoldPeriods.
    if (randomizeMaskProps || signalType == SignalType::Uniform) {
        maskPropHoldPeriod = signalPropHoldDist(randomGen);
    }

#ifdef DEBUG
    std::cout << " Init maskPropHoldPeriod " << maskPropHoldPeriod << std::endl;
#endif
    auto numOutputs = maxLimits.size();
    for (auto i = 0; i < numOutputs; i++) {
        std::shared_ptr<SignalGenerator> signalDist;
        if (signalType == SignalType::Normal) {
            //Normal (min,max, p1:mean,p2:std,_,_)
            //initial stddev of the normal dist is chosen as 1/6 of the range
            signalDist = std::make_shared<SignalGenerator>(signalType, minLimits[i], maxLimits[i], targets[i], (maxLimits[i] - minLimits[i]) / 6, 0, 0);
        } else if (signalType == SignalType::Sine || signalType == SignalType::GaussSine) {
            //GaussSine (min,max, p1:offset,p2:frequency,p3:amplitude,p4: normal_stddev); p4 is ignored if we are only suing sine
            //init freq is sampling freq/5, init amp is 1/6 of range, init normal's stddev is 1/6 of the range
            signalDist = std::make_shared<SignalGenerator>(signalType, minLimits[i], maxLimits[i], targets[i], 1000 / (5 * periodInSamples * samplingIntervalMS), (maxLimits[i] - minLimits[i]) / 6, (maxLimits[i] - minLimits[i]) / 6);
        } else if (signalType == SignalType::Uniform) {
            //Uniform(min,max,p1:min, p2:max,_,_)
            signalDist = std::make_shared<SignalGenerator>(signalType, minLimits[i], maxLimits[i], minLimits[i], maxLimits[i], 0, 0);
        }

        if (randomizeMaskProps) {
            //param one can span the full range for all signals
            signalDist->enableRandomizedParam(Param::One, std::make_pair(minLimits[i], maxLimits[i]));
            if (signalType == SignalType::Normal) {
                signalDist->enableRandomizedParam(Param::Two, std::make_pair(0, (maxLimits[i] - minLimits[i]) / 6));
            } else if (signalType == SignalType::Sine || signalType == SignalType::GaussSine) {
                //sinusoid's freq can span minFreq (samplingFreq/maskPropHoldPeriod) to maxFreq (samplingFreq/4)
                signalDist->enableRandomizedParam(Param::Two, std::make_pair(1000 / (maskPropHoldPeriod * periodInSamples * samplingIntervalMS), 1000 / (4 * periodInSamples * samplingIntervalMS)));
                //amp can span full range
                signalDist->enableRandomizedParam(Param::Three, std::make_pair(minLimits[i], maxLimits[i]));
                //normal's stddev can span some range.
                signalDist->enableRandomizedParam(Param::Four, std::make_pair(0, (maxLimits[i] - minLimits[i]) / 6));
            }
        }
        signalDists.push_back(std::move(signalDist));
    }
}

bool MaskGenerator::shouldMaskPropChange() {
    if (randomizeMaskProps) {
        if (maskPropHoldCounter == maskPropHoldPeriod) {
            maskPropHoldCounter = 0;
            return true;
        } else {
            maskPropHoldCounter++;
        }
    }
    return false;
}

Vector MaskGenerator::computeNewTargets(bool run) {
#ifdef DEBUG
    std::cout << "---------RandomPlanner: " << name << "---------" << std::endl;
#endif

    auto numOutputs = targets.size();
    outputs = currOutputVals->updateValuesFromPort();

    //Use this if you want piecewise uniformly constant, and remove if you need uniformly random
    if (signalType == SignalType::Uniform) {
        if (maskPropHoldCounter == maskPropHoldPeriod) {
            maskPropHoldCounter = 0;
            maskPropHoldPeriod = signalPropHoldDist(randomGen);
            run = true;
        } else {
            run = false;
            maskPropHoldCounter++;
        }
    }

    if (run) {
        Vector newTargets(numOutputs);
        bool getNewProps = shouldMaskPropChange();
        if (getNewProps) {
            maskPropHoldPeriod = signalPropHoldDist(randomGen);
#ifdef DEBUG
            std::cout << "Creating new mask properties for period " << maskPropHoldPeriod << std::endl;
#endif
        }

        for (auto i = 0; i < numOutputs; i++) {
            auto signalDist = signalDists[i];
            if (getNewProps) {
                signalDist->selectNewValForParam(Param::One);
                signalDist->selectNewValForParam(Param::Two);
                signalDist->selectNewValForParam(Param::Three);
                signalDist->selectNewValForParam(Param::Four);
            }

            newTargets[i] = signalDist->getSignalValue();
        }
#ifdef DEBUG
        std::cout << "currOps " << outputs << "currTargets " << targets << "newTargets " << newTargets;
#endif
        targets = newTargets;
        return newTargets;
    }
    
#ifdef DEBUG
    std::cout << "Skipping" << std::endl;
#endif
    return targets;
}

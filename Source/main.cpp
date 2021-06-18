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
 * File:   main.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#include "Inputs.h"
#include "Abstractions.h"
#include "Sensors.h"
#include "Manager.h"
#include "debug.h"
#include <iostream>
#include <vector>
#include <map>
#include <sys/time.h>
#include <sstream>

std::map<std::string, std::string> parseArgs(int argc, char **argv) {
    std::map<std::string, std::string> args;
    std::vector<std::string> words(argv + 1, argv + argc);//gather all words as a vector of strings
    std::string word, arg_name, arg_val;
    bool prevWordIsArgName = false, error = false;
    
    //find the arg_name, arg_val pairs
    for (auto i = 0; i < words.size(); i++) {
        word = words[i];
        //if we detect a keyword
        if (word.compare(0,2,"--") == 0) {
            //check if prev word is also keyword - error
            if (prevWordIsArgName) {
                error = true;
                break;
            }
            prevWordIsArgName = true;
            arg_name = word.substr(2);
        } else {
            //if we detect regular word, the previous must be keyword or the arg_name must be idips
            if (prevWordIsArgName) {
                arg_val = word;
                prevWordIsArgName = false;
                args[arg_name] = arg_val;
            } else if (arg_name.compare("idips") == 0) {
                arg_val = arg_val + " " + word;
                args[arg_name] = arg_val;
            } else {
                error = true;
                break;
            }
        }
    }
    if (error) {
        std::cout << "Usage: " << argv[0] <<
                " --mode <Mode> [--idips <Sysid inputs>][--mask <mask name> --ctldir <dir> --ctlfile <fileprefix>]"
                << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return args;
}

Mode getMode(std::map<std::string, std::string> args) {
    if (args.find("mode") == args.end()) {
        std::cout << "No --mode specified. --mode should be one of Baseline, Sysid, Mask" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::string modeName = args["mode"];
#ifdef DEBUG
    std::cout << "Mode is " << modeName << std::endl;
#endif
    if (modeName.compare("Baseline") == 0) {
        return Mode::Baseline;
    } else if (modeName.compare("Sysid") == 0) {
        return Mode::Sysid;
    } else if (modeName.compare("Mask") == 0) {
        return Mode::Mask;
    } else {
        std::cout << "Mode " << modeName << " is invalid. It should be one of Baseline, Sysid, Mask" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

std::vector<std::string> getSysidNames(std::map<std::string, std::string> args) {
    if (args.find("idips") == args.end()) {
        std::cout << "No --idips specified. --idips should have a list of input names" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::istringstream idips(args["idips"]);
    std::string idip;
    std::vector<std::string> idNames;
    while (idips >> idip) {
        idNames.push_back(idip);
    }
    return idNames;
}

MaskGenType getMaskType(std::map<std::string, std::string> args) {
    if (args.find("mask") == args.end()) {
        std::cout << "No --mask specified. --mask should be one of Constant, Uniform, Gauss, GaussSine, Sine, Preset" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::string maskName(args["mask"]);
#ifdef DEBUG
    std::cout << "Mask type is " << maskName << std::endl;
#endif
    if (maskName.compare("Constant") == 0) {
        return MaskGenType::Constant;
    } else if (maskName.compare("Uniform") == 0) {
        return MaskGenType::Uniform;
    } else if (maskName.compare("Gauss") == 0) {
        return MaskGenType::Gauss;
    } else if (maskName.compare("GaussSine") == 0) {
        return MaskGenType::GaussSine;
    } else if (maskName.compare("Sine") == 0) {
        return MaskGenType::Sine;
    } else if (maskName.compare("Preset") == 0) {
        return MaskGenType::Preset;
    } else {
        std::cout << "Mask name " << maskName << " is invalid. It should be one of Constant, Uniform, Gauss, GaussSine, Sine, Preset" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

std::string getCtlDir(std::map<std::string, std::string> args) {
    if (args.find("ctldir") == args.end()) {
        std::cout << "No --ctldir specified." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    #ifdef DEBUG
    std::cout << "controller file directory is " << args["ctldir"] << std::endl;
#endif
    return args["ctldir"];
}

std::string getCtlFilePrefix(std::map<std::string, std::string> args) {
    if (args.find("ctlfile") == args.end()) {
        std::cout << "No --ctlfile specified." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    #ifdef DEBUG
    std::cout << "controller file prefix is " << args["ctlfile"] << std::endl;
#endif
    return args["ctlfile"];
}
    
const uint32_t samplingIntervalMS = 20; //20 is default

//Usage: ./maya --mode <Mode> [--idips <Sysid inputs>][--mask <mask name> --ctldir <dir> --ctlfile <file prefix>]

int main(int argc, char** argv) {
    auto args = parseArgs(argc, argv);
    auto mode = getMode(args);

    //random seed
    struct timeval time;
    gettimeofday(&time, NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    //Create manager
    Manager manager(samplingIntervalMS, mode);
 
    //add sensors
    manager.addSensor(std::make_unique<Time>("Time"));
    manager.addSensor(std::make_unique<CPUPowerSensor>("CPUPower"));

    //add inputs
    manager.addInput(std::make_unique<CPUFrequency>("CPUFreq"));
    manager.addInput(std::make_unique<IdleInject>("IdlePct"));
    manager.addInput(std::make_unique<PowerBalloon>("PBalloon"));

    if (mode == Mode::Sysid) {
        manager.addSysIdParams(getSysidNames(args));
    } else if (mode == Mode::Mask) {
        std::string ctlFileName(getCtlFilePrefix(args));
        std::string dirPath(getCtlDir(args));
        auto maskName = getMaskType(args);
        uint32_t ctlPeriod = 1; //invoke controller at every sampling interval
        uint32_t maskGenPeriod = 3; //invoke mask gen for every 3 invocations of the controller so that controller can converge.
        manager.addController("MayaController",{"CPUPower"},
        {
            "CPUFreq", "IdlePct", "PBalloon"
        },
        ControllerType::SSV, dirPath, ctlFileName, ctlPeriod);

        if (maskName == MaskGenType::Uniform) {
            manager.addMaskGenerator("MayaMaskGenerator", "MayaController", maskName, 
                    dirPath, ctlFileName,  maskGenPeriod * ctlPeriod, false);
        } else {
            manager.addMaskGenerator("MayaMaskGenerator", "MayaController", maskName, 
                    dirPath, ctlFileName,  maskGenPeriod * ctlPeriod, true);
        }
    }

    manager.run();
    return 0;
}


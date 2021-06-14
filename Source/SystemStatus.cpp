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
 * File:   SystemStatus.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#include "SystemStatus.h"
#include <algorithm>
#include <iostream>
#include <fstream>

SystemStatus::SystemStatus(std::string name, SystemType systemType, uint32_t totalUnits) :
name(name),
systemType(systemType),
totalUnits(totalUnits) {
    unitStatus = std::vector<bool>(totalUnits);
    updatePhysicalUnitInfo();
}

SystemStatus::SystemStatus(std::string name, SystemType systemType) :
name(name),
systemType(systemType) {

}

void SystemStatus::setTotalUnits(uint32_t numTotalUnits) {
    totalUnits = numTotalUnits;
    unitStatus = std::vector<bool>(totalUnits);
    for (uint32_t id = 0; id < numTotalUnits; id++) {
        unitIds.push_back(id);
    }
    updatePhysicalUnitInfo();
}

void SystemStatus::updatePhysicalUnitInfo() {
    std::ifstream file;
    std::string list, fileName;
    if (systemType == SystemType::CPU) {
        for (auto& unitId : unitIds) {
            std::vector<uint32_t> siblings;

            fileName = "/sys/devices/system/cpu/cpu" +
                    std::to_string(unitId) + "/topology/thread_siblings_list";
            file.open(fileName);
            file >> list;
            file.close();

            std::string delim = ",";
            auto start = 0;
            auto end = list.find(delim);
            while (end != std::string::npos) {
                siblings.push_back(std::stoul(list.substr(start, end - start)));
                start = end + delim.length();
                end = list.find(delim, start);
            }
            siblings.push_back(std::stoul(list.substr(start, end - start)));

#ifdef DEBUG
            std::cout << "SMT Siblings for core " << unitId << " from system is " << list << ":";
            for (auto& sibling : siblings) {
                std::cout << " " << sibling;
            }
            std::cout << std::endl;
#endif

            auto minSibling = *std::min_element(siblings.begin(), siblings.end());
            if (std::find(physicalUnitIds.begin(), physicalUnitIds.end(), minSibling) == physicalUnitIds.end()) {
                physicalUnitIds.push_back(minSibling);
            }
        }
#ifdef DEBUG
        std::cout << "Physical core ids are:";
        for (auto& sibling : physicalUnitIds) {
            std::cout << " " << sibling;
        }
        std::cout << std::endl;
#endif
    }
}

void SystemStatus::setUnitStatus(uint32_t unitId, bool newStatus) {
    if (unitStatus[unitId] != newStatus) {
        unitStatus[unitId] = newStatus;
        if (newStatus == true) {
            totalActiveUnits++;
        } else {
            totalActiveUnits--;
        }
    }
}

void SystemStatus::setUnitStatus(std::vector<bool> newStatus) {
    std::copy(newStatus.begin(), newStatus.end(), unitStatus.begin());
    totalActiveUnits = std::count(unitStatus.begin(), unitStatus.end(), true);
}

void SystemStatus::print() {
    std::cout << name << " ( " << totalActiveUnits << "/" << totalUnits << " ) : ";
    for (auto&& status : unitStatus) {
        std::cout << status;
    }
    std::cout << std::endl;
}

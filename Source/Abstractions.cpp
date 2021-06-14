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
 * File:   Abstractions.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#include "Abstractions.h"
//#include "Inputs.h"
#include "debug.h"

Pin::Pin() : Pin("Empty", -1.0) {
}

Pin::Pin(std::string name_) : Pin(name_, 0.0) {

}

Pin::Pin(std::string name_, double value) : name(name_), value(value), connected(false) {
}

auto Pin::getName() {
    return name;
}

auto Pin::getValue() {
    valueUnread = false;
    return value;
}

auto Pin::isConnected() {
    return connected;
}

auto Pin::isValueUnread() {
    return valueUnread;
}

void Pin::setName(std::string name_) {
    name = name_;
}

void Pin::setValue(double value_) {
    value = value_;
    valueUnread = true;
}

void Pin::setConnected() {
    connected = true;
}

Port::Port(std::string portName_) : Port(portName_,{}) {

}

Port::Port(std::string portName, std::initializer_list<std::string> pinNames) :
portName(portName) {
    for (auto& pinName : pinNames) {
        pins.push_back(std::make_unique<Pin>(pinName));
    }
#ifdef DEBUG
    std::cout << "port width " << pins.size() << std::endl;
#endif
}

auto Port::getName() {
    return portName;
}

std::vector<std::string> Port::getPinNames() {
    std::vector<std::string> pinNames;
    for (auto& pin : pins) {
        pinNames.push_back(pin->getName());
    }
    return pinNames;
}

auto Port::getPinName(uint32_t pinNum) {
    sanitizePinNum(pinNum);
    return pins[pinNum]->getName();
}

auto Port::getPinNum(std::string pinName) {
    uint32_t pinNum = 0;
    for (auto& pin : pins) {
        if (pinName.compare(pin->getName()) == 0) {
            return pinNum;
        }
        pinNum++;
    }
    std::cout << "Pin with name " << pinName << " does not exist in Port " <<
            portName << std::endl;
    std::exit(EXIT_FAILURE);
}

auto Port::getNumPins() {
    return pins.size();
}

void Port::sanitizePinNum(uint32_t pinNum) {
    if (pinNum < 0 || pinNum > pins.size() - 1) {
        std::cout << "Start pin must be > 0 and end pin " << "must be < " <<
                pins.size() - 1 << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void Port::sanitizePinNums(std::vector<uint32_t> pinNumList) {
    if (pinNumList.size() > pins.size()) {
        std::cout << "Too many pins specified." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    for (auto pinNum : pinNumList) {
        sanitizePinNum(pinNum);
    }
}

void Port::addPin(std::string pinName) {
    pins.push_back(std::make_unique<Pin>(pinName));
}

void Port::addPin(std::vector<std::string> pinNames) {
    for (auto& pinName : pinNames) {
        pins.push_back(std::make_unique<Pin>(pinName));
    }
}

OutputPort::OutputPort(std::string name) : Port(name) {

}

OutputPort::OutputPort(std::string name, std::initializer_list<std::string> pinNames) :
Port(name, pinNames) {

}

Vector OutputPort::transmitValues(std::vector<uint32_t> pinNums) {
    sanitizePinNums(pinNums);
    uint32_t i = 0;
    Vector result(pinNums.size());
    for (auto pinNum : pinNums) {
        result[i] = pins[pinNum]->getValue();
        i++;
    }
    return result;
}

Vector OutputPort::transmitValues(std::vector<std::string> pinNames) {
    uint32_t i = 0;
    Vector result(pinNames.size());
    for (auto& pinName : pinNames) {
        result[i] = pins[getPinNum(pinName)]->getValue();
        i++;
    }
    return result;
}

Vector OutputPort::transmitValues() {
    return transmitValues(getPinNames());
}

void OutputPort::updateValuesToPort(Vector newValues) {
    if (newValues.size() != pins.size()) {
        std::cout << "Values to be updated must match available pins." << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        for (uint32_t i = 0; i < newValues.size(); i++) {
            pins[i]->setValue(newValues[i]);
        }
    }
}

void OutputPort::setConnected(std::vector<uint32_t> pinNums) {
    sanitizePinNums(pinNums);
    for (auto pinNum : pinNums) {
        if (pins[pinNum]->isConnected() == false) {
            pins[pinNum]->setConnected();
        }
    }
}

InputPort::InputPort(std::string name) : Port(name) {

}

InputPort::InputPort(std::string name, std::initializer_list<std::string> pinNames) :
Port(name, pinNames) {

}

void InputPort::setConnected(std::vector<uint32_t> pinNums) {
    sanitizePinNums(pinNums);
    for (auto pinNum : pinNums) {
        if (pins[pinNum]->isConnected() == false) {
            pins[pinNum]->setConnected();
        } else {
            std::cout << portName << "[" << pins[pinNum]->getName() <<
                    "] is already connected!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}

Vector InputPort::updateValuesFromPort() {
    uint32_t i = 0;
    Vector result(pins.size());
    for (auto& pin : pins) {
        result[i] = pin->getValue();
        i++;
    }
    return result;

}

void InputPort::receiveValues(std::vector<uint32_t> pinNums, Vector newValues) {
    sanitizePinNums(pinNums);
    uint32_t i = 0;
    for (auto pinNum : pinNums) {
        pins[pinNum]->setValue(newValues[i]);
        i++;
    }
}

void InputPort::receiveValues(std::vector<std::string> pinNames, Vector newValues) {
    std::vector<uint32_t> pinNums;
    for (auto pinName : pinNames) {
        pinNums.push_back(getPinNum(pinName));
    }
    receiveValues(pinNums, newValues);
}

void InputPort::receiveValues(Vector newValues) {
    receiveValues(getPinNames(), newValues);
}

bool InputPort::areValuesUnread() {
    for (auto& pin : pins) {
        if (pin->isValueUnread() == true) {
            return true;
        }
    }
    return false;
}

Wire::Wire(std::shared_ptr<OutputPort> src, std::shared_ptr<InputPort> dst, uint32_t dly) :
Wire(src, 0, src->getNumPins() - 1, dst, 0, dst->getNumPins() - 1, dly) {

}

Wire::Wire(std::shared_ptr<OutputPort> src, uint32_t srcPinNum,
        std::shared_ptr<InputPort> dst, uint32_t destPinNum, uint32_t dly) :
Wire(src, srcPinNum, srcPinNum, dst, destPinNum, destPinNum, dly) {

}

Wire::Wire(std::shared_ptr<OutputPort> src, uint32_t sPinNumBegin, uint32_t sPinNumEnd,
        std::shared_ptr<InputPort> dst, uint32_t dPinNumBegin, uint32_t dPinNumEnd, uint32_t dly) :
srcPort(src),
destPort(dst),
delay(dly) {
#ifdef DEBUG
    std::cout << "Connecting " << srcPort->getName() << ": " << sPinNumBegin << "-" << sPinNumEnd << " with " <<
            destPort->getName() << ":" << dPinNumBegin << "-" << dPinNumEnd << std::endl;
#endif
    auto destWidth = dPinNumEnd - dPinNumBegin + 1;
    auto srcWidth = sPinNumEnd - sPinNumBegin + 1;

    if (destWidth != srcWidth) {
        std::cout << "Destination port " << dst->getName() << " of width " << destWidth
                << " does not match source port " << src->getName() << " of width " << srcWidth << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        for (uint32_t pinNum = sPinNumBegin; pinNum <= sPinNumEnd; pinNum++) {
            srcPinNumList.push_back(pinNum);
        }
        for (uint32_t pinNum = dPinNumBegin; pinNum <= dPinNumEnd; pinNum++) {
            destPinNumList.push_back(pinNum);
        }
        PortInterfaceForWire::callSetConnected(srcPort, srcPinNumList);
        PortInterfaceForWire::callSetConnected(destPort, destPinNumList);
    }
}

Wire::Wire(std::shared_ptr<OutputPort> src, std::initializer_list<uint32_t> sPinNumList,
        std::shared_ptr<InputPort> dst, std::initializer_list<uint32_t> dPinNumList, uint32_t dly) :
Wire(src, std::vector<uint32_t>(sPinNumList), dst, std::vector<uint32_t>(dPinNumList), dly) {
}

Wire::Wire(std::shared_ptr<OutputPort> src, std::vector<uint32_t> sPinNumList,
        std::shared_ptr<InputPort> dst, std::vector<uint32_t> dPinNumList, uint32_t dly) :
srcPort(src),
destPort(dst),
srcPinNumList(sPinNumList),
destPinNumList(dPinNumList),
delay(dly) {
    auto destWidth = dPinNumList.size();
    auto srcWidth = sPinNumList.size();

    if (destWidth != srcWidth) {
        std::cout << "Destination connection " << dst->getName() << " of width " << destWidth
                << "does not match source connection " << src->getName() << " of width " << srcWidth << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        PortInterfaceForWire::callSetConnected(srcPort, srcPinNumList);
        PortInterfaceForWire::callSetConnected(destPort, destPinNumList);
    }
}

Wire::Wire(std::shared_ptr<OutputPort> src, std::string srcPinName,
        std::shared_ptr<InputPort> dst, std::string dstPinName, uint32_t dly) :
Wire(src,{srcPinName}, dst,{dstPinName}, dly) {

}

Wire::Wire(std::shared_ptr<OutputPort> src, std::initializer_list<std::string> srcPinNames,
        std::shared_ptr<InputPort> dst, std::initializer_list<std::string> dstPinNames, uint32_t dly) :
Wire(src, std::vector<std::string>(srcPinNames), dst, std::vector<std::string>(dstPinNames), dly) {

}

Wire::Wire(std::shared_ptr<OutputPort> src, std::vector<std::string> srcPinNames,
        std::shared_ptr<InputPort> dst, std::vector<std::string> dstPinNames, uint32_t dly) :
srcPort(src),
destPort(dst),
delay(dly) {
    auto destWidth = dstPinNames.size();
    auto srcWidth = srcPinNames.size();

    if (destWidth != srcWidth) {
        std::cout << "Destination connection " << dst->getName() << " of width " << destWidth
                << "does not match source connection " << src->getName() << " of width " << srcWidth << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        for (auto& srcPinName : srcPinNames) {
            srcPinNumList.push_back(srcPort->getPinNum(srcPinName));
        }
        for (auto& dstPinName : dstPinNames) {
            destPinNumList.push_back(destPort->getPinNum(dstPinName));
        }
        PortInterfaceForWire::callSetConnected(srcPort, srcPinNumList);
        PortInterfaceForWire::callSetConnected(destPort, destPinNumList);
    }
}

void Wire::transfer() {
    if (cycles == delay) {
        cycles = 0;
        destPort->receiveValues(destPinNumList, srcPort->transmitValues(srcPinNumList));
    } else {
        cycles++;
    }
}



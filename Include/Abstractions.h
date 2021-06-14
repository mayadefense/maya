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
 * File:   Abstractions.h
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

/*
 * Each module (controller, planner, sensor or input) has a port to send and/or 
 * receive values. Each port can have multiple pins, and each pin has one value.
 * A wire connects ports (or some pins between ports) between modules.
 * Sensors read values from the system and send it out through a read port. 
 * Inputs accept values to be written to the system via a write port.
 * 
 */ 

#ifndef ABSTRACTIONS_H
#define ABSTRACTIONS_H

#include <string>
#include <vector>
#include <chrono>
#include <memory>

#include "MathSupport.h"

/*enum class Mach {
    Tarekc,
    Iacoma,
    MM
};*/


class Pin {
public:
    Pin();
    Pin(std::string name);
    Pin(std::string name, double value);

    auto getName();
    auto getValue();
    auto isConnected();
    auto isValueUnread();

    void setConnected();
    void setName(std::string name_);
    void setValue(double value_);

private:
    std::string name;
    double value;
    bool connected, valueUnread;
};

class Port {
public:
    Port(std::string portName, std::initializer_list<std::string> pinNames);
    Port(std::string portName);

    auto getName();
    std::vector<std::string> getPinNames();
    auto getPinName(uint32_t pinNum);
    auto getPinNum(std::string pinName);
    auto getNumPins();

    void addPin(std::string pinName);
    void addPin(std::vector<std::string> pinNames);

protected:
    void sanitizePinNum(uint32_t pinNum);
    void sanitizePinNums(std::vector<uint32_t> pinNumList);

    //changed from public to protected - give access to wire only.
    virtual void setConnected(std::vector<uint32_t> pinNums) = 0;
    friend class PortInterfaceForWire;

    std::string portName;
    std::vector<std::unique_ptr<Pin>> pins;
};

class PortInterfaceForWire {
private:

    static void callSetConnected(std::shared_ptr<Port> p, std::vector<uint32_t> pinNums) {
        p->setConnected(pinNums);
    }
    friend class Wire;
};

class OutputPort : public Port {
public:
    OutputPort(std::string name, std::initializer_list<std::string> pinNames);
    OutputPort(std::string name);
    //port->outside
    Vector transmitValues();
    Vector transmitValues(std::vector<uint32_t> pinNums);
    Vector transmitValues(std::vector<std::string> pinNames);

    //module->port
    void updateValuesToPort(Vector newValues); // local->port      
protected:
    void setConnected(std::vector<uint32_t> pinNums) override;
};

class InputPort : public Port {
public:
    InputPort(std::string name, std::initializer_list<std::string> pinNames);
    InputPort(std::string name);
    //port->module
    Vector updateValuesFromPort(); //port->local

    //outside->port
    void receiveValues(std::vector<uint32_t> pinNums, Vector newValues);
    void receiveValues(std::vector<std::string> pinNames, Vector newValues);
    void receiveValues(Vector newValues);

    bool areValuesUnread();
protected:
    void setConnected(std::vector<uint32_t> pinNums) override;

private:
    bool portValuesUnread;
};

class Wire {
public:
    Wire(std::shared_ptr<OutputPort> src, std::shared_ptr<InputPort> dst, uint32_t dly = 0);

    Wire(std::shared_ptr<OutputPort> src, uint32_t srcPinNum,
            std::shared_ptr<InputPort> dst, uint32_t destPinNum, uint32_t dly = 0);
    Wire(std::shared_ptr<OutputPort> src, uint32_t srcPinNumBegin, uint32_t srcPinNumEnd,
            std::shared_ptr<InputPort> dst, uint32_t destPinNumBegin, uint32_t destPinNumEnd, uint32_t dly = 0);
    Wire(std::shared_ptr<OutputPort> src, std::initializer_list<uint32_t> srcPinNumList,
            std::shared_ptr<InputPort> dst, std::initializer_list<uint32_t> destPinNumList, uint32_t dly = 0);
    Wire(std::shared_ptr<OutputPort> src, std::vector<uint32_t> srcPinNumList,
            std::shared_ptr<InputPort> dst, std::vector<uint32_t> destPinNumList, uint32_t dly = 0);

    Wire(std::shared_ptr<OutputPort> src, std::string srcPinName,
            std::shared_ptr<InputPort> dst, std::string dstPinName, uint32_t dly = 0);
    Wire(std::shared_ptr<OutputPort> src, std::initializer_list<std::string> srcPinNames,
            std::shared_ptr<InputPort> dst, std::initializer_list<std::string> dstPinNames, uint32_t dly = 0);
    Wire(std::shared_ptr<OutputPort> src, std::vector<std::string> srcPinNames,
            std::shared_ptr<InputPort> dst, std::vector<std::string> dstPinNames, uint32_t dly = 0);
    void transfer();
private:

    std::shared_ptr<OutputPort> srcPort;
    std::shared_ptr<InputPort> destPort;
    std::vector<uint32_t> srcPinNumList, destPinNumList;
    uint32_t delay, cycles = 0;
};



#endif /* ABSTRACTIONS_H */


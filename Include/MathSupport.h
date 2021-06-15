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
 * File:   MathSupport.h
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

/*
 * Linear algebra functions for Vector arithmetic and Matrix-vector multiply.
 * Supports Vector<op>scalar too for certain operations.
 */

#ifndef MATHSUPPORT_H
#define MATHSUPPORT_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <initializer_list>
#include <algorithm>
#include <numeric>

class Vector {
public:
    typedef typename std::vector<double>::size_type size_type;
    typedef typename std::vector<double>::iterator iterator;
    typedef typename std::vector<double>::const_iterator const_iterator;
    // Empty constructor

    Vector() : _data(0.0) {
    }

    // Initializing constructor

    Vector(size_type n) : _data(n, 0) {
    }

    // Initializer constructor

    Vector(std::initializer_list<double> l) : _data(l) {
    }

    Vector(std::vector<double> l) : _data(l) {
    }

    Vector(double *initloc, size_type n): _data(n,0) {
        for (size_type i = 0; i < n; i++) {
            _data[i] = initloc[i];
        }
    }

    Vector(std::vector<uint64_t> l) {
        for (auto li : l) {
            _data.push_back((double) li);
        }
    }

    Vector(std::string filename) {
        from_file(filename);
    }

    // Copy constructor

    Vector(const Vector& v) : _data(v._data) {
    }

    // Move constructor

    Vector(Vector&& v) : _data(std::move(v._data)) {
    }

    // Copy assignment operator

    Vector& operator=(const Vector& v) {
        _data = v._data;
        return *this;
    }

    // Move assignment operator

    Vector& operator=(Vector&& v) {
        _data = std::move(v._data);
        return *this;
    }

    // Constant assignment operator

    Vector& operator=(const double& v) {
        _data.assign(_data.size(), v);
        return *this;
    }

    // Append

    Vector append(const Vector& v) {
        Vector vret(*this);
        vret._data.insert(vret.end(), v.begin(), v.end());
        return vret;
    }

    void pack(const Vector& v1, const Vector& v2) {
        _data = v1._data;
        _data.insert(_data.end(), v2.begin(), v2.end());
    }

    // Size

    size_type size() const {
        return _data.size();
    }

    // Indexing

    double& operator[](size_type idx) {
        return _data[idx];
    }

    const double& operator[](size_type idx) const {
        return _data[idx];
    }

    // Iterators

    iterator begin() {
        return _data.begin();
    }

    const_iterator begin() const {
        return _data.begin();
    }

    iterator end() {
        return _data.end();
    }

    const_iterator end() const {
        return _data.end();
    }

    // Load values

    void from_string(std::string vals) {
        _data.clear();
        std::istringstream ss(vals);
        double tmp;
        while (ss >> tmp) {
            _data.push_back(tmp);
        }
    }

    void from_file(std::string filename) {
        std::ifstream fs(filename);
        if (!fs) {
            std::cerr << "Unable to open " << filename << std::endl;
            exit(1);
        }
        _data.clear();
        double tmp;
        while (fs >> tmp) {
            _data.push_back(tmp);
        }
        fs.close();
    }

    void generateRandom() {
        std::generate(_data.begin(), _data.end(), std::rand);
    }

    // Sorting

    std::vector<size_type> sortIndex() {
        std::vector<size_type> indices(this->size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(),
                [this] (size_type a, size_type b) {
                    return _data[a] < _data[b];
                });
        return indices;
    }

    // Formatting

    std::string format(std::string fmt, std::string sep = " ") {
        std::ostringstream output;
        std::string sp = "";
        for (auto& v : * this) {
            //                output << sp << Print::format(fmt, v);
            sp = sep;
        }
        return output.str();
    }

private:
    std::vector<double> _data;
};

class Matrix {
public:
    typedef typename std::vector<double>::size_type size_type;
    typedef typename std::vector<double>::iterator iterator;
    typedef typename std::vector<double>::const_iterator const_iterator;
    // Empty constructor

    Matrix() : _row(0), _col(0), _data(0) {
    }

    // Initializing constructor
    // Square

    Matrix(size_type r) : _row(r), _col(r), _data(r*r, 0) {
    }

    // Rectangle

    Matrix(size_type r, size_type c) : _row(r), _col(c), _data(r*c, 0) {
    }

    // Copy constructor

    Matrix(const Matrix& m) : _row(m.row()), _col(m.col()), _data(m._data) {
    }

    // Move constructor

    Matrix(Matrix&& m) : _row(m.row()), _col(m.col()), _data(std::move(m._data)) {
    }

    // Copy assignment operator

    Matrix& operator=(const Matrix& m) {
        _data = m._data;
        _row = m._row;
        _col = m._col;
        return *this;
    }

    // Move assignment operator

    Matrix& operator=(Matrix&& m) {
        _data = std::move(m._data);
        _row = m._row;
        _col = m._col;
        m._row = 0;
        m._col = 0;
        return *this;
    }

    // Size

    size_type row() const {
        return _row;
    }

    size_type col() const {
        return _col;
    }

    // Indexing

    double* operator[](size_type idx) {
        return &(_data[idx * _col]);
    }

    const double* operator[](size_type idx) const {
        return &(_data[idx * _col]);
    }

    void from_file(std::string filename) {
        std::ifstream fs(filename);
        if (!fs) {
            std::cerr << "Unable to open " << filename << std::endl;
            exit(1);
        }
        _data.clear();
        double tmp;
        for (auto i = 0; i < _row; i++) {
            for (auto j = 0; j < _col; j++) {
                fs >> tmp;
                _data.push_back(tmp);
            }
        }
        fs.close();
    }

private:
    size_type _row, _col;
    std::vector<double> _data;
};

// Operator overloads - declared Non-member Non-friend
// Vector Operators:
// Addition operator
Vector operator+(const Vector& lhs, const Vector& rhs);

// Addition with scalar operator
Vector operator+(const Vector& lhs, const double& rhs);
Vector operator+(const double& lhs, const Vector& rhs);

// doublenary Negation operator
Vector operator-(const Vector& val);

// Subtraction operator
Vector operator-(const Vector& lhs, const Vector& rhs);

// Subtraction with scalar operator
Vector operator-(const Vector& lhs, const double& rhs);
Vector operator-(const double& lhs, const Vector& rhs);

// Element by element multiplication
Vector operator*(const Vector& lhs, const Vector& rhs);

// Multiplication by scalar operator
Vector operator*(const Vector& lhs, const double& rhs);
Vector operator*(const double& lhs, const Vector& rhs);

// Element by element division
Vector operator/(const Vector& lhs, const Vector& rhs);

// Division with scalar operator
Vector operator/(const Vector& lhs, const double& rhs);
Vector operator/(const double& lhs, const Vector& rhs);


// Comparison operators
bool operator==(const Vector& lhs, const Vector& rhs);
bool operator!=(const Vector& lhs, const Vector& rhs);

bool operator<(const Vector& lhs, const Vector& rhs);
bool operator<=(const Vector& lhs, const double& rhs);
bool operator<(const Vector& lhs, const double& rhs);
bool operator>=(const Vector& lhs, const double& rhs);
bool operator>(const Vector& lhs, const double& rhs);

bool operator<=(const double& lhs, const Vector& rhs);
bool operator<(const double& lhs, const Vector& rhs);
bool operator>=(const double& lhs, const Vector& rhs);
bool operator>(const double& lhs, const Vector& rhs);

// Matrix-Vector operators:
// Matrix-vector multiplication operator
Vector operator*(const Matrix& m, const Vector& v);

// IO stream operators
std::ostream& operator<<(std::ostream& os, const Vector& v);

std::ostream& operator<<(std::ostream& os, const Matrix& m);

#endif /* MATHSUPPORT_H */


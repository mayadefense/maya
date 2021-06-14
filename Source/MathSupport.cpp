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
 * File:   MathSupport.cpp
 * Author: Raghavendra Pradyumna Pothukuchi and Sweta Yamini Pothukuchi
 */

#include "MathSupport.h"

// Operator overloads - declared Non-member Non-friend
// Vector Operators:
// Addition operator

Vector operator+(const Vector& lhs, const Vector& rhs) {
    if (lhs.size() == rhs.size()) {
        Vector v(lhs.size());
        for (auto i = 0; i < lhs.size(); i++) {
            v[i] = lhs[i] + rhs[i];
        }
        return v;
    }
    std::cerr << "Mismatched sizes for operator +" << std::endl;
    return Vector();
}

// Addition with scalar operator

Vector operator+(const Vector& lhs, const double& rhs) {
    Vector v(lhs.size());
    for (auto i = 0; i < lhs.size(); i++) {
        v[i] = lhs[i] + rhs;
    }
    return v;
}

Vector operator+(const double& lhs, const Vector& rhs) {
    Vector v(rhs.size());
    for (auto i = 0; i < rhs.size(); i++) {
        v[i] = lhs + rhs[i];
    }
    return v;
}

// Unary Negation operator

Vector operator-(const Vector& val) {
    Vector v(val.size());
    for (auto i = 0; i < val.size(); i++) {
        v[i] = -val[i];
    }
    return v;
}

// Subtraction operator

Vector operator-(const Vector& lhs, const Vector& rhs) {
    if (lhs.size() == rhs.size()) {
        Vector v(lhs.size());
        for (auto i = 0; i < lhs.size(); i++) {
            v[i] = lhs[i] - rhs[i];
        }
        return v;
    }
    std::cerr << "Mismatched sizes for operator -" << std::endl;
    return Vector();
}

// Subtraction with scalar operator

Vector operator-(const Vector& lhs, const double& rhs) {
    Vector v(lhs.size());
    for (auto i = 0; i < lhs.size(); i++) {
        v[i] = lhs[i] - rhs;
    }
    return v;
}

Vector operator-(const double& lhs, const Vector& rhs) {
    Vector v(rhs.size());
    for (auto i = 0; i < rhs.size(); i++) {
        v[i] = lhs - rhs[i];
    }
    return v;
}

// Element by element multiplication

Vector operator*(const Vector& lhs, const Vector& rhs) {
    if (lhs.size() == rhs.size()) {
        Vector v(lhs.size());
        for (auto i = 0; i < lhs.size(); i++) {
            v[i] = lhs[i] * rhs[i];
        }
        return v;
    }
    std::cerr << "Mismatched sizes for operator *" << std::endl;
    return Vector();
}

// Multiplication by scalar operator

Vector operator*(const Vector& lhs, const double& rhs) {
    Vector v(lhs.size());
    for (auto i = 0; i < lhs.size(); i++) {
        v[i] = lhs[i] * rhs;
    }
    return v;
}

Vector operator*(const double& lhs, const Vector& rhs) {
    Vector v(rhs.size());
    for (auto i = 0; i < rhs.size(); i++) {
        v[i] = lhs * rhs[i];
    }
    return v;
}

// Element by element division

Vector operator/(const Vector& lhs, const Vector& rhs) {
    if (lhs.size() == rhs.size()) {
        Vector v(lhs.size());
        for (auto i = 0; i < lhs.size(); i++) {
            v[i] = lhs[i] / rhs[i];
        }
        return v;
    }
    std::cerr << "Mismatched sizes for operator /" << std::endl;
    return Vector();
}

// Division with scalar operator

Vector operator/(const Vector& lhs, const double& rhs) {
    Vector v(lhs.size());
    for (auto i = 0; i < lhs.size(); i++) {
        v[i] = lhs[i] / rhs;
    }
    return v;
}

Vector operator/(const double& lhs, const Vector& rhs) {
    Vector v(rhs.size());
    for (auto i = 0; i < rhs.size(); i++) {
        v[i] = lhs / rhs[i];
    }
    return v;
}

// Comparison operators

bool operator==(const Vector& lhs, const Vector& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (auto i = 0; i < lhs.size(); i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

bool operator!=(const Vector& lhs, const Vector& rhs) {
    return !(lhs == rhs);
}

bool operator<(const Vector& lhs, const Vector& rhs) {
    if (lhs.size() != rhs.size()) {
        std::cerr << "Mismatched dimensions for matrix-vector product" << std::endl;
        return false;
    }
    auto i = 0;
    for (auto val : lhs) {
        if (val >= rhs[i++]) {
            return false;
        }
    }
    return true;
}

bool operator<=(const Vector& lhs, const double& rhs) {
    for (auto val : lhs) {
        if (val > rhs) {
            return false;
        }
    }
    return true;
}

bool operator<(const Vector& lhs, const double& rhs) {
    for (auto val : lhs) {
        if (val >= rhs) {
            return false;
        }
    }
    return true;
}

bool operator>=(const Vector& lhs, const double& rhs) {
    for (auto val : lhs) {
        if (val < rhs) {
            return false;
        }
    }
    return true;
}

bool operator>(const Vector& lhs, const double& rhs) {
    for (auto val : lhs) {
        if (val <= rhs) {
            return false;
        }
    }
    return true;
}

bool operator<=(const double& lhs, const Vector& rhs) {
    return rhs >= lhs;
}

bool operator<(const double& lhs, const Vector& rhs) {
    return rhs > lhs;
}

bool operator>=(const double& lhs, const Vector& rhs) {
    return rhs <= lhs;
}

bool operator>(const double& lhs, const Vector& rhs) {
    return rhs < lhs;
}

// Matrix-Vector operators:
// Matrix-vector multiplication operator

Vector operator*(const Matrix& m, const Vector& v) {
    if (m.col() == v.size()) {
        Vector ret(m.row());
        for (auto r = 0; r < m.row(); r++) {
            double tmp = 0;
            for (auto c = 0; c < m.col(); c++) {
                tmp += m[r][c] * v[c];
            }
            ret[r] = tmp;
        }
        return ret;
    }
    std::cerr << "Mismatched dimensions for matrix-vector product" << std::endl;
    return Vector();
}

// IO stream operators

std::ostream& operator<<(std::ostream& os, const Vector& v) {
    os << "[";
    std::string sep = " ";
    for (auto val : v) {
        os << sep;
        os << val;
        sep = ", ";
    }
    os << "]" << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Matrix& m) {
    os << "[" << std::endl;
    for (auto r = 0; r < m.row(); r++) {
        os << " [";
        std::string sep = " ";
        for (auto c = 0; c < m.col(); c++) {
            os << sep;
            os << m[r][c];
            sep = ", ";
        }
        os << "]" << std::endl;
    }
    os << "]" << std::endl;
    return os;
}




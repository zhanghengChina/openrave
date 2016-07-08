// -*- coding: utf-8 -*-
// Copyright (C) 2016 Puttichai Lertkultanon <L.Puttichai@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU Lesser General Public License as published by the Free Software Foundation, either version 3
// of the License, or at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
#include "ramp.h"
#include <iostream>

namespace RampOptimizerInternal {

////////////////////////////////////////////////////////////////////////////////
// Ramp
Ramp::Ramp(Real v0_, Real a_, Real dur_, Real x0_)  {
    BOOST_ASSERT(dur_ >= -epsilon);

    v0 = v0_;
    a = a_;
    duration = dur_;
    x0 = x0_;

    v1 = v0 + (a*duration);
    d = duration*(v0 + 0.5*a*duration);
    x1 = x0 + d;
}

Real Ramp::EvalPos(Real t) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t <= 0) {
        return x0;
    }
    else if (t >= duration) {
        return x1;
    }

    return t*(v0 + 0.5*a*t) + x0;
}

Real Ramp::EvalVel(Real t) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t <= 0) {
        return v0;
    }
    else if (t >= duration) {
        return v1;
    }

    return v0 + (a*t);
}

Real Ramp::EvalAcc(Real t) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    return a;
}

void Ramp::GetPeaks(Real& bmin, Real& bmax) const {
    if (FuzzyZero(a, epsilon)) {
        if (v0 > 0) {
            bmin = x0;
            bmax = x1;
        }
        else {
            bmin = x1;
            bmax = x0;
        }
        return;
    }
    else if (a > 0) {
        bmin = x0;
        bmax = x1;
    }
    else {
        bmin = x1;
        bmax = x0;
    }

    Real tDeflection = -v0/a;
    if ((tDeflection <= 0) || (tDeflection >= duration)) {
        return;
    }

    Real xDeflection = EvalPos(tDeflection);
    bmin = Min(bmin, xDeflection);
    bmax = Max(bmax, xDeflection);
    return;
}

void Ramp::Initialize(Real v0_, Real a_, Real dur_, Real x0_) {
    BOOST_ASSERT(dur_ >= -epsilon);

    v0 = v0_;
    a = a_;
    duration = dur_;
    x0 = x0_;

    v1 = v0 + (a*duration);
    d = duration*(v0 + 0.5*a*duration);
    x1 = x0 + d;
}

void Ramp::PrintInfo(std::string name) const {
    std::cout << "Ramp information: " << name << std::endl;
    std::cout << str(boost::format("  v0 = %.15e")%v0) << std::endl;
    std::cout << str(boost::format("   a = %.15e")%a) << std::endl;
    std::cout << str(boost::format("   t = %.15e")%duration) << std::endl;
    std::cout << str(boost::format("  x0 = %.15e")%x0) << std::endl;
    std::cout << str(boost::format("  v1 = %.15e")%v1) << std::endl;
    std::cout << str(boost::format("   d = %.15e")%d) << std::endl;
    std::cout << str(boost::format("  x1 = %.15e")%x1) << std::endl;
}

void Ramp::UpdateDuration(Real newDuration) {
    BOOST_ASSERT(newDuration >= -epsilon);
    if (newDuration < 0) {
        newDuration = 0;
    }
    // Update the members accordinly
    duration = newDuration;
    v1 = v0 + (a*duration);
    d = duration*(v0 + 0.5*a*duration);
    x1 = x0 + d;
}

////////////////////////////////////////////////////////////////////////////////
// ParabolicCurve
ParabolicCurve::ParabolicCurve(std::vector<Ramp> rampsIn) {
    BOOST_ASSERT(!rampsIn.empty());

    ramps.reserve(rampsIn.size());
    switchpointsList.reserve(rampsIn.size() + 1);
    d = 0.0;
    duration = 0.0;
    switchpointsList.push_back(duration);

    for (std::size_t i = 0; i != rampsIn.size(); ++i) {
        ramps.push_back(rampsIn[i]);
        d = d + ramps.back().d;
        duration = duration + ramps[i].duration;
        switchpointsList.push_back(duration);
    }
    v0 = rampsIn.front().v0;
    v1 = rampsIn.back().v1;
    SetInitialValue(rampsIn[0].x0);
}

void ParabolicCurve::Append(ParabolicCurve curve) {
    BOOST_ASSERT(!curve.IsEmpty());
    // Users need to make sure that the displacement and velocity are continuous

    std::size_t prevLength = 0;
    std::size_t sz = curve.ramps.size();
    if (ramps.size() == 0) {
        switchpointsList.reserve(sz + 1);
        d = 0.0;
        duration = 0.0;
        switchpointsList.push_back(duration);
        x0 = curve.ramps[0].x0;
    }
    prevLength = ramps.size();
    ramps.reserve(prevLength + sz);

    for (std::size_t i = 0; i != sz; ++i) {
        ramps.push_back(curve.ramps[i]);
        d = d + ramps.back().d;
        duration = duration + ramps.back().duration;
        switchpointsList.push_back(duration);
    }
    v1 = curve.v1;
    SetInitialValue(x0);
}

void ParabolicCurve::Reset() {
    x0 = 0;
    x1 = 0;
    duration = 0;
    d = 0;
    v0 = 0;
    v1 = 0;
    switchpointsList.clear();
    ramps.clear();
}

void ParabolicCurve::SetInitialValue(Real newx0) {
    x0 = newx0;
    size_t nRamps = ramps.size();
    for (size_t i = 0; i < nRamps; ++i) {
        ramps[i].x0 = newx0;
        newx0 = newx0 + ramps[i].d;
    }
    x1 = x0 + d;
}

void ParabolicCurve::FindRampIndex(Real t, int& index, Real& remainder) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t < epsilon) {
        index = 0;
        remainder = 0;
    }
    else if (t > duration - epsilon) {
        index = ((int) ramps.size()) - 1;
        remainder = ramps.back().duration;
    }
    else {
        index = 0;
        // Iterate through switchpointsList
        std::vector<Real>::const_iterator it = switchpointsList.begin();
        while (it != switchpointsList.end() && t > *it) {
            index++;
            it++;
        }
        BOOST_ASSERT(index < (int)switchpointsList.size());
        index = index - 1;
        remainder = t - *(it - 1);
    }
    return;
}

void ParabolicCurve::Initialize(std::vector<Ramp> rampsIn) {
    ramps.resize(0);
    ramps.reserve(rampsIn.size());

    switchpointsList.resize(0);
    switchpointsList.reserve(rampsIn.size() + 1);

    d = 0.0;
    duration = 0.0;
    switchpointsList.push_back(duration);
    v0 = rampsIn.front().v0;
    v1 = rampsIn.back().v1;

    for (std::size_t i = 0; i != rampsIn.size(); ++i) {
        ramps.push_back(rampsIn[i]);
        d = d + ramps.back().d;
        duration = duration + ramps[i].duration;
        switchpointsList.push_back(duration);
    }
    SetInitialValue(rampsIn[0].x0);
}

void ParabolicCurve::PrintInfo(std::string name) const {
    std::cout << "ParabolicCurve information: " << name << std::endl;
    std::cout << str(boost::format("  This parabolic curve consists of %d ramps")%(ramps.size())) << std::endl;
    std::cout << str(boost::format("  v0 = %.15e")%(ramps[0].v0)) << std::endl;
    std::cout << str(boost::format("   t = %.15e")%duration) << std::endl;
    std::cout << str(boost::format("  x0 = %.15e")%x0) << std::endl;
    std::cout << str(boost::format("  x1 = %.15e")%x1) << std::endl;
    std::cout << str(boost::format("   d = %.15e")%d) << std::endl;
    std::string swList = GenerateStringFromVector(switchpointsList);
    swList = "  Switch points = " + swList;
    std::cout << swList << std::endl;
}

Real ParabolicCurve::EvalPos(Real t) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t <= 0) {
        return x0;
    }
    else if (t >= duration) {
        return x1;
    }

    int index;
    Real remainder;
    FindRampIndex(t, index, remainder);
    return ramps[index].EvalPos(remainder);
}

Real ParabolicCurve::EvalVel(Real t) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t <= 0) {
        return v0;
    }
    else if (t >= duration) {
        return v1;
    }

    int index;
    Real remainder;
    FindRampIndex(t, index, remainder);
    return ramps[index].EvalVel(remainder);
}

Real ParabolicCurve::EvalAcc(Real t) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t <= 0) {
        return ramps[0].a;
    }
    else if (t >= duration) {
        return ramps.back().a;
    }

    int index;
    Real remainder;
    FindRampIndex(t, index, remainder);
    return ramps[index].a;
}

void ParabolicCurve::GetPeaks(Real& bmin, Real& bmax) const {
    bmin = inf;
    bmax = -inf;
    Real temp1, temp2;
    for (std::vector<Ramp>::const_iterator it = ramps.begin(); it != ramps.end(); ++it) {
        it->GetPeaks(temp1, temp2);
        if (temp1 < bmin) {
            bmin = temp1;
        }
        if (temp2 > bmax) {
            bmax = temp2;
        }
    }
    RAMP_OPTIM_ASSERT(bmin < inf);
    RAMP_OPTIM_ASSERT(bmax > -inf);
    return;
}

////////////////////////////////////////////////////////////////////////////////
// paraboliccurvesnd
ParabolicCurvesND::ParabolicCurvesND(std::vector<ParabolicCurve> curvesIn) {
    BOOST_ASSERT(!curvesIn.empty());
    constraintchecked = 0;
    modified = 0;

    ndof = (int) curvesIn.size();
    // Here we need to check whether every curve has (roughly) the same duration
    ///////////////////
    Real minDur = curvesIn[0].duration;
    for (std::vector<ParabolicCurve>::const_iterator it = curvesIn.begin() + 1; it != curvesIn.end(); ++it) {
        BOOST_ASSERT(FuzzyEquals(it->duration, minDur, epsilon));
        minDur = Min(minDur, it->duration);
    }
    // Need to think about how to handle discrepancies in durations.

    curves = curvesIn;
    duration = minDur;

    x0Vect.reserve(ndof);
    x1Vect.reserve(ndof);
    dVect.reserve(ndof);
    v0Vect.reserve(ndof);
    v1Vect.reserve(ndof);
    for (int i = 0; i < ndof; ++i) {
        x0Vect.push_back(curves[i].x0);
        x1Vect.push_back(curves[i].x1);
        v0Vect.push_back(curves[i].v0);
        v1Vect.push_back(curves[i].v1);
        dVect.push_back(curves[i].d);
    }

    // Manage switch points (later I will merge this for loop with the one above)
    switchpointsList = curves[0].switchpointsList;
    std::vector<Real>::iterator it;
    for (int i = 0; i < ndof; i++) {
        for (std::size_t j = 1; j != curves[i].switchpointsList.size() - 1; ++j) {
            // Iterate from the second element to the second last element (the first and the last
            // switch points should be the same for every DOF)
            it = std::lower_bound(switchpointsList.begin(), switchpointsList.end(), curves[i].switchpointsList[j]);
            if (!FuzzyEquals(curves[i].switchpointsList[j], *it, epsilon)) {
                // Insert only non-redundant switch points
                switchpointsList.insert(it, curves[i].switchpointsList[j]);
            }
        }
    }
}

void ParabolicCurvesND::Append(ParabolicCurvesND curvesnd) {
    BOOST_ASSERT(!curvesnd.IsEmpty());
    // Users need to make sure that the displacement and velocity vectors are continuous

    if (curves.empty()) {
        curves = curvesnd.curves;
        ndof = curves.size();
        duration = curvesnd.duration;
        x0Vect = curvesnd.x0Vect;
        x1Vect = curvesnd.x1Vect;
        dVect = curvesnd.dVect;
        v0Vect = curvesnd.v0Vect;
        v1Vect = curvesnd.v1Vect;
        switchpointsList = curvesnd.switchpointsList;
    }
    else {
        BOOST_ASSERT(curvesnd.ndof == ndof);
        Real originalDur = duration;
        duration = duration + curvesnd.duration;
        for (int i = 0; i < ndof; i++) {
            curves[i].Append(curvesnd.curves[i]);
            v1Vect[i] = curvesnd.curves[i].v1;
            x1Vect[i] = curves[i].x1;
            dVect[i] = dVect[i] + curvesnd.curves[i].d;
        }
        std::vector<Real> tempSwitchpointsList = curvesnd.switchpointsList;
        // Add every element in tempSwitchpointsList by originalDur
        for (std::vector<Real>::iterator it = tempSwitchpointsList.begin(); it != tempSwitchpointsList.end(); ++it) {
            *it += originalDur;
        }
        switchpointsList.reserve(switchpointsList.size() + tempSwitchpointsList.size());
        switchpointsList.insert(switchpointsList.end(), tempSwitchpointsList.begin(), tempSwitchpointsList.end());
    }
    return;
}

void ParabolicCurvesND::Initialize(std::vector<ParabolicCurve> curvesIn) {
    constraintchecked = 0;
    modified = 0;

    ndof = curvesIn.size();
    // Here we need to check whether every curve has (roughly) the same duration
    ///////////////////
    Real minDur = curvesIn[0].duration;
    for (std::vector<ParabolicCurve>::const_iterator it = curvesIn.begin() + 1; it != curvesIn.end(); ++it) {
        BOOST_ASSERT(FuzzyEquals(it->duration, minDur, epsilon));
        minDur = Min(minDur, it->duration);
    }
    // Need to think about how to handle discrepancies in durations.

    curves = curvesIn;
    duration = minDur;

    x0Vect.reserve(ndof);
    x1Vect.reserve(ndof);
    dVect.reserve(ndof);
    v0Vect.reserve(ndof);
    v1Vect.reserve(ndof);
    for (int i = 0; i < ndof; ++i) {
        x0Vect.push_back(curves[i].x0);
        x1Vect.push_back(curves[i].x1);
        v0Vect.push_back(curves[i].v0);
        v1Vect.push_back(curves[i].v1);
        dVect.push_back(curves[i].d);
    }

    // Manage switch points (later I will merge this for loop with the one above)
    switchpointsList = curves[0].switchpointsList;
    std::vector<Real>::iterator it;
    for (int i = 0; i < ndof; i++) {
        for (std::size_t j = 1; j != curves[i].switchpointsList.size() - 1; ++j) {
            // Iterate from the second element to the second last element (the first and the last
            // switch points should be the same for every DOF)
            it = std::lower_bound(switchpointsList.begin(), switchpointsList.end(), curves[i].switchpointsList[j]);
            if (!FuzzyEquals(curves[i].switchpointsList[j], *it, epsilon)) {
                // Insert only non-redundant switch points
                switchpointsList.insert(it, curves[i].switchpointsList[j]);
            }
        }
    }

    // std::cout << "INITIALIZATION WITH DURATION = " << duration << std::endl;
}

void ParabolicCurvesND::PrintInfo(std::string name) const {
    std::cout << "ParabolicCurvesND information: " << name << std::endl;
    std::cout << str(boost::format("  This parabolic curve has %d DOFs")%ndof) << std::endl;
    std::cout << str(boost::format("  t = %.15e")%duration) << std::endl;
    std::string x0VectString = GenerateStringFromVector(x0Vect);
    x0VectString = "  x0Vect = " + x0VectString;
    std::cout << x0VectString << std::endl;
    std::string x1VectString = GenerateStringFromVector(x1Vect);
    x0VectString = "  x1Vect = " + x1VectString;
    std::cout << x1VectString << std::endl;
    std::string swList = GenerateStringFromVector(switchpointsList);
    swList = "  Switch points = " + swList;
    std::cout << swList << std::endl;
}

void ParabolicCurvesND::EvalPos(Real t, std::vector<Real>& xVect) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t <= 0) {
        xVect = x0Vect;
        return;
    }
    else if (t > duration) {
        xVect = x1Vect;
        return;
    }

    xVect.resize(ndof);
    for (int i = 0; i < ndof; i++) {
        xVect[i] = curves[i].EvalPos(t);
    }
    return;
}

void ParabolicCurvesND::EvalVel(Real t, std::vector<Real>& vVect) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t <= 0) {
        vVect = v0Vect;
        return;
    }
    else if (t >= duration) {
        vVect = v1Vect;
        return;
    }

    vVect.resize(ndof);
    for (int i = 0; i < ndof; i++) {
        vVect[i] = curves[i].EvalVel(t);
    }
    return;
}

void ParabolicCurvesND::EvalAcc(Real t, std::vector<Real>& aVect) const {
    BOOST_ASSERT(t >= -epsilon);
    BOOST_ASSERT(t <= duration + epsilon);
    if (t < 0) {
        t = 0;
    }
    else if (t > duration) {
        t = duration;
    }

    aVect.resize(ndof);
    for (int i = 0; i < ndof; i++) {
        aVect[i] = curves[i].EvalAcc(t);
    }
    return;
}

void ParabolicCurvesND::GetPeaks(std::vector<Real>& bminVect, std::vector<Real>& bmaxVect) const {
    bminVect.resize(ndof);
    bminVect.resize(ndof);
    for (int i = 0; i < ndof; ++i) {
        curves[i].GetPeaks(bminVect[i], bmaxVect[i]);
    }
    return;
}

void ParabolicCurvesND::Reset() {
    ndof = 0;
    duration = 0;
    x0Vect.clear();
    x1Vect.clear();
    dVect.clear();
    v1Vect.clear();
    v0Vect.clear();
    switchpointsList.clear();
    curves.clear();
    constraintchecked = 0;
    modified = 0;
}

std::string GenerateStringFromVector(const std::vector<Real>& vect) {
    std::string s = "[ ";
    std::string separator = "";
    for (size_t i = 0; i < vect.size(); ++i) {
        s = s + separator + str(boost::format("%.15e")%(vect[i]));
        separator = ", ";
    }
    s = s + "]";
    return s;
}

} // end namespace RampOptimizerInternal

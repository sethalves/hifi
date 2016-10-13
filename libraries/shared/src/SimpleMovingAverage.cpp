//
//  SimpleMovingAverage.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 4/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SharedUtil.h"
#include "SimpleMovingAverage.h"

SimpleMovingAverage::SimpleMovingAverage(int numSamplesToAverage) :
    _numSamples(0),
    _lastEventTimestamp(0),
    _average(0.0f),
    _eventDeltaAverage(0.0f),
    WEIGHTING(1.0f / numSamplesToAverage),
    ONE_MINUS_WEIGHTING(1 - WEIGHTING) {
}

SimpleMovingAverage::SimpleMovingAverage(const SimpleMovingAverage& other) {
    *this = other;
}


SimpleMovingAverage& SimpleMovingAverage::operator=(const SimpleMovingAverage& other) {
    std::unique_lock<std::mutex> lock(_lock);
    other.takeLock();
    _numSamples = other._numSamples;
    _lastEventTimestamp = other._lastEventTimestamp;
    _average = other._average;
    _eventDeltaAverage = other._eventDeltaAverage;
    other.releaseLock();
    return *this;
}

int SimpleMovingAverage::updateAverage(float sample) {
    std::unique_lock<std::mutex> lock(_lock);
    if (_numSamples > 0) {
        _average = (ONE_MINUS_WEIGHTING * _average) + (WEIGHTING * sample);

        float eventDelta = (usecTimestampNow() - _lastEventTimestamp) / 1000000.0f;

        if (_numSamples > 1) {
            _eventDeltaAverage = (ONE_MINUS_WEIGHTING * _eventDeltaAverage) +
                (WEIGHTING * eventDelta);
        } else {
            _eventDeltaAverage = eventDelta;
        }
    } else {
        _average = sample;
        _eventDeltaAverage = 0;
    }

    _lastEventTimestamp =  usecTimestampNow();

    return ++_numSamples;
}

void SimpleMovingAverage::reset() {
    std::unique_lock<std::mutex> lock(_lock);
    _numSamples = 0;
    _average = 0.0f;
    _eventDeltaAverage = 0.0f;
}

int SimpleMovingAverage::getSampleCount() const {
    std::unique_lock<std::mutex> lock(_lock);
    return _numSamples;
};

float SimpleMovingAverage::getAverage() const {
    std::unique_lock<std::mutex> lock(_lock);
    return _average;
};

float SimpleMovingAverage::getEventDeltaAverage() const {
    std::unique_lock<std::mutex> lock(_lock);
    return (ONE_MINUS_WEIGHTING * _eventDeltaAverage) +
        (WEIGHTING * ((usecTimestampNow() - _lastEventTimestamp) / 1000000.0f));
}

float SimpleMovingAverage::getAverageSampleValuePerSecond() const {
    float eventDeltaAverage = getEventDeltaAverage();
    std::unique_lock<std::mutex> lock(_lock);
    return _average * (1.0f / eventDeltaAverage);
}

uint64_t SimpleMovingAverage::getUsecsSinceLastEvent() const {
    std::unique_lock<std::mutex> lock(_lock);
    return usecTimestampNow() - _lastEventTimestamp;
}

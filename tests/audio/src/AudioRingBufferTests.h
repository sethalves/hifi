//
//  AudioRingBufferTests.h
//  tests/audio/src
//
//  Created by Yixin Wang on 6/24/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioRingBufferTests_h
#define hifi_AudioRingBufferTests_h

#include <QtTest/QtTest>

#include "AudioRingBuffer.h"


class AudioRingBufferTests : public QObject {
    Q_OBJECT
private slots:
    void runAllTests();
private:
    void assertBufferSize(const AudioRingBuffer& buffer, int samples);
};

#endif // hifi_AudioRingBufferTests_h

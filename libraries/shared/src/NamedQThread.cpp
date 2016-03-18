//
//  NamedQThread.cpp
//  libraries/shared/src
//
//  Created by Seth Alves on 2016-3-18.
//  Copyright 2016 High Fidelity, Inc.
//
//  QThreads which can be tracked from qApp
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QApplication>
#include "NamedQThread.h"

NamedQThread::NamedQThread(QString name) :
    QThread(qApp) {
    setObjectName(name);
}

NamedQThread::~NamedQThread() {
}

void NamedQThread::run() {
    ID = QThread::currentThreadId();
    QThread::run();
}

//
//  NamedQThread.h
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


#ifndef hifi_NamedQThread_h
#define hifi_NamedQThread_h

#include <QThread>

class NamedQThread : public QThread {
    Q_OBJECT

public:
    NamedQThread(QString name);
    virtual ~NamedQThread();
    Qt::HANDLE getID() { return ID; }
    virtual void run() override;

private:
    Qt::HANDLE ID;
};

#endif // hifi_NamedQThread_h

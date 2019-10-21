//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <BuildInfo.h>

#include <SharedUtil.h>
#include <SettingInterface.h>
#include "WebRTCServerTestApp.h"

using namespace std;

int main(int argc, char * argv[]) {
    setupHifiApplication("WebRTC Server Test");

    Setting::init();

    WebRTCServerTestApp app(argc, argv);
    return app.exec();
}

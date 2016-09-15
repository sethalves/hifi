//
//  WebView.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtWebEngine 1.1

WebEngineView {
    id: root
    objectName: "BaseWebViewWebEngineView"
    property var newUrl;

    // creates a global EventBridge object.
    WebEngineScript {
        id: createGlobalEventBridge
        sourceCode: eventBridgeJavaScriptToInject
        injectionPoint: WebEngineScript.DocumentCreation
        worldId: WebEngineScript.MainWorld
    }

    // detects when to raise and lower virtual keyboard
    WebEngineScript {
        id: raiseAndLowerKeyboard
        injectionPoint: WebEngineScript.Deferred
        sourceUrl: resourceDirectoryUrl + "/html/raiseAndLowerKeyboard.js"
        worldId: WebEngineScript.MainWorld
    }

    userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard ]

    profile: desktop.browserProfile

    Component.onCompleted: {
        console.log("Connecting JS messaging to Hifi Logging")
        // Ensure the JS from the web-engine makes it to our logging
        root.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
            console.log("Web Window JS message: " + sourceID + " " + lineNumber + " " +  message);
        });
    }



    // FIXME hack to get the URL with the auth token included.  Remove when we move to Qt 5.6
    Timer {
        id: urlReplacementTimer
        running: false
        repeat: false
        interval: 50
        onTriggered: url = newUrl;
    }

    onUrlChanged: {
        var originalUrl = url.toString();
        newUrl = urlHandler.fixupUrl(originalUrl).toString();
        if (newUrl !== originalUrl) {
            root.stop();
            if (urlReplacementTimer.running) {
                console.warn("Replacement timer already running");
                return;
            }
            urlReplacementTimer.start();
        }
    }

    onLoadingChanged: {
        // Required to support clicking on "hifi://" links
        if (WebEngineView.LoadStartedStatus == loadRequest.status) {
            var url = loadRequest.url.toString();
            if (urlHandler.canHandleUrl(url)) {
                if (urlHandler.handleUrl(url)) {
                    root.stop();
                }
            }
        }
    }
}

"use strict";

//
//  tabletUI.js
//
//  scripts/system/tablet-ui/
//
//  Created by Seth Alves 2016-9-29
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Script, HMD, WebTablet, UIWebTablet, UserActivityLogger, Settings, Entities, Messages, Tablet, Overlays, MyAvatar */

(function() { // BEGIN LOCAL_SCOPE
    var tabletShown = false;
    var tabletLocation = null;
    var activeHand = null;

    Script.include("../libraries/WebTablet.js");

    function showTabletUI() {
        tabletShown = true;
        print("show tablet-ui");

        var DEFAULT_WIDTH = 0.4375;
        var DEFAULT_HMD_TABLET_SCALE = 100;
        var HMD_TABLET_SCALE = Settings.getValue("hmdTabletScale") || DEFAULT_HMD_TABLET_SCALE;
        UIWebTablet = new WebTablet("qml/hifi/tablet/TabletRoot.qml", DEFAULT_WIDTH * (HMD_TABLET_SCALE / 100), null, activeHand, true);
        UIWebTablet.register();
        HMD.tabletID = UIWebTablet.tabletEntityID;
        HMD.homeButtonID = UIWebTablet.homeButtonEntity;
    }

    function hideTabletUI() {
        tabletShown = false;
        print("hide tablet-ui");
        if (UIWebTablet) {
            if (UIWebTablet.onClose) {
                UIWebTablet.onClose();
            }

            tabletLocation = UIWebTablet.getLocation();
            UIWebTablet.unregister();
            UIWebTablet.destroy();
            UIWebTablet = null;
            HMD.tabletID = null;
            HMD.homeButtonID = null;
        }
    }

    function updateShowTablet() {
        if (tabletShown) {
            var currentMicLevel = getMicLevel();
            var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            tablet.updateAudioBar(currentMicLevel);
        }

        if (tabletShown && UIWebTablet && Overlays.getOverlayType(UIWebTablet.webOverlayID) != "web3d") {
            // when we switch domains, the tablet entity gets destroyed and recreated.  this causes
            // the overlay to be deleted, but not recreated.  If the overlay is deleted for this or any
            // other reason, close the tablet.
            hideTabletUI();
            HMD.closeTablet();
        } else if (HMD.showTablet && !tabletShown) {
            UserActivityLogger.openedTablet();
            showTabletUI();
        } else if (!HMD.showTablet && tabletShown) {
            UserActivityLogger.closedTablet();
            hideTabletUI();
        }
    }

    function toggleHand(channel, hand, senderUUID, localOnly) {
        if (channel === "toggleHand") {
            activeHand = JSON.parse(hand);
        }
    }

    Messages.subscribe("toggleHand");
    Messages.messageReceived.connect(toggleHand);

    Script.setInterval(updateShowTablet, 100);

    // Initialise variables used to calculate audio level
    var accumulatedLevel = 0.0;
    // Note: Might have to tweak the following two based on the rate we're getting the data
    var AVERAGING_RATIO = 0.05;

    // Calculate microphone level with the same scaling equation (log scale, exponentially averaged) in AvatarInputs and pal.js
    function getMicLevel() {
        var LOUDNESS_FLOOR = 11.0;
        var LOUDNESS_SCALE = 2.8 / 5.0;
        var LOG2 = Math.log(2.0);
        var micLevel = 0.0;
        accumulatedLevel = AVERAGING_RATIO * accumulatedLevel + (1 - AVERAGING_RATIO) * (MyAvatar.audioLoudness);
        // Convert to log base 2
        var logLevel = Math.log(accumulatedLevel + 1) / LOG2;

        if (logLevel <= LOUDNESS_FLOOR) {
            micLevel = logLevel / LOUDNESS_FLOOR * LOUDNESS_SCALE;
        } else {
            micLevel = (logLevel - (LOUDNESS_FLOOR - 1.0)) * LOUDNESS_SCALE;
        }
        if (micLevel > 1.0) {
            micLevel = 1.0;
        }
        return micLevel;
    }

    Script.scriptEnding.connect(function () {
        Entities.deleteEntity(HMD.tabletID);
        HMD.tabletID = null;
        HMD.homeButtonID = null;
    });
}()); // END LOCAL_SCOPE

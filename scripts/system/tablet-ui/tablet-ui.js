

/* global Script, HMD, WebTablet, UIWebTablet */

(function() { // BEGIN LOCAL_SCOPE
    var tabletShown = false;
    var tabletLocation = null;

    Script.include("../libraries/WebTablet.js");

    function showTabletUI() {
        tabletShown = true;
        print("show tablet-ui");
        // UIWebTablet = new WebTablet("controls/Keyboard.qml", null, null, tabletLocation);
        UIWebTablet = new WebTablet("desktop/TabletUI.qml", null, null, tabletLocation);
        // UserActivityLogger.openedTabletUI();
    }

    function hideTabletUI() {
        tabletShown = false;
        print("hide tablet-ui");
        if (UIWebTablet) {
            tabletLocation = UIWebTablet.getLocation();
            UIWebTablet.destroy();
            UIWebTablet = null;
        }
    }

    function updateShowTablet() {
        if (HMD.showTablet && !tabletShown) {
            showTabletUI();
        } else if (!HMD.showTablet && tabletShown) {
            hideTabletUI();
        }
    }

    
    Script.update.connect(updateShowTablet);
    // Script.setInterval(updateShowTablet, 1000);

}()); // END LOCAL_SCOPE

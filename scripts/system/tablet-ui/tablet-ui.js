

/* global Script, HMD, WebTablet, UIWebTablet */

(function() { // BEGIN LOCAL_SCOPE
    var tabletShown = false;
    var tabletLocation = null;

    Script.include("../libraries/WebTablet.js");

    function showTabletUI() {
        tabletShown = true;
        print("show tablet-ui");
        // var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
        UIWebTablet = new WebTablet("qml/desktop/TabletUI.qml", null, null, tabletLocation);
        var root = UIWebTablet.getRoot();
        var buttons = Toolbars.getToolbarButtons("com.highfidelity.interface.toolbar.system");
        print("HERE got buttons: ", buttons.length);
        for (var i = 0; i < buttons.length; i++) {
            print("HERE hooking up button: ", buttons[i].objectName);
            Toolbars.hookUpButtonClone("com.highfidelity.interface.toolbar.system", root, buttons[i]);
        }
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

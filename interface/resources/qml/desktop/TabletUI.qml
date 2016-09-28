import QtQuick 2.0

import "../hifi/toolbars"

Row {
    id: root
    spacing: 4
    Image {
        id: fuh
        source: "../../images/mic-mute.svg"
    }
    Rectangle {
        id: tabletUIRect
        width: 40
        height: 40
        color: "#252525"
    }

    Component { id: toolbarButtonBuilder; ToolbarButton { } }

    Component.onCompleted: {
        // var systemToolbar = desktop.getToolbar("com.highfidelity.interface.toolbar.system");
        // console.debug("HERE systemToolbar: ", systemToolbar);
        // console.debug("HERE systemToolbar: ", systemToolbar.getButtonNames());
        // var buttons = systemToolbar.buttons;
        // console.debug("HERE buttons: ", buttons);
        // var result = toolbarButtonBuilder.createObject(root, buttons[0]);

        for (var i = 0; i < blah.length; i++) {
            var result = toolbarButtonBuilder.createObject(root, blah[i]);
        }
    }
}

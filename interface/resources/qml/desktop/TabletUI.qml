import QtQuick 2.0

import "../hifi/toolbars"

Item {
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.margins: 50;

    Grid {
        id: root
        objectName: "tabletUIBase"
        spacing: 50
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        columns: 4
        Component { id: toolbarButtonBuilder; ToolbarButton { } }
        // Component.onCompleted: {
        //     for (var i = 0; i < toolbarButtons.length; i++) {
        //         var result = toolbarButtonBuilder.createObject(root, toolbarButtons[i]);
        //     }
        // }

        function addCloneButton(properties) {
            var result = toolbarButtonBuilder.createObject(root, properties);
            return result;
        }
    }
}

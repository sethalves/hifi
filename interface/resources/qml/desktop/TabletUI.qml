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
        function addCloneButton(properties) {
            var result = toolbarButtonBuilder.createObject(root, properties);
            return result;
        }
    }
}

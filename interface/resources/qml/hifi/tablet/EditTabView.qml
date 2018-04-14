import QtQuick 2.7
import QtQuick.Controls 2.2
import QtWebChannel 1.0
import "../../controls"
import "../toolbars"
import QtGraphicalEffects 1.0
import "../../controls-uit" as HifiControls
import "../../styles-uit"

TabBar {
    id: editTabView
    // anchors.fill: parent
    width: parent.width
    contentWidth: parent.width
    padding: 0
    spacing: 0

    readonly property HifiConstants hifi: HifiConstants {}

    EditTabButton {
        title: "CREATE"
        active: true
        enabled: true
        property string originalUrl: ""

        property Component visualItem: Component {

            Rectangle {
                color: "#404040"
                id: container

                Flickable {
                    height: parent.height
                    width: parent.width
                    clip: true

                    contentHeight: createEntitiesFlow.height +  importButton.height + assetServerButton.height +
                                   header.anchors.topMargin + createEntitiesFlow.anchors.topMargin +
                                   assetServerButton.anchors.topMargin + importButton.anchors.topMargin +
                                   header.paintedHeight

                    contentWidth: width

                    ScrollBar.vertical : ScrollBar {
                        visible: parent.contentHeight > parent.height
                        width: 20
                        background: Rectangle {
                            color: hifi.colors.tableScrollBackgroundDark
                        }
                    }

                    Text {
                        id: header
                        color: "#ffffff"
                        text: "Choose an Entity Type to Create:"
                        font.pixelSize: 14
                        font.bold: true
                        anchors.top: parent.top
                        anchors.topMargin: 28
                        anchors.left: parent.left
                        anchors.leftMargin: 28
                    }

                    Flow {
                        id: createEntitiesFlow
                        spacing: 35
                        anchors.right: parent.right
                        anchors.rightMargin: 55
                        anchors.left: parent.left
                        anchors.leftMargin: 55
                        anchors.top: parent.top
                        anchors.topMargin: 70


                        NewEntityButton {
                            icon: "icons/create-icons/94-model-01.svg"
                            text: "MODEL"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newModelButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/21-cube-01.svg"
                            text: "CUBE"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newCubeButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/22-sphere-01.svg"
                            text: "SPHERE"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newSphereButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/24-light-01.svg"
                            text: "LIGHT"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newLightButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/20-text-01.svg"
                            text: "TEXT"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newTextButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/image.svg"
                            text: "IMAGE"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newImageButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/25-web-1-01.svg"
                            text: "WEB"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newWebButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/23-zone-01.svg"
                            text: "ZONE"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newZoneButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/90-particles-01.svg"
                            text: "PARTICLE"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newParticleButton" }
                                                      });
                                editTabView.currentIndex = 4
                            }
                        }

                        NewEntityButton {
                            icon: "icons/create-icons/126-material-01.svg"
                            text: "MATERIAL"
                            onClicked: {
                                editRoot.sendToScript({
                                                          method: "newEntityButtonClicked", params: { buttonName: "newMaterialButton" }
                                                      });
                                editTabView.currentIndex = 2
                            }
                        }
                    }

                    HifiControls.Button {
                        id: assetServerButton
                        text: "Open This Domain's Asset Server"
                        color: hifi.buttons.black
                        colorScheme: hifi.colorSchemes.dark
                        anchors.right: parent.right
                        anchors.rightMargin: 55
                        anchors.left: parent.left
                        anchors.leftMargin: 55
                        anchors.top: createEntitiesFlow.bottom
                        anchors.topMargin: 35
                        onClicked: {
                            editRoot.sendToScript({
                                                      method: "newEntityButtonClicked", params: { buttonName: "openAssetBrowserButton" }
                                                  });
                        }
                    }

                    HifiControls.Button {
                        id: importButton
                        text: "Import Entities (.json)"
                        color: hifi.buttons.black
                        colorScheme: hifi.colorSchemes.dark
                        anchors.right: parent.right
                        anchors.rightMargin: 55
                        anchors.left: parent.left
                        anchors.leftMargin: 55
                        anchors.top: assetServerButton.bottom
                        anchors.topMargin: 20
                        onClicked: {
                            editRoot.sendToScript({
                                                      method: "newEntityButtonClicked", params: { buttonName: "importEntitiesButton" }
                                                  });
                        }
                    }
                }
            } // Flickable
        }
    }

    EditTabButton {
        title: "LIST"
        active: true
        enabled: true
        property string originalUrl: ""

        property Component visualItem: Component {
            WebView {
                id: entityListToolWebView
                url: Paths.defaultScripts + "/system/html/entityList.html"
                enabled: true
            }
        }
    }

    EditTabButton {
        title: "PROPERTIES"
        active: true
        enabled: true
        property string originalUrl: ""

        property Component visualItem: Component {
            WebView {
                id: entityPropertiesWebView
                url: Paths.defaultScripts + "/system/html/entityProperties.html"
                enabled: true
            }
        }
    }

    EditTabButton {
        title: "GRID"
        active: true
        enabled: true
        property string originalUrl: ""

        property Component visualItem: Component {
            WebView {
                id: gridControlsWebView
                url: Paths.defaultScripts + "/system/html/gridControls.html"
                enabled: true
            }
        }
    }

    EditTabButton {
        title: "P"
        active: true
        enabled: true
        property string originalUrl: ""

        property Component visualItem: Component {
            WebView {
                id: particleExplorerWebView
                url: Paths.defaultScripts + "/system/particle_explorer/particleExplorer.html"
                enabled: true
            }
        }
    }

    function fromScript(message) {
        switch (message.method) {
            case 'selectTab':
                selectTab(message.params.id);
                break;
            default:
                console.warn('Unrecognized message:', JSON.stringify(message));
        }
    }

    // Changes the current tab based on tab index or title as input
    function selectTab(id) {
        if (typeof id === 'number') {
            if (id >= 0 && id <= 4) {
                editTabView.currentIndex = id;
            } else {
                console.warn('Attempt to switch to invalid tab:', id);
            }
        } else if (typeof id === 'string'){
            switch (id.toLowerCase()) {
                case 'create':
                    editTabView.currentIndex = 0;
                    break;
                case 'list':
                    editTabView.currentIndex = 1;
                    break;
                case 'properties':
                    editTabView.currentIndex = 2;
                    break;
                case 'grid':
                    editTabView.currentIndex = 3;
                    break;
                case 'particle':
                    editTabView.currentIndex = 4;
                    break;
                default:
                    console.warn('Attempt to switch to invalid tab:', id);
            }
        } else {
            console.warn('Attempt to switch tabs with invalid input:', JSON.stringify(id));
        }
    }
}

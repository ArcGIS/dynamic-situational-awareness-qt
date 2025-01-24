/*******************************************************************************
 *  Copyright 2012-2018 Esri
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import Esri.ArcGISRuntime.OpenSourceApps.DSA
import QtQuick.Layouts

Rectangle {
    id: optionsRoot
    property real scaleFactor: (Screen.logicalPixelDensity * 25.4) / (Qt.platform.os === "windows" || Qt.platform.os === "linux" ? 96 : 72)

    color: Material.primary

    // Add the Options Controller Class
    OptionsController {
        id: optionsController
    }

    ConfigurationController {
        id: configurationController
    }

    TabBar {
        id: bar
        width: parent.width
        TabButton {
            text: qsTr("Options")
        }
        TabButton {
            text: qsTr("Configurations")
        }
    }

    StackLayout {
        width: parent.width
        anchors {
            top: bar.bottom
            bottom: buttonClose.top
        }

        currentIndex: bar.currentIndex

        // Create a flickable column so that n number of options can be added
        Item {
        Flickable {
            id: optionsFlickable

            anchors {
                fill: parent
                margins: 10 * scaleFactor
            }
            contentHeight: optionsColumn.height + parent.height * .15

            clip: true
            Column {
                id: optionsColumn
                width: parent.width

                spacing: 10 * scaleFactor

                Label {
                    text: "Map"
                    font {
                        family: DsaStyles.fontFamily
                        underline: true
                        pixelSize: DsaStyles.titleFontPixelSize * 0.75
                    }
                    color: Material.foreground
                }

                // Toggle navigation controls
                CheckBox {
                    text: "Show navigation controls"
                    checked: true
                    onCheckedChanged: {
                        // update visibility of UI components
                        navTool.visible = checked;
                        compass.visible = checked;
                    }
                }

                // Toggle location/elevation overlay
                CheckBox {
                    text: "Show location and elevation"
                    checked: true
                    onCheckedChanged: {
                        // update visibility of UI component
                        currentLocation.visible = checked;
                    }
                }

                // Toggle friendly tracks labels
                CheckBox {
                    text: "Show friendly tracks labels"
                    checked: true
                    onCheckedChanged: {
                        optionsController.showFriendlyTracksLabels = checked;
                    }
                }

                Label {
                    text: "Location"
                    font {
                        family: DsaStyles.fontFamily
                        underline: true
                        pixelSize: DsaStyles.titleFontPixelSize * 0.75
                    }
                    color: Material.foreground
                }

                // Whether to use GPS for the location/elevation display or not.
                // The alternative is the use the Scene's base surface.
                CheckBox {
                    id: useGPS
                    text: "Use GPS for current elevation display"
                    checked: optionsController.useGpsForElevation
                    onCheckedChanged: optionsController.useGpsForElevation = checked
                }


                CheckBox {
                    text: "Location Broadcast"
                    checked: messageFeeds.controller.locationBroadcastEnabled
                    onCheckedChanged: messageFeeds.controller.locationBroadcastEnabled = checked
                }

                Row {
                    height: 40 * scaleFactor
                    spacing: 5 * scaleFactor

                    Text {
                        text: "Location Broadcast frequency (ms)"
                        color: Material.foreground
                        font {
                            pixelSize: 10 * scaleFactor
                            family: DsaStyles.fontFamily
                        }
                    }

                    TextField {
                        width: 50 * scaleFactor
                        text: messageFeeds.controller.locationBroadcastFrequency
                        color: Material.foreground
                        font {
                            pixelSize: 10 * scaleFactor
                            family: DsaStyles.fontFamily
                        }

                        validator: IntValidator { bottom:0 }

                        onTextChanged: messageFeeds.controller.locationBroadcastFrequency = Number(text)
                    }
                }

                Label {
                    text: "General"
                    font {
                        family: DsaStyles.fontFamily
                        underline: true
                        pixelSize: DsaStyles.titleFontPixelSize * 0.75
                    }
                    color: Material.foreground
                }

                // Change the default coordinate formats between DMS, USNG, MGRS, etc.
                Row {
                    width: parent.width
                    spacing: 10 * scaleFactor

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Default Coordinate Format"
                        font {
                            pixelSize: 12 * scaleFactor
                            family: DsaStyles.fontFamily
                        }
                        color: Material.foreground
                    }

                    ComboBox {
                        anchors.verticalCenter: parent.verticalCenter
                        model: optionsController.coordinateFormats
                        Component.onCompleted: currentIndex = optionsController.initialFormatIndex
                        onCurrentTextChanged: optionsController.setCoordinateFormat(currentText);
                    }
                }

                // Change the default units between feet and meters
                Row {
                    width: parent.width
                    spacing: 10 * scaleFactor

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Default Unit of Measurement"
                        font {
                            pixelSize: 12 * scaleFactor
                            family: DsaStyles.fontFamily
                        }
                        color: Material.foreground
                    }

                    ComboBox {
                        anchors.verticalCenter: parent.verticalCenter
                        model: optionsController.units
                        Component.onCompleted: currentIndex = optionsController.initialUnitIndex
                        onCurrentTextChanged: optionsController.setUnitOfMeasurement(currentText)
                    }
                }

                Row {
                    height: 40 * scaleFactor
                    spacing: 5 * scaleFactor

                    Text {
                        text: "User name"
                        color: Material.foreground
                        font {
                            pixelSize: 10 * scaleFactor
                            family: DsaStyles.fontFamily
                        }
                    }

                    TextField {
                        width: 128 * scaleFactor
                        text: optionsController.userName
                        color: Material.foreground
                        font {
                            pixelSize: 10 * scaleFactor
                            family: DsaStyles.fontFamily
                        }

                        onTextEdited: {
                            if (optionsController.userName !== text)
                                optionsController.userName = text;
                        }
                    }
                }
            }
        }
}
        // Create a flickable column so that n number of options can be added
        Item {
            Flickable {
                id: configurationsFlickable
                anchors {
                    fill: parent
                    margins: 10 * scaleFactor
                }
                clip: true

                Component {
                    id: configurationListItemDelegate

                    Rectangle {
                        color: Material.backgroundColor
                        height: 40 * scaleFactor
                        width: parent.width

                        ProgressBar {
                            id: progressBarPercentDownloaded
                            anchors {
                                bottom: parent.bottom
                                right: parent.right
                                left: parent.left
                            }
                            visible: model.Downloading
                            to: 100
                            value: model.PercentDownloaded
                        }

                        CheckBox {
                            id: checkboxSelected
                            anchors {
                                left: parent.left
                                verticalCenter: parent.verticalCenter
                            }
                            checked: model.Selected
                            enabled: model.Downloaded
                            onClicked: {
                                configurationController.select(index)
                                checked = Qt.binding(function () { // restore the binding
                                    return model.Selected;
                                });
                            }
                        }

                        Label {
                            id: labelName
                            anchors {
                                left: checkboxSelected.right
                                verticalCenter: parent.verticalCenter
                            }
                            height: parent.height
                            text: model.Name
                            verticalAlignment: Text.AlignVCenter
                        }

                        Label {
                            id: labelRequiresRestart
                            anchors {
                                left: labelName.right
                                verticalCenter: parent.verticalCenter
                                margins: 2
                            }
                            text: "*"
                            color: "yellow"
                            font.italic: true
                            visible: model.RequiresRestart
                        }

                        Image {
                            id: imageCancel
                            source: "qrc:/Resources/icons/xhdpi/ic_menu_closeclear_dark.png"
                            height: parent.height
                            width: model.Downloading ? parent.height : 0
                            anchors {
                                right: imageDownload.left
                                verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: configurationController.cancel(index)
                            }
                        }

                        Image {
                            id: imageDownload
                            source: "qrc:/Resources/icons/xhdpi/ic_menu_sendmap_dark_d.png"
                            height: parent.height
                            width: model.CanDownload && !configurationController.downloading ? parent.height : 0
                            enabled: !configurationController.downloading
                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: configurationController.download(index)
                            }
                            rotation: 180
                        }

                        // Image {
                        //     id: imageRemove
                        //     source: DsaResources.iconTrash
                        //     height: parent.height
                        //     // width: model.Downloaded && !model.Selected && !model.Loaded ? parent.height / 2 : 0
                        //     width: 0 // TODO: remove when downloads from user are implemented
                        //     anchors {
                        //         right: parent.right
                        //         verticalCenter: parent.verticalCenter
                        //     }
                        //     MouseArea {
                        //         anchors.fill: parent
                        //         onClicked: configurationController.remove(index)
                        //     }
                        // }
                    }
                }

                ListView {
                    id: configurationList
                    anchors {
                        fill: parent
                    }
                    interactive: true
                    clip: true
                    spacing: 2 * scaleFactor
                    model: configurationController.configurations
                    delegate: configurationListItemDelegate
                }
            }

            Label {
                id: labelRequiresRestartText
                anchors {
                    bottom: parent.bottom
                    right: parent.right
                    margins: 25
                }
                padding: 5
                color: "yellow"
                background: Rectangle {
                    radius: 5
                    color: "green"
                }

                text: "*requires restart"
                visible: configurationController.requiresRestart
            }
        }
    }
    Button {
        id: buttonClose
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            margins: 10 * scaleFactor
        }
        text: "Close"
        onClicked: optionsRoot.visible = false;
    }
}

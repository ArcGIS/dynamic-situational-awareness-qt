/*******************************************************************************
 *  Copyright 2012-2025 Esri
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
import QtQuick.Layouts
import Esri.ArcGISRuntime.OpenSourceApps.DSA

DsaPanel {
    id: configurationsRoot
    title: qsTr("Configurations")

    ConfigurationController {
        id: toolController
    }

    Component {
        id: configurationListItemDelegate

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
            }
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
                visible: model.PercentDownloaded > 0 && model.PercentDownloaded < 100
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
                    toolController.select(index)
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

            Image {
                id: imageRequiresReload
                source: DsaResources.iconAlertLow
                anchors {
                    left: labelName.right
                    verticalCenter: parent.verticalCenter
                    margins: 4
                }
                height: parent.height / 2
                width: model.Selected && !model.Loaded ? parent.height / 2 : 0
            }

            Image {
                id: imageCancel
                source: "qrc:/Resources/icons/xhdpi/ic_menu_closeclear_dark.png"
                height: parent.height / 2
                width: model.PercentDownloaded > 0 && model.PercentDownloaded < 100 ? parent.height / 2 : 0
                anchors {
                    right: imageDownload.left
                    verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: toolController.cancel(index)
                }
            }

            Image {
                id: imageDownload
                source: "qrc:/Resources/icons/xhdpi/ic_menu_sendmap_dark_d.png"
                height: parent.height
                width: !model.Downloaded && !model.Url.length !== "" ? parent.height : 0
                enabled: !toolController.downloading
                anchors {
                    right: imageRemove.left
                    verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: toolController.download(index)
                }
            }

            Image {
                id: imageRemove
                source: DsaResources.iconTrash
                height: parent.height
                width: model.Downloaded && !model.Selected && !model.Loaded ? parent.height : 0
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: toolController.remove(index)
                }
            }
        }
    }

    ListView {
        id: configurationList
        anchors {
            top: configurationsRoot.titleBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        interactive: true
        clip: true
        spacing: 2 * scaleFactor
        model: toolController.configurations
        delegate: configurationListItemDelegate
    }
}

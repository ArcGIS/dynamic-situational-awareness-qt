
// Copyright 2017 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Window 2.2
import Esri.DSA 1.0

DsaPanel {
    id: createAlertsRoot
    width: 272 * scaleFactor
    title: qsTr("Edit Alert Rules")
    clip: true

    EditAlertsController {
        id: toolController
        active: parent.visible
        onPickedElement: {
            for (var i = 0; i < targetCB.count; ++i) {
                if (targetCB.textAt(i) === overlayName)
                {
                    targetCB.currentIndex = i;
                    featureIdEdit.text = elementId;
                    break;
                }
            }
        }
    }

    property bool readyToAdd: levelCb.currentIndex !== -1 &&
                              sourceCb.currentIndex !== -1 &&
                              targetCB.currentIndex !== -1 &&
                              featureIdEdit.text.length > 0
    property bool expandNewAlert: true

    Rectangle {
        id: newAlertFrame
        anchors {
            top: titleBar.bottom
            left: parent.left
            right: parent.right
            margins: 8 * scaleFactor
        }
        height: expandNewAlert ? parent.height * 0.75 : addButton.height + (anchors.margins * 2)
        color: "transparent"
        border.color: Material.foreground
        border.width: 1 * scaleFactor
        radius: 12
    }

    Image {
        id: collapseNewAlertButton
        anchors {
            margins: 8 * scaleFactor
            top: newAlertFrame.top
            left: newAlertFrame.left
        }

        width: 42 * scaleFactor
        height: 48 * scaleFactor
        source: "qrc:/Resources/icons/xhdpi/drawer_large.png"

        MouseArea {
            anchors.fill: parent
            onClicked: expandNewAlert = !expandNewAlert;
        }
    }

    Text {
        id: newAlertText
        anchors {
            margins: 8 * scaleFactor
            verticalCenter: collapseNewAlertButton.verticalCenter
            horizontalCenter: newAlertFrame.horizontalCenter
        }
        text: qsTr("New condition")
        color: Material.foreground
        font.pixelSize: DsaStyles.titleFontPixelSize * scaleFactor
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
    }

    Column {
        clip: true
        spacing: 16 * scaleFactor

        anchors {
            top: newAlertText.bottom
            left: newAlertFrame.left
            right: newAlertFrame.right
            bottom: newAlertFrame.bottom
            margins: 8 * scaleFactor
        }

        TextField {
            id: newAlertName
            color: Material.accent
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            font.pixelSize: DsaStyles.titleFontPixelSize * scaleFactor
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            placeholderText: "<condition name>"
        }

        ComboBox {
            id: levelCb
            anchors {
                left: parent.left
                right: parent.right
            }

            font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
            textRole: "display"
            model: toolController.levelNames
            currentIndex: -1

            Text {
                anchors.centerIn: parent
                visible: parent.currentIndex === -1
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                color: Material.accent
                text: "<level>"
            }
        }

        Text {
            anchors {
                left: parent.left
                right: parent.right
            }
            text: qsTr("Alert when")
            color: Material.foreground
            font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
            horizontalAlignment: Text.AlignLeft
        }

        ComboBox {
            id: sourceCb
            anchors {
                left: parent.left
                right: parent.right
            }

            font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
            textRole: "display"
            model: toolController.sourceNames
            currentIndex: -1

            Text {
                anchors.fill: parent
                visible: parent.currentIndex === -1
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                color: Material.accent
                text: "<source feed>"
            }
        }

        ButtonGroup {
            id: spatialOperationGroup
        }

        RadioButton {
            id: withinAreaRb
            anchors {
                left: parent.left
                right: parent.right
            }
            text: "within area"
            font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
            checked: true
            ButtonGroup.group: spatialOperationGroup
        }

        Row {
            spacing: 8 * scaleFactor
            RadioButton {
                id: withinDistanceRb
                text: "within distance"
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                ButtonGroup.group: spatialOperationGroup
            }

            SpinBox {
                id: withinDistanceSB
                anchors.verticalCenter: withinDistanceRb.verticalCenter
                enabled: withinDistanceRb.checked
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                editable: true
                value: 50
                from: 0
                to: 9999
            }

            Text {
                anchors.verticalCenter: withinDistanceRb.verticalCenter
                enabled: withinDistanceRb.checked
                text: qsTr("m of")
                color: Material.foreground
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
        }

        Row {
            spacing: 8 * scaleFactor
            height: pickButton.height
            Text {
                id: featureIdLabel
                text: qsTr("feature")
                color: Material.foreground
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            TextField {
                id: featureIdEdit
                validator: IntValidator{}
                color: Material.accent
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                font.bold: true
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                placeholderText: "<Feature ID>"
            }

            OverlayButton {
                id: pickButton
                selected: toolController.pickMode
                iconUrl: DsaResources.iconGps
                anchors.verticalCenter: featureIdLabel.verticalCenter
                onClicked: {
                    toolController.togglePickMode();
                }
            }
        }

        ComboBox {
            id: targetCB
            anchors {
                left: parent.left
                right: parent.right
            }
            font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor

            textRole: "display"
            model: toolController.targetNames
            currentIndex: -1

            Text {
                anchors.fill: targetCB
                visible: parent.currentIndex === -1
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: "<target overlay>"
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                color: Material.accent
            }
        }

        RoundButton {
            id: addButton
            enabled: readyToAdd

            anchors {
                horizontalCenter: parent.horizontalCenter
            }

            background: Rectangle {
                implicitWidth: 40 * scaleFactor
                implicitHeight: 40 * scaleFactor
                opacity: enabled ? 1 : 0.3
                radius: addButton.radius
                color: Material.accent

                Image {
                    anchors.centerIn: parent
                    width: 26 * scaleFactor
                    height: width
                    source: "qrc:/Resources/icons/xhdpi/ic_menu_add_dark_d.png"
                }
            }

            onClicked: {
                if (withinDistanceRb.checked) {
                    toolController.addWithinDistanceAlert(newAlertName.text,
                                                          levelCb.currentIndex,
                                                          sourceCb.currentText,
                                                          withinDistanceSB.value,
                                                          Number(featureIdEdit.text),
                                                          targetCB.currentIndex);
                }
                else if (withinAreaRb.checked) {
                    toolController.addWithinAreaAlert(newAlertName.text,
                                                      levelCb.currentIndex,
                                                      sourceCb.currentText,
                                                      Number(featureIdEdit.text),
                                                      targetCB.currentIndex);
                }

                levelCb.currentIndex = -1;
                sourceCb.currentIndex = -1;
                featureIdEdit.text = "";
                targetCB.currentIndex = "";
            }
        }
    }

    Text {
        id: conditionsListTitle
        anchors {
            margins: 8 * scaleFactor
            top: newAlertFrame.bottom
            horizontalCenter: newAlertFrame.horizontalCenter
        }
        text: qsTr("Conditions")
        color: Material.foreground
        font.pixelSize: DsaStyles.titleFontPixelSize * scaleFactor
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
    }

    ListView {
        id: alertsList
        clip: true
        anchors {
            margins: 8 * scaleFactor
            top: conditionsListTitle.bottom
            bottom: removeConditionButton.top
            left: parent.left
            right: parent.right
        }

        model: toolController.conditionsList
        currentIndex: -1

        delegate: Row {
            Text {
                text: name
                color: index === alertsList.currentIndex ? Material.accent : Material.foreground
                font.pixelSize: DsaStyles.toolFontPixelSize * scaleFactor
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAnywhere
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    onClicked: alertsList.currentIndex = index;
                }
            }
        }
    }

    RoundButton {
        id: removeConditionButton
        enabled: alertsList.currentIndex !== -1

        height: expandNewAlert ? 0 : implicitHeight

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            margins: 8 * scaleFactor
        }

        background: Rectangle {
            implicitWidth: 40 * scaleFactor
            implicitHeight: 40 * scaleFactor
            opacity: enabled ? 1 : 0.3
            radius: addButton.radius
            color: Material.accent

            Image {
                anchors.centerIn: parent
                width: expandNewAlert ? 0 : 26 * scaleFactor
                height: width
                source: DsaResources.iconTrash
            }
        }

        onClicked: {
            toolController.removeConditionAt(alertsList.currentIndex);
            alertsList.currentIndex = -1;
        }
    }
}

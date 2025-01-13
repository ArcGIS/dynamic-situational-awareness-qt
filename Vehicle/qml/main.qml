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
import QtQml.Models
import Esri.ArcGISRuntime.OpenSourceApps.DSA
import Esri.ArcGISRuntime.OpenSourceApps.Vehicle
import Esri.ArcGISRuntime.Toolkit as Toolkit

Vehicle {
    id: appRoot
    width: 800 * scaleFactor
    height: 600 * scaleFactor

    property real scaleFactor: (Screen.logicalPixelDensity * 25.4) / (Qt.platform.os === "windows" || Qt.platform.os === "linux" ? 96 : 72)
    property alias messageFeeds: messageFeedsTool
    property real hudOpacity: 0.9
    property real hudRadius: 3 * scaleFactor
    property real hudMargins: 5 * scaleFactor

    signal clearDialogAccepted();
    signal closeDialogAccepted();
    signal inputDialogAccepted(var input, var index);
    signal markupLayerReceived(var path, var overlayVisible);

    LocationController {
        id: locationController
        enabled: true
    }

    PrimaryToolbar {
        id: topToolbar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        toolbarLabelText: categoryToolbar.titleText
        height: DsaStyles.mainToolbarHeight * scaleFactor
        menuVisible: true

        HomeToolRow  {
            id: homeToolRow
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 16 * scaleFactor
            }
        }

        MapToolRow {
            id: mapToolRow
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 10 * scaleFactor
            }
        }

        ReportToolRow {
            id: reportToolRow
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 10 * scaleFactor
            }
        }

        AnalysisToolRow {
            id: analysisToolRow
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 10 * scaleFactor
            }
        }

        AlertToolRow {
            id: alertToolRow
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 10 * scaleFactor
            }
        }

        MarkupToolRow {
            id: markupToolRow
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 10 * scaleFactor
            }
        }

        ConfigurationsToolRow {
            id: configurationsToolRow
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 10 * scaleFactor
            }
        }
    }

    // Create SceneQuickView here, and create its Scene etc. in C++ code
    SceneView {
        id: sceneView
        anchors {
            top: topToolbar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        objectName: "sceneView"

        onMousePressed: followHud.stopFollowing();

        DistressButton {
            anchors {
                top: parent.top
                horizontalCenter: navTool.horizontalCenter
                topMargin: sceneView.height < navTool.height * 1.75 ? 10 * scaleFactor : 40 * scaleFactor
            }
            messageFeedsController: messageFeeds.controller
        }

        CurrentLocation {
            id: currentLocation
            anchors {
                bottom: sceneView.attributionTop
                right: navTool.left
                margins: hudMargins
            }
            opacity: hudOpacity
            radius: hudRadius
        }

        FollowHud {
            id: followHud
            anchors {
                bottom: sceneView.attributionTop
                horizontalCenter: parent.horizontalCenter
                margins: hudMargins
            }
            enabled: false
            opacity: hudOpacity
            radius: hudRadius
        }

        NavigationTool {
            id: navTool
            anchors {
                margins: hudMargins
                verticalCenter: parent.verticalCenter
                right: sceneView.right
            }
            opacity: hudOpacity
            radius: hudRadius
        }

        Toolkit.NorthArrow {
            id: compass
            geoView: sceneView
            anchors {
                horizontalCenter: navTool.horizontalCenter
                verticalCenter: followHud.verticalCenter
                margins: 10 * scaleFactor
            }
            width: DsaStyles.primaryIconSize * scaleFactor
            height: width
            autoHide: false;
        }

        CoordinateConversionToolProxy {
            id: dsaCoordinateController
            inInputMode: coordinateConversion.inInputMode
        }

        Toolkit.CoordinateConversion {
            id: coordinateConversion
            anchors {
                bottom: sceneView.attributionTop
                left: categoryToolbar.right
                right: followHud.left
                margins: hudMargins
            }

            geoView: sceneView
            controller: dsaCoordinateController.controller
            inputFormat: dsaCoordinateController.inputFormat
            visible: dsaCoordinateController.active

            palette {
                text: Material.foreground
                highlight: Material.accent
            }

            font {
                family: DsaStyles.fontFamily
                pixelSize: DsaStyles.toolFontPixelSize
            }

            background: Rectangle {
                color: Material.background
                opacity: hudOpacity
                radius: hudRadius
            }

            onVisibleChanged: {
                if (!visible)
                    return;

                if (mapToolRow.state !== "Convert XY") {
                    mapToolRow.state = "Convert XY";
                    categoryToolbar.state = "map";
                }
            }
        }

        ContextMenu {
            id: contextMenu
        }

        CategoryToolbarColumn {
            id: categoryToolbar
            anchors {
                top: parent.top
                left: parent.left
                bottom: sceneView.attributionTop
            }
            width: 56 * scaleFactor
            appTitle: "DSA - V"

            onSettingsClicked: optionsTool.visible = true;
            onAboutClicked: aboutTool.visible = true;
        }

        TableOfContents {
            id: tableOfContentsTool
            anchors {
                right: parent.right
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: false
            onClosed: {
                mapToolRow.tocIconSelected = false;
                visible = false;
                mapToolRow.state = "clear";
            }
        }

        AlertList {
            id: alertsTool
            anchors {
                right: parent.right
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: false
            onClosed: {
                visible = false;
                alertToolRow.state = "clear";
            }
        }

        AlertConditionsTool {
            id: alertConditionsTool
            anchors {
                right: parent.right
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: false
            onClosed: {
                visible = false;
                alertToolRow.state = "clear";
            }
        }

        MarkupTool {
            id: markup
            anchors {
                right: parent.right
                top: parent.top
            }
            visible: false
        }

        Viewshed {
            id: viewshedTool
            anchors {
                right: parent.right
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false

            onMyLocationModeSelected: {
                navTool.startFollowing();
            }

            onClosed: {
                analysisToolRow.state = "clear";
            }
        }

        LineOfSightTool {
            id: lineOfSightTool
            width: drawer.width
            anchors {
                right: parent.right
                top: parent.top
            }
            visible: false
        }

        AnalysisList {
            id: analysisListTool
            anchors {
                right: parent.right
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: false
            onClosed: {
                visible = false;
                analysisToolRow.state = "clear";
            }
        }

        ObservationReportTool {
            id: observationReportTool
            anchors {
                right: parent.right
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: false
            onClosed: {
                visible = false;
                reportToolRow.state = reportToolRow.clearState;
            }

            onVisibleChanged: {
                if (!visible)
                    return;

                categoryToolbar.state = "reports";

                if (reportToolRow.state !== reportToolRow.observationState)
                    reportToolRow.state = reportToolRow.observationState;
            }
        }

        ConfigurationsTool {
            id: configurationsTool
            anchors {
                right: parent.right
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            onClosed: {
                visible = false;
                configurationsToolRow.state = configurationsToolRow.clearState;
            }

            onVisibleChanged: {
                if (!visible)
                    return;

                categoryToolbar.state = "configurations";

                if (configurationsToolRow.state !== configurationsToolRow.observationState)
                    configurationsToolRow.state = configurationsToolRow.observationState;
            }
        }

        Toolkit.PopupStackView {
            id: identifyResults
            anchors {
                top: sceneView.top
                right: sceneView.right
                bottom: sceneView.attributionTop
            }
            palette {
                text: Material.foreground
            }
            background: Rectangle {
                color: Material.primary
            }
            visible: false
        }

        Drawer {
            id: drawer
            width: 272 * scaleFactor
            height: sceneView.height - 20 * scaleFactor // approximation for attribution text
            edge: Qt.RightEdge
            y: topToolbar.height
            interactive: false

            onClosed: {
                // update state for each category
                mapToolRow.state = "clear";
                alertToolRow.state = "clear";
                viewshedTool.state = "clear";
                reportToolRow.state = "clear";
                markupToolRow.state = "clear";
            }

            Rectangle {
                id: toolRect
                anchors.fill: parent

                states: [
                    State {
                        name: "basemap"
                        PropertyChanges {
                            target: basemapsTool
                            visible: true
                        }
                    },
                    State {
                        name: "data"
                        PropertyChanges {
                            target: addLocalDataTool
                            visible: true
                        }
                    },
                    State {
                        name: "message"
                        PropertyChanges {
                            target: messageFeedsTool
                            visible: true
                        }
                    },
                    State {
                        name: "open scene"
                        PropertyChanges {
                            target: openSceneTool
                            visible: true
                        }
                    }
                ]

                OpenSceneTool {
                    id: openSceneTool
                    anchors.fill: parent
                    onSceneSelected: closed();
                    visible: false
                    onClosed: drawer.close();
                    onResetToDefaultSelected: resetToDefaultScene();
                }

                BasemapPicker {
                    id: basemapsTool
                    anchors.fill: parent
                    onBasemapSelected: closed();
                    visible: false
                    onClosed: drawer.close();
                }

                AddLocalData {
                    id: addLocalDataTool
                    anchors.fill: parent
                    showDataConnectionPane: true
                    visible: false
                    onClosed: drawer.close();
                }

                MessageFeeds {
                    id: messageFeedsTool
                    anchors.fill: parent
                    visible: false
                    onClosed: drawer.close();
                }
            }
        }
    }

    About {
        id: aboutTool
        anchors.fill: parent
        visible: false
    }

    IdentifyController {
        id: identifyController
        active: mapToolRow.state === "Identify"

        onActiveChanged: {
            if (!active)
            {
                identifyResults.dismiss();
                mapToolRow.state = "clear";
            }
        }

        onPopupManagersChanged: {
            if (popupManagers.length > 0) {
                identifyResults.popupManagers = popupManagers;
                identifyResults.visible = true;
            }
        }
    }

    onErrorOccurred: {
        // if the parent is null, the app is in a loading state and not yet ready to display errors
        msgDialog.title = "Error"
        if (parent) {
            msgDialog.informativeText = message;
            msgDialog.open();
        }
        else {
            msgDialog.informativeText += message + "\n"
        }
    }

    onParentChanged: {
        if (parent && msgDialog.informativeText.length > 0)
            msgDialog.open();
    }

    Options {
        id: optionsTool
        anchors.fill: parent
        visible: false
    }

    DsaMessageDialog {
        id: msgDialog
        title: "Error"
    }

    BusyIndicator {
        anchors.centerIn: parent
        visible: identifyController.busy
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    DsaMessageDialog {
        id: clearDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: clearDialogAccepted();
    }

    function showClearDialog(title) {
        clearDialog.informativeText = title;
        clearDialog.open();
    }

    DsaMessageDialog {
        id: closeDialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: closeDialogAccepted();
    }

    function showCloseDialog(title) {
        closeDialog.informativeText = title;
        closeDialog.open();
    }

    DsaInputDialog {
        id: inputDialog
        onAccepted: {
            inputDialogAccepted(inputDialog.userInputText, inputDialog.titleText);
        }
    }

    function showInputDialog(titleText, labelText, placeholderText) {
        inputDialog.titleText = titleText;
        inputDialog.inputLabel = labelText;
        inputDialog.inputPlaceholderText = placeholderText;
        inputDialog.open();
    }

    DsaYesNoDialog {
        id: markupDialog
        property string path

        onAccepted: markupLayerReceived(path, true);
        onRejected: markupLayerReceived(path, false);
    }
}

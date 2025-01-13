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
import Esri.ArcGISRuntime.OpenSourceApps.Handheld
import Esri.ArcGISRuntime.Toolkit as Toolkit

Handheld {
    id: appRoot
    width: 320 * scaleFactor
    height: 480 * scaleFactor

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
        menuVisible: false

        HomeToolRow  {
            id: homeToolRow
            isMobile: true
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 10 * scaleFactor
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
            bottom: categoryToolbar.top
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
                left: sceneView.left
                margins: hudMargins
            }
            radius: hudRadius
            opacity: hudOpacity
        }

        FollowHud {
            id: followHud
            anchors {
                bottom: currentLocation.visible ? currentLocation.top : sceneView.attributionTop
                left: sceneView.left
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
                right: parent.right
                bottom: sceneView.attributionTop
                bottomMargin: 10 * scaleFactor
                rightMargin: parent.height < navTool.height * 1.6 ? 60 * scaleFactor : 15 * scaleFactor
            }
            autoHide: false
            width: DsaStyles.primaryIconSize * scaleFactor
            height: width
        }

        CoordinateConversionToolProxy {
            id: dsaCoordinateController
            inInputMode: coordinateConversion.inInputMode
        }

        Toolkit.CoordinateConversion {
            id: coordinateConversion
            anchors {
                bottom: followHud.visible ? followHud.top : currentLocation.top
                left: sceneView.left
                right: navTool.left
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

        TableOfContents {
            id: tableOfContentsTool
            anchors {
                left: parent.left
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: true

            onClosed: {
                mapToolRow.tocIconSelected = false;
                visible = false;
                mapToolRow.state = "clear";
            }
        }

        AlertList {
            id: alertsTool
            anchors {
                left: parent.left
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: true
            onClosed: {
                visible = false;
                alertToolRow.state = "clear";
            }
        }

        AlertConditionsTool {
            id: alertConditionsTool
            anchors {
                left: parent.left
                top: parent.top
                bottom: sceneView.attributionTop
            }
            width: drawer.width
            visible: false
            isMobile: true
            onClosed: {
                visible = false;
                alertToolRow.state = "clear";
            }

            onPickModeChanged: {
                if (!toolActive)
                    return;

                if (pickMode)
                    visible = false;
                else
                    visible = true;
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
            isMobile: true
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
            isMobile: true
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

            onPickModeChanged: {
                if (!toolActive)
                    return;

                if (pickMode)
                    visible = false;
                else
                    visible = true;
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
                left: sceneView.left
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
            width: parent.width
            height: parent.height - topToolbar.height
            edge: Qt.BottomEdge
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

    CategoryToolbarRow {
        id: categoryToolbar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: DsaStyles.categoryToolbarWidth * scaleFactor
        appTitle: "DSA - H"
    }

    IdentifyController {
        id: identifyController
        active: mapToolRow.state === "Identify"

        onActiveChanged: {
            if (!active)
                identifyResults.dismiss();
        }

        onPopupManagersChanged: {
            if (popupManagers.length > 0) {
                identifyResults.popupManagers = popupManagers;
                identifyResults.visible = true;
            }
        }
    }

    Options {
        id: optionsTool
        anchors.fill: parent
        visible: false
    }

    About {
        id: aboutTool
        anchors.fill: parent

        visible: false
    }

    onErrorOccurred: {
        // if the parent is null, the app is in a loading state and not yet ready to display errors
        if (parent) {
            msgDialog.informativeText = message;
            msgDialog.open();
        }
        else {
            msgDialog.informativeText += message + "\n";
        }
    }

    onParentChanged: {
        if (parent && msgDialog.informativeText.length > 0)
            msgDialog.open();
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

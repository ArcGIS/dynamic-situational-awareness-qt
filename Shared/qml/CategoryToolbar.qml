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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import Esri.ArcGISRuntime.OpenSourceApps.DSA
import QtQuick.Controls.Material

Item {
    id: categoryToolbar

    property string appTitle
    property string titleText
    property real scaleFactor: (Screen.logicalPixelDensity * 25.4) / (Qt.platform.os === "windows" || Qt.platform.os === "linux" ? 96 : 72)
    signal settingsClicked()
    signal aboutClicked()

    Rectangle {
        anchors.fill: parent
        color: Material.background
        opacity: 0.9
    }

    state: "home"
    states: [
        State {
            name: "home"
            PropertyChanges {
                target: homeIcon
                selected: true
            }
            PropertyChanges {
                target: categoryToolbar
                titleText: appTitle
            }
        },
        State {
            name: "map"
            PropertyChanges {
                target: mapIcon
                selected: true
            }
            PropertyChanges {
                target: categoryToolbar
                titleText: "Map"
            }
        },
        State {
            name: "reports"
            PropertyChanges {
                target: reportsIcon
                selected: true
            }
            PropertyChanges {
                target: categoryToolbar
                titleText: "Create Report"
            }
        },
        State {
            name: "analysis"
            PropertyChanges {
                target: analysisIcon
                selected: true
            }
            PropertyChanges {
                target: categoryToolbar
                titleText: "Analysis"
            }
        },
        State {
            name: "alerts"
            PropertyChanges {
                target: alertIcon
                selected: true
            }
            PropertyChanges {
                target: categoryToolbar
                titleText: "Alerts"
            }
        },
        State {
            name: "markup"
            PropertyChanges {
                target: markupIcon
                selected: true
            }
            PropertyChanges {
                target: categoryToolbar
                titleText: "Markup"
            }
        },
        State {
            name: "configurations"
            PropertyChanges {
                target: configurationsIcon
                selected: true
            }
            PropertyChanges {
                target: categoryToolbar
                titleText: "Configurations"
            }
        }
    ]
}

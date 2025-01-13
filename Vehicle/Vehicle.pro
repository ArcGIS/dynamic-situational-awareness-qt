################################################################################
#  Copyright 2012-2018 Esri
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
################################################################################

TARGET = DSA_Vehicle_Qt
TEMPLATE = app

QT += core gui opengl network positioning sensors qml quick xml
CONFIG += c++17

ARCGIS_RUNTIME_VERSION = 200.5.0
DEFINES += ARCGIS_MAPS_SDK_VERSION=$$ARCGIS_RUNTIME_VERSION
include($$PWD/../Shared/build/arcgisruntime.pri)

include($$PWD/../3rdparty/zlib_minizip_ng.pri)

INCLUDEPATH += $$PWD/../Shared/ \
    $$PWD/../Shared/alerts \
    $$PWD/../Shared/analysis \
    $$PWD/../Shared/messages \
    $$PWD/../Shared/packages \
    $$PWD/../Shared/utilities \
    $$PWD/../Shared/markup \
    $$PWD/../Shared/configurations

HEADERS += \
    AppInfo.h \
    Vehicle.h \
    VehicleStyles.h \
    $$files($$PWD/../Shared/*.h) \
    $$files($$PWD/../Shared/alerts/*.h) \
    $$files($$PWD/../Shared/analysis/*.h) \
    $$files($$PWD/../Shared/messages/*.h) \
    $$files($$PWD/../Shared/packages/*.h) \
    $$files($$PWD/../Shared/utilities/*.h) \
    $$files($$PWD/../Shared/markup/*.h) \
    $$files($$PWD/../Shared/configurations/*.h)

SOURCES += \
    main.cpp \
    Vehicle.cpp \
    VehicleStyles.cpp \
    $$files($$PWD/../Shared/*.cpp) \
    $$files($$PWD/../Shared/alerts/*.cpp) \
    $$files($$PWD/../Shared/analysis/*.cpp) \
    $$files($$PWD/../Shared/messages/*.cpp) \
    $$files($$PWD/../Shared/packages/*.cpp) \
    $$files($$PWD/../Shared/utilities/*.cpp) \
    $$files($$PWD/../Shared/markup/*.cpp) \
    $$files($$PWD/../Shared/configurations/*.cpp)

RESOURCES += \
    qml/qml.qrc \
    ../Shared/Resources/Resources.qrc \
    ../Shared/Resources/application.qrc \
    ../Shared/qml/shared_qml.qrc

QML_IMPORT_PATH += $$PWD/../Shared/qml

!android {
  PRECOMPILED_HEADER = $$PWD/../Shared/pch.hpp
  CONFIG += precompile_header
}

#-------------------------------------------------------------------------------

win32 {
    include (Win/Win.pri)
}

macx {
    include (Mac/Mac.pri)
}

ios {
    include (iOS/iOS.pri)
}

android {
    include (Android/Android.pri)
}

DISTFILES += \
    Resources/qtquickcontrols2.conf \
    ../qdoc/dsa-overview.qdoc

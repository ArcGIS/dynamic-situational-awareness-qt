
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

// PCH header
#include "pch.hpp"

// C++ API headers
#include "PopupManager.h"
#include "SceneQuickView.h"

// Qt headers
#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QObject>
#include <QQmlEngine>
#include <QQuickView>
#include <QSettings>

// Toolkit headers
#include "Esri/ArcGISRuntime/Toolkit/register.h"

// DSA headers
#include "AddLocalDataController.h"
#include "AlertConditionsController.h"
#include "AlertListController.h"
#include "AnalysisListController.h"
#include "AppInfo.h"
#include "BasemapPickerController.h"
#include "ConfigurationController.h"
#include "ContextMenuController.h"
#include "CoordinateConversionToolProxy.h"
#include "DsaResources.h"
#include "FollowPositionController.h"
#include "Handheld.h"
#include "HandheldStyles.h"
#include "IdentifyController.h"
#include "LineOfSightController.h"
#include "LocationController.h"
#include "LocationTextController.h"
#include "MarkupController.h"
#include "MessageFeedsController.h"
#include "NavigationController.h"
#include "ObservationReportController.h"
#include "OpenMobileScenePackageController.h"
#include "OptionsController.h"
#include "PackageImageProvider.h"
#include "TableOfContentsController.h"
#include "ViewedAlertsController.h"
#include "ViewshedController.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

//------------------------------------------------------------------------------

#define kSettingsFormat                 QSettings::IniFormat

//------------------------------------------------------------------------------

#define kArgShowName                    "show"
#define kArgShowValueName               "showOption"
#define kArgShowDescription             "Show option maximized | minimized | fullscreen | normal | default"
#define kArgShowDefault                 "show"

#define kShowMaximized                  "maximized"
#define kShowMinimized                  "minimized"
#define kShowFullScreen                 "fullscreen"
#define kShowNormal                     "normal"
#define STRINGIZE(x) #x
#define QUOTE(x) STRINGIZE(x)

//------------------------------------------------------------------------------

using namespace Esri::ArcGISRuntime;

QObject* dsaStylesProvider(QQmlEngine* engine, QJSEngine* scriptEngine);
QObject* dsaResourcesProvider(QQmlEngine* engine, QJSEngine* scriptEngine);

int main(int argc, char *argv[])
{
  QGuiApplication app(argc, argv);

  QCoreApplication::setApplicationName(kApplicationName);
  QCoreApplication::setApplicationVersion(kApplicationVersion);
  QCoreApplication::setOrganizationName(kOrganizationName);
#ifdef Q_OS_MAC
  QCoreApplication::setOrganizationDomain(kOrganizationName);
#else
  QCoreApplication::setOrganizationDomain(kOrganizationDomain);
#endif
  QSettings::setDefaultFormat(kSettingsFormat);

  // Register the map view for QML
  qmlRegisterType<SceneQuickView>("Esri.ArcGISRuntime.OpenSourceApps.Handheld", 1, 1, "SceneView");
  qRegisterMetaType<PopupManager*>("PopupManager*");

  // Register the Handheld (QQuickItem) for QML
  qmlRegisterType<Dsa::Handheld::Handheld>("Esri.ArcGISRuntime.OpenSourceApps.Handheld", 1, 1, "Handheld");
  qmlRegisterType<Dsa::BasemapPickerController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "BasemapPickerController");
  qmlRegisterType<Dsa::AddLocalDataController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "AddLocalDataController");
  qmlRegisterType<Dsa::LocationController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "LocationController");
  qmlRegisterType<Dsa::MessageFeedsController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "MessageFeedsController");
  qmlRegisterType<Dsa::FollowPositionController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "FollowPositionController");
  qmlRegisterType<Dsa::TableOfContentsController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "TableOfContentsController");
  qmlRegisterType<Dsa::NavigationController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "NavigationController");
  qmlRegisterType<Dsa::MarkupController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "MarkupController");
  qmlRegisterType<Dsa::ViewshedController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "ViewshedController");
  qmlRegisterType<Dsa::OptionsController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "OptionsController");
  qmlRegisterSingletonType<Dsa::Handheld::HandheldStyles>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "DsaStyles", &dsaStylesProvider);
  qmlRegisterSingletonType<Dsa::DsaResources>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "DsaResources", &dsaResourcesProvider);
  qmlRegisterType<Dsa::IdentifyController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "IdentifyController");
  qmlRegisterType<Dsa::AlertListController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "AlertListController");
  qmlRegisterType<Dsa::ViewedAlertsController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "ViewedAlertsController");
  qmlRegisterType<Dsa::LocationTextController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "LocationTextController");
  qmlRegisterType<Dsa::AlertConditionsController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "AlertConditionsController");
  qmlRegisterType<Dsa::LineOfSightController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "LineOfSightController");
  qmlRegisterType<Dsa::ContextMenuController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "ContextMenuController");
  qmlRegisterType<Dsa::AnalysisListController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "AnalysisListController");
  qmlRegisterType<Dsa::ObservationReportController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "ObservationReportController");
  qmlRegisterType<Dsa::OpenMobileScenePackageController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "OpenMobileScenePackageController");
  qmlRegisterType<Dsa::CoordinateConversionToolProxy>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "CoordinateConversionToolProxy");
  qmlRegisterType<Dsa::ConfigurationController>("Esri.ArcGISRuntime.OpenSourceApps.DSA", 1, 1, "ConfigurationController");

  // Initialize application view
  QQuickView view;
  view.setResizeMode(QQuickView::SizeRootObjectToView);

  view.engine()->addImageProvider(QStringLiteral("packages"), new Dsa::PackageImageProvider);

  Esri::ArcGISRuntime::Toolkit::registerComponents(*view.engine());

#ifndef DEPLOYMENT_BUILD
  // Add the import Path
  view.engine()->addImportPath(QDir(QCoreApplication::applicationDirPath()).filePath("qml"));
  QString arcGISRuntimeImportPath = QUOTE(ARCGIS_RUNTIME_IMPORT_PATH);

#if defined(LINUX_PLATFORM_REPLACEMENT)
  // on some linux platforms the string 'linux' is replaced with 1
  // fix the replacement paths which were created
  QString replaceString = QUOTE(LINUX_PLATFORM_REPLACEMENT);
  arcGISRuntimeImportPath = arcGISRuntimeImportPath.replace(replaceString, "linux", Qt::CaseSensitive);
#endif

  // Add the Runtime and Extras path
  view.engine()->addImportPath(arcGISRuntimeImportPath);
#endif // DEPLOYMENT_BUILD

  // To quit via Qt.quit() from QML, you must connect the QQmlEngine::quit()
  // signal to the QCoreApplication::quit() slot
  QObject::connect(view.engine(), &QQmlEngine::quit, &QCoreApplication::quit);

  // Set the source
  view.setSource(QUrl(kApplicationSourceUrl));

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
  // Process command line
  QCommandLineOption showOption(kArgShowName, kArgShowDescription, kArgShowValueName, kArgShowDefault);

  QCommandLineParser commandLineParser;

  commandLineParser.setApplicationDescription(kApplicationDescription);
  commandLineParser.addOption(showOption);
  commandLineParser.addHelpOption();
  commandLineParser.addVersionOption();
  commandLineParser.process(app);

  // Show app window

  auto showValue = commandLineParser.value(kArgShowName).toLower();

  if (showValue.compare(kShowMaximized) == 0)
  {
    view.showMaximized();
  }
  else if (showValue.compare(kShowMinimized) == 0)
  {
    view.showMinimized();
  }
  else if (showValue.compare(kShowFullScreen) == 0)
  {
    view.showFullScreen();
  }
  else if (showValue.compare(kShowNormal) == 0)
  {
    view.showNormal();
  }
  else
  {
    view.show();
  }

#else
  view.show();
#endif

  return app.exec();
}

//------------------------------------------------------------------------------

// qml dsa styles provider
QObject* dsaStylesProvider(QQmlEngine* engine, QJSEngine*)
{
  static Dsa::Handheld::HandheldStyles* dsaStyles = new Dsa::Handheld::HandheldStyles(engine);
  return dsaStyles;
}

QObject* dsaResourcesProvider(QQmlEngine* engine, QJSEngine*)
{
  static Dsa::DsaResources* dsaResources = new Dsa::DsaResources(engine);
  dsaResources->setArcGISMapsSDKVersion(QUOTE(ARCGIS_MAPS_SDK_VERSION));
  return dsaResources;
}

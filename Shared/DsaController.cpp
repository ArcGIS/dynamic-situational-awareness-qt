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

#include "DsaController.h"

// C++ API headers
#include "Camera.h"
#include "Error.h"
#include "GeoView.h"
#include "Layer.h"
#include "LayerListModel.h"
#include "Scene.h"
#include "SelectionProperties.h"
#include "Viewpoint.h"

// Toolkit headers
#include "CoordinateConversionConstants.h"

// Qt headers
#include <QDir>
#include <QFileInfo>
#include <QFuture>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

// DSA headers
#include "AlertConstants.h"
#include "AppConstants.h"
#include "BasemapPickerController.h"
#include "ContextMenuController.h"
#include "DsaUtility.h"
#include "LayerCacheManager.h"
#include "MessageFeedConstants.h"
#include "OpenMobileScenePackageController.h"
#include "ToolManager.h"
#include "ToolResourceProvider.h"

using namespace Esri::ArcGISRuntime;

namespace Dsa {

namespace
{

bool readJsonFile(QIODevice& device, QSettings::SettingsMap& map);
bool writeJsonFile(QIODevice& device, const QSettings::SettingsMap& map);
Viewpoint viewpointFromJson(const QJsonObject& initialLocation);
QJsonObject defaultViewpoint();

} // namespace

/*!
  \class Dsa::DsaController
  \inmodule Dsa
  \inherits QObject
  \brief This is the controller for the DSA app. It is responsible for connecting the
  view (such as the \l Esri::ArcGISRuntime::GeoView) to the business logic of the app.

  For example, signals from the view are passed to the \l ToolResourceProvider
  where they can be accessed by the list of \l AbstractTool objects stored in
  the \l Esri::ArcGISRuntime::ToolManager.

  This type is also responsible for reading and writing app configuration details to
  a JSON settings file. Information in the JSON file is sent to each tool as a set of
  properties.
 */

/*!
  \brief Constructor for a model taking an optional \a parent.
 */
DsaController::DsaController(QObject* parent):
  QObject(parent),
  m_scene(new Scene(this)),
  m_jsonFormat(QSettings::registerFormat("json", &readJsonFile, &writeJsonFile)),
  m_conflictingToolNames{QStringLiteral("Alert Conditions"),
                         QStringLiteral("Markup Tool"),
                         QStringLiteral("viewshed"),
                         QStringLiteral("Observation Report")}
{
  // setup config settings
  setupConfig();
  m_scene->setInitialViewpoint(viewpointFromJson(defaultViewpoint()));
  m_dataPath = m_dsaSettings["RootDataDirectory"].toString();

  connect(m_scene, &Scene::errorOccurred, this, &DsaController::onError);

  connect(ToolResourceProvider::instance(), &ToolResourceProvider::sceneChanged, this, [this, firstLoad{true}]() mutable
  {
    m_scene = ToolResourceProvider::instance()->scene();

    // Here we set up the initial location of the scene fron the config. We do this on the first scene
    // we load when the application starts up. On any subsequent scenes that the user loads
    // via the UI, we take this new initial location as the source of truth from then on and
    // write it to file.
    if (!m_scene)
      return;

    updateInitialLocationOnSceneChange(firstLoad);
    firstLoad = false;
  });
}

/*!
  \brief Destructor.
 */
DsaController::~DsaController()
{
  // save the settings
  saveSettings();
}

/*!
  \brief Returns the Esri::ArcGISRuntime::Scene used by the app.
 */
Scene* DsaController::scene() const
{
  return m_scene;
}

/*!
  \brief Initialize the app with the Esri::ArcGISRuntime::GeoView \a geoView.

  When this method is called, the various tools in the app are initialized.
 */
void DsaController::init(GeoView* geoView)
{
  ToolResourceProvider::instance()->setGeoView(geoView);

  bool hasActiveScene = false;
  auto openScenePackageTool = ToolManager::instance().tool<OpenMobileScenePackageController>();
  if (openScenePackageTool)
  {
    openScenePackageTool->setProperties(m_dsaSettings);
    hasActiveScene = openScenePackageTool->hasActiveScene();
  }

  m_cacheManager = new LayerCacheManager(this);
  if (openScenePackageTool && !openScenePackageTool->packageDataPath().isEmpty())
    m_cacheManager->addExcludedPath(openScenePackageTool->packageDataPath());

  // Only set the default scene if the scene package tool hasn't set a scene.
  if (!hasActiveScene)
  {
    if (!m_dsaSettings.contains(AppConstants::INITIALLOCATION_PROPERTYNAME))
    {
      // While a scene change would normally write out the viewpoint
      // to config if/when it is missing, this here is a special case. We
      // want the config file to contain a nice human-editable initial location
      // using a distance measure. Extracting the viewpoint from the scene
      // will give us a calculated viewpoint with no distance measure, so we have
      // to set this manually.
      m_dsaSettings[AppConstants::INITIALLOCATION_PROPERTYNAME] = defaultViewpoint();
    }

    ToolResourceProvider::instance()->setScene(m_scene);
  }
  // set the selection color for graphics and features
  geoView->setSelectionProperties(SelectionProperties(Qt::red));

  // connect all tool signals
  for(AbstractTool* abstractTool : ToolManager::instance())
  {
    if (!abstractTool)
      continue;

    abstractTool->setProperties(m_dsaSettings);

    connect(abstractTool, &AbstractTool::errorOccurred, this, &DsaController::onError);
    connect(abstractTool, &AbstractTool::propertyChanged, this, &DsaController::onPropertyChanged);

    if (abstractTool->metaObject()->indexOfSignal("toolErrorOccurred(QString,QString)") != -1)
      connect(abstractTool, SIGNAL(toolErrorOccurred(QString,QString)), this, SLOT(onToolError(QString, QString)));

    // certain tools can conflict - for example, those which interact directly with the view
    if (!isConflictingTool(abstractTool->toolName()))
      continue;

    // whenever a conflciting tool is activated, deactivate all of the other conflicting tools
    connect(abstractTool, &AbstractTool::activeChanged, this, [this, abstractTool]()
    {
      bool anyActive = false;

      // if this tool is becoming active, deactivate all conflicting tools
      if (abstractTool->isActive())
      {
        anyActive = true;
        auto toolsIt = ToolManager::instance().begin();
        auto toolsEnd = ToolManager::instance().end();
        for (; toolsIt != toolsEnd; ++toolsIt)
        {
          AbstractTool* candidateTool = *toolsIt;
          if (!candidateTool)
            continue;

          if (candidateTool->toolName() == abstractTool->toolName())
            continue;

          if (!isConflictingTool(candidateTool->toolName()))
            continue;

          if (candidateTool->isActive())
            candidateTool->setActive(false);
        }
      }
      // otherwise, if this tool is becoming deactivated, check if any of the conflicting tools are now active
      else
      {

        auto toolsIt = ToolManager::instance().begin();
        auto toolsEnd = ToolManager::instance().end();
        for (; toolsIt != toolsEnd; ++toolsIt)
        {
          AbstractTool* candidateTool = *toolsIt;
          if (!candidateTool)
            continue;

          if (!isConflictingTool(candidateTool->toolName()))
            continue;

          if (!candidateTool->isActive())
            continue;

          anyActive = true;
          break;
        }
      }

      // The context menu should only be active when the other tools which interact with the view are not
      ContextMenuController* contextMenu = ToolManager::instance().tool<ContextMenuController>();
      if (contextMenu && contextMenu->isActive() == anyActive)
        contextMenu->setActive(!anyActive);
    });
  }
}

/*!
 * \brief Slot to handle an ArcGISRuntime Error \a error.
 */
void DsaController::onError(const Error& error)
{
  qDebug() << "Error" << error.message() << error.additionalMessage();
  emit errorOccurred(error.message(), error.additionalMessage());
}

/*!
 * \brief Slot to handle an \a errorMessage (with an \a additionalMessage) from an \l AbstractTool.
 */
void DsaController::onToolError(const QString& errorMessage, const QString& additionalMessage)
{
  qDebug() << "Error" << errorMessage << additionalMessage;
  emit errorOccurred(errorMessage, additionalMessage);
}

/*!
 * \brief Slot to handle a change to the \a propertyName to the new value \l propertyValue.
 */
void DsaController::onPropertyChanged(const QString& propertyName, const QVariant& propertyValue)
{
  if (m_dsaSettings.value(propertyName) == propertyValue)
    return;

  m_dsaSettings.insert(propertyName, propertyValue);
  // save the settings
  saveSettings();

  // inform tools of the change
  auto it = ToolManager::instance().begin();
  auto itEnd = ToolManager::instance().end();
  for (;it != itEnd; ++it)
  {
    AbstractTool* tool = *it;
    if (!tool)
      continue;

    disconnect(tool, &AbstractTool::propertyChanged,this, &DsaController::onPropertyChanged);
    tool->setProperties(m_dsaSettings);
    connect(tool, &AbstractTool::propertyChanged, this, &DsaController::onPropertyChanged);
  }
}

/*!
  \brief Creates a new scene with the default basemap and default surface.
 */
void DsaController::resetToDefaultScene()
{
  // obtain other required tools
  auto basemapTool = ToolManager::instance().tool<BasemapPickerController>();
  if (!basemapTool)
    return;

  // create scene
  Scene* newScene = new Scene(this);
  newScene->setInitialViewpoint(viewpointFromJson(defaultViewpoint()));

  // set on sceneview
  ToolResourceProvider::instance()->setScene(newScene);

  // add basemap
  basemapTool->selectInitialBasemap();

  // add elevation
  m_cacheManager->addElevation(m_dsaSettings);

  // remove any layers if automatically added
  if (!newScene->operationalLayers()->isEmpty())
  {
    for (auto layer : *newScene->operationalLayers())
    {
      delete layer;
    }
    newScene->operationalLayers()->clear();
  }

  // clear current scene and index in properties
  onPropertyChanged(AppConstants::SCENEINDEX_PROPERTYNAME, -1);
  onPropertyChanged(AppConstants::CURRENTSCENE_PROPERTYNAME, "");
  onPropertyChanged(AppConstants::LAYERS_PROPERTYNAME, QJsonArray().toVariantList());
  m_cacheManager->setProperties(m_dsaSettings);
}

/*!
 * \internal
 */
void DsaController::setupConfig()
{
  // create the default settings map
  createDefaultSettings();

  // get the app config
  m_configFilePath = QString("%1/%2").arg(m_dsaSettings["RootDataDirectory"].toString(), QStringLiteral("DsaAppConfig.json"));

  // If the config file does not exist, create it, and set all of the defaults
  if (!QFileInfo::exists(m_configFilePath))
  {
    saveSettings();
  }
  else
  {
    // Open the config file, get settings, set them to the application controller
    QSettings settings(m_configFilePath, m_jsonFormat);
    const QStringList allKeys = settings.allKeys();

    // get the values from the config, and write to the settings map
    for (const QString& key : allKeys)
      m_dsaSettings[key] = settings.value(key);
  }
}

/*! \brief internal
 *
 * Writes the default local data paths as JSON to the settings map.
 */
void DsaController::writeDefaultLocalDataPaths()
{
  const QString rootDir = m_dsaSettings["RootDataDirectory"].toString();
  QStringList pathsList{QString("%1/").arg(rootDir),
        QString("%1/OperationalData").arg(rootDir)};
  m_dsaSettings[QStringLiteral("LocalDataPaths")] = pathsList;
}

/*! \brief internal
 *
 * Writes the default Alert Conditions as JSON to the settings map.
 */
void DsaController::writeDefaultConditions()
{
  QJsonArray allConditionsJson;

  // Add a condition "Distress" when an object from the Friendly Tracks Land feed has attribute status911 = 1
  QJsonObject conditionJson;
  conditionJson.insert(AlertConstants::CONDITION_NAME, QStringLiteral("Distress"));
  conditionJson.insert(AlertConstants::CONDITION_LEVEL, static_cast<int>(AlertLevel::Critical));
  conditionJson.insert(AlertConstants::CONDITION_TYPE, AlertConstants::attributeEqualsAlertConditionType());
  conditionJson.insert(AlertConstants::CONDITION_SOURCE, QStringLiteral("Friendly Tracks - Land"));
  QJsonObject queryObject;
  queryObject.insert(AlertConstants::ATTRIBUTE_NAME, QStringLiteral("status911"));
  conditionJson.insert(AlertConstants::CONDITION_QUERY, queryObject);
  conditionJson.insert(AlertConstants::CONDITION_TARGET, "1");
  allConditionsJson.append(conditionJson);
  m_dsaSettings.insert(AlertConstants::ALERT_CONDITIONS_PROPERTYNAME, allConditionsJson.toVariantList());
}

/*! \brief internal
 *
 * Writes the default message feeds to the settings map.
 */
void DsaController::writeDefaultMessageFeeds()
{
  m_dsaSettings[MessageFeedConstants::MESSAGE_FEED_UDP_PORTS_PROPERTYNAME] = QStringList { QString("45678"), QString("45679") };

  QJsonArray messageFeedsJson;

  QJsonObject cotMessageFeedJson;
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("SA Events"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("cot"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("mil2525c"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("saevents.png"));
  cotMessageFeedJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(cotMessageFeedJson);

  QJsonObject friendlyTracksLandJson;
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Friendly Tracks - Land"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("position_report_land"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("mil2525c"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("friendlytracks.png"));
  friendlyTracksLandJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(friendlyTracksLandJson);

  QJsonObject friendlyTracksAirJson;
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Friendly Tracks - Air"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("position_report_air"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("mil2525c"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("friendlytracks-air.png"));
  friendlyTracksAirJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("absolute"));
  messageFeedsJson.append(friendlyTracksAirJson);

  QJsonObject spotRepJson;
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Observation Reports"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("spotrep"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("observation1600.png"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("observation1600.png"));
  spotRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(spotRepJson);

  QJsonObject sitRepJson;
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Situation Reports"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("sitrep"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("sitrep1600.png"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("sitrep1600.png"));
  sitRepJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(sitRepJson);

  QJsonObject eodReportsJson;
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("EOD Reports"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("eod"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("eod1600.png"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("eod1600.png"));
  eodReportsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(eodReportsJson);

  QJsonObject sensorObsJson;
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_NAME, QStringLiteral("Sensor Observations"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_TYPE, QStringLiteral("sensor_obs"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_THUMBNAIL, QStringLiteral("sensorobs1600.png"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_RENDERER, QStringLiteral("sensorobs1600.png"));
  sensorObsJson.insert(MessageFeedConstants::MESSAGE_FEEDS_PLACEMENT, QStringLiteral("draped"));
  messageFeedsJson.append(sensorObsJson);
  m_dsaSettings[MessageFeedConstants::MESSAGE_FEEDS_PROPERTYNAME] = messageFeedsJson;

  QJsonObject locationBroadcastJson;
  locationBroadcastJson.insert(MessageFeedConstants::LOCATION_BROADCAST_CONFIG_MESSAGE_TYPE, QStringLiteral("position_report_land"));
  locationBroadcastJson.insert(MessageFeedConstants::LOCATION_BROADCAST_CONFIG_PORT, 45679);
  m_dsaSettings[MessageFeedConstants::LOCATION_BROADCAST_CONFIG_PROPERTYNAME] = locationBroadcastJson;

  QJsonObject observationReportJson;
  observationReportJson.insert(MessageFeedConstants::OBSERVATION_REPORT_CONFIG_PORT, 45679);
  m_dsaSettings[MessageFeedConstants::OBSERVATION_REPORT_CONFIG_PROPERTYNAME] = observationReportJson;
}

/*! \brief internal
 *
 * Returns \c true if this tool is considered to conlfict with other tools.
 *
 * For example, this could be tools which interact directly with the view.
 */
bool DsaController::isConflictingTool(const QString& toolName) const
{
  return m_conflictingToolNames.contains(toolName);
}

/*! \brief internal
 *
 * This creates the default values for the config file. If the app
 * starts and there is no config file, it will create one, and write
 * the following values to the file.
 */
void DsaController::createDefaultSettings()
{
  // setup the defaults
  m_dsaSettings["RootDataDirectory"] = DsaUtility::activeConfigurationPath();
  m_dsaSettings[AppConstants::USERNAME_PROPERTYNAME] = QHostInfo::localHostName();
  m_dsaSettings["BasemapDirectory"] = QString("%1/BasemapData").arg(m_dsaSettings["RootDataDirectory"].toString());
  m_dsaSettings["ElevationDirectory"] = QString("%1/ElevationData").arg(m_dsaSettings["RootDataDirectory"].toString());
  m_dsaSettings["SimulationDirectory"] = QString("%1/SimulationData").arg(m_dsaSettings["RootDataDirectory"].toString());
  m_dsaSettings["ResourceDirectory"] = QString("%1/ResourceData").arg(m_dsaSettings["RootDataDirectory"].toString());
  writeDefaultLocalDataPaths();
  m_dsaSettings["DefaultBasemap"] = QStringLiteral("topographic");
  m_dsaSettings["DefaultElevationSource"] = QString("%1/CaDEM.tpk").arg(m_dsaSettings["ElevationDirectory"].toString());
  m_dsaSettings["GpxFile"] = QString("%1/MontereyMounted.gpx").arg(m_dsaSettings["SimulationDirectory"].toString());
  m_dsaSettings["SimulateLocation"] = QStringLiteral("true");
  writeDefaultMessageFeeds();
  m_dsaSettings["CoordinateFormat"] = Esri::ArcGISRuntime::Toolkit::CoordinateConversionConstants::MGRS_FORMAT;
  m_dsaSettings[AppConstants::UNIT_OF_MEASUREMENT_PROPERTYNAME] = AppConstants::UNIT_METERS;
  m_dsaSettings["UseGpsForElevation"] = QStringLiteral("true");
  QJsonObject markupJson;
  markupJson.insert(QStringLiteral("port"), 45680);
  m_dsaSettings[QStringLiteral("MarkupConfig")] = markupJson;
  writeDefaultConditions();
  m_dsaSettings[OpenMobileScenePackageController::PACKAGE_DIRECTORY_PROPERTYNAME] = QString("%1/Packages").arg(m_dsaSettings["RootDataDirectory"].toString());
}

/*!
 * \brief Save the app properties to a custom JSON QSettings file.
 */
void DsaController::saveSettings()
{
  QSettings settings(m_configFilePath, m_jsonFormat);

  auto it = m_dsaSettings.cbegin();
  auto itEnd = m_dsaSettings.cend();
  for (; it != itEnd; ++it)
    settings.setValue(it.key(), it.value());
}

void DsaController::writeInitialLocation(const Viewpoint& viewpoint)
{
  if (viewpoint.isEmpty())
    return;

  auto initialCamera = viewpoint.camera();
  if (initialCamera.isEmpty())
    return;

  QJsonObject initialLocationJson;
  const QString centerString = initialCamera.location().toJson();
  const QJsonDocument centerDoc = QJsonDocument::fromJson(centerString.toLatin1());
  initialLocationJson.insert( QStringLiteral("center"), centerDoc.object());
  initialLocationJson.insert( QStringLiteral("distance"), 0);
  initialLocationJson.insert( QStringLiteral("heading"), initialCamera.heading());
  initialLocationJson.insert( QStringLiteral("pitch"), initialCamera.pitch());
  initialLocationJson.insert( QStringLiteral("roll"), initialCamera.roll());

  m_dsaSettings[AppConstants::INITIALLOCATION_PROPERTYNAME] = initialLocationJson.toVariantMap();
}

Viewpoint DsaController::readInitialLocation()
{
  return viewpointFromJson(m_dsaSettings[AppConstants::INITIALLOCATION_PROPERTYNAME].toJsonObject());
}

void DsaController::updateInitialLocationOnSceneChange(bool isInitialization)
{
  if (!m_scene)
    return;

  auto geoView = ToolResourceProvider::instance()->geoView();
  if (!geoView)
    return;

  if (!isInitialization)
  {
    // This is a user-defined scene change, so we take the scene's
    // position as our new source of truth.
    auto v = m_scene->initialViewpoint();
    if (!v.isEmpty())
    {
      writeInitialLocation(m_scene->initialViewpoint());
    }
  }
  else
  {
    // This is an initilization scene change, replace the scene's
    // initial location with our current initial location.
    auto v = readInitialLocation();

    if (!v.isEmpty())
    {
      // Note use of setViewPoint instead of setInitialLocation. The latter
      // only works if the scene is not loaded, but all MSPK scenes are loaded with
      // the MSPK so it can't be used.
      geoView->setViewpointAsync(readInitialLocation(), 0);
    }
    else
    {
      // If there is no defined config location we will take the opportunity to
      // write it out using the current scene if applicable.
      auto v = m_scene->initialViewpoint();
      if (!v.isEmpty())
        writeInitialLocation(m_scene->initialViewpoint());
    }
  }
}

namespace
{

/*! \brief Read method for custom QSettings JSON format
 *
 * Attempts to read the information in \a device in a JSON format
 * and stores the results in \a map.
 *
 * Returns \c true if the read was successful and \c false if not.
 */
bool readJsonFile(QIODevice& device, QSettings::SettingsMap& map)
{
  const QByteArray data = device.readAll();
  const QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

  const QJsonObject jsonObject = jsonDoc.object();

  map = jsonObject.toVariantMap();

  return !map.isEmpty();
}

/*! \brief Write method for custom QSettings JSON format
 *
 * Attempts to encode the information in \a map as valid JSON
 * and writes the results to \a device.
 *
 * Returns \c true if the write was successful and \c false if not.
 */
bool writeJsonFile(QIODevice& device, const QSettings::SettingsMap& map)
{
  const QJsonObject jsonObject = QJsonObject::fromVariantMap(map);

  if (jsonObject.isEmpty())
    return false;

  const QJsonDocument conditionsJsonDoc(jsonObject);
  const qint64 writtenBytes = device.write(conditionsJsonDoc.toJson(QJsonDocument::Indented));

  return writtenBytes != -1;
}

Viewpoint viewpointFromJson(const QJsonObject& initialLocation)
{
  if (initialLocation.isEmpty())
    return Viewpoint{};

  // set the initial center Point from JSON if it is found
  auto centerIt = initialLocation.find("center");
  if (centerIt == initialLocation.constEnd())
    return Viewpoint{};

  const QJsonValue centerVal = centerIt.value();
  const QJsonDocument centerDoc = QJsonDocument(centerVal.toObject());
  const auto newCenter = Point::fromJson(centerDoc.toJson(QJsonDocument::JsonFormat::Compact));

  auto distanceIt = initialLocation.find("distance");
  if (distanceIt == initialLocation.constEnd())
    return Viewpoint{};
  double initialDistance = distanceIt.value().toDouble(0);

  auto headingIt = initialLocation.find("heading");
  if (headingIt == initialLocation.constEnd())
    return Viewpoint{};
  double initialHeading = headingIt.value().toDouble(0.0);

  auto pitchIt = initialLocation.find("pitch");
  if (pitchIt == initialLocation.constEnd())
    return Viewpoint{};
  double initialPitch = pitchIt.value().toDouble(0.0);

  auto rollIt = initialLocation.find("roll");
  if (rollIt == initialLocation.constEnd())
    return Viewpoint{};
  double initialRoll = rollIt.value().toDouble(0.0);

  // Return the initial viewpoint
  const Camera initCamera(geometry_cast<Point>(newCenter), initialDistance, initialHeading, initialPitch, initialRoll);
  Viewpoint initViewpoint(newCenter, initCamera);
  return initViewpoint;
}

QJsonObject defaultViewpoint()
{
  QJsonObject defaultViewpoint;
  const QString point = DsaUtility::montereyCA().toJson();
  const QJsonDocument centerDoc = QJsonDocument::fromJson(point.toLatin1());
  defaultViewpoint.insert( QStringLiteral("center"), centerDoc.object());
  defaultViewpoint.insert( QStringLiteral("distance"), 5000);
  defaultViewpoint.insert( QStringLiteral("heading"), 0.0);
  defaultViewpoint.insert( QStringLiteral("pitch"), 75.0);
  defaultViewpoint.insert( QStringLiteral("roll"), 0.0);
  return defaultViewpoint;
}

} // namespace
} // namespace Dsa

// Signal Documentation

/*!
  \fn void DsaController::errorOccurred(const QString& message, const QString& additionalMessage);

  \brief Signal emitted when an error occurs.

  An error \a message and \a additionalMessage are passed through as parameters, describing
  the error that occurred.
 */


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

#include "DsaUtility.h"

// C++ API headers
#include "SpatialReference.h"

// Qt headers
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QJsonValueConstRef>
#include <QtMath>

// DSA headers
#include "ConfigurationController.h"

using namespace Esri::ArcGISRuntime;

namespace Dsa {

const QString DsaUtility::FILE_NAME_DSA_CONFIGURATIONS = "DsaConfigurations.json";

/*!
  \class Dsa::DsaUtility
  \inmodule Dsa
  \brief Static helper class with a variety of utility methods for the Dsa app.
 */

/*!
  \brief Returns the platform independent data path to \c [HOME]/ArcGIS/Runtime/Data/DSA.
 */
QString DsaUtility::configurationsDirectoryPath()
{
  // return the cached path to the configurations folder
  static QString fullPathToConfigurationsDirectory;
  if (!fullPathToConfigurationsDirectory.isNull())
    return fullPathToConfigurationsDirectory;

  // setup the root directory based on the os/platform
#if defined Q_OS_ANDROID || defined Q_OS_IOS
  QDir dir{QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)};
#else
  QDir dir{QDir::home()};
#endif

  // append the standard path to the dsa application data folder structure
  dir.cd("ArcGIS");
  dir.cd("Runtime");
  dir.cd("Data");
  dir.cd("DSA");

  // return the full path to the configurations directory
  fullPathToConfigurationsDirectory = QString(dir.absolutePath());
  return fullPathToConfigurationsDirectory;
}

QString DsaUtility::configurationsFilePath()
{
  // return the cached path to the configuration file
  static QString fullPathToConfigurationsFile;
  if (!fullPathToConfigurationsFile.isNull())
    return fullPathToConfigurationsFile;

  // return the full path to the configurations file
  QDir dir{DsaUtility::configurationsDirectoryPath()};
  fullPathToConfigurationsFile = dir.absoluteFilePath(FILE_NAME_DSA_CONFIGURATIONS);
  return fullPathToConfigurationsFile;
}

QString DsaUtility::activeConfigurationPath()
{
  // return the cached path to the configuration folder
  static QString fullPathToActiveConfigurationFolder;
  if (!fullPathToActiveConfigurationFolder.isNull())
    return fullPathToActiveConfigurationFolder;

  // check for the default configurations file and create if it doesn't exist
  QFile fileConfigurations{DsaUtility::configurationsFilePath()};
  if (!fileConfigurations.exists())
    ConfigurationController::createDefaultConfigurationsFile();

  // make sure the file can be opened for read
  QString selectedConfigurationName{};
  if (!fileConfigurations.open(QIODevice::ReadOnly))
    return QString{};

  // check that the document parses and contains an array
  QJsonParseError parseError;
  const auto docConfigurations = QJsonDocument::fromJson(fileConfigurations.readAll(), &parseError);
  if (docConfigurations.isNull() || docConfigurations.isEmpty() || !docConfigurations.isArray())
    return QString{};

  // loop through all the elements in the array
  const auto configurationsArray = docConfigurations.array();
  for (const auto& configurationNode : configurationsArray)
  {
    // skip any non object types
    if (!configurationNode.isObject())
      continue;

    // get the selected value
    const auto configurationObject = configurationNode.toObject();
    const auto configurationSelected = configurationObject["selected"].toBool();
    if (configurationSelected)
    {
      selectedConfigurationName = configurationObject["name"].toString();
      break;
    }
  }

  // abort if the file contained nothing that could resolve to a name that represents a folder
  if (selectedConfigurationName.isEmpty())
    return QString{};

  // find the selected configuration folder on disk and create if not exists
  QDir dirConfigurations{DsaUtility::configurationsDirectoryPath()};
  QDir dirConfiguration{dirConfigurations.filePath(selectedConfigurationName)};
  if (!dirConfiguration.exists())
    dirConfigurations.mkdir(selectedConfigurationName);

  // cache the value for the data path, toggle the cached flag and return the value
  fullPathToActiveConfigurationFolder = dirConfiguration.absolutePath();
  return fullPathToActiveConfigurationFolder;
}

/*!
  \brief Returns an \l Esri::ArcGISRuntime::Point in Monterey, California.
 */
Point DsaUtility::montereyCA()
{
  return Point(-121.9, 36.6, SpatialReference::wgs84());
}

/*!
  \brief Returns the distance in meters between the \a from and \a to points.

  \note Assumes both points are in the same spatial reference.
 */
double DsaUtility::distance3D(const Point& from, const Point& to)
{
  const QVector3D fromCartesian = toCartesianPoint(from);
  const QVector3D toCartesian = toCartesianPoint(to);

  const QVector3D result(fromCartesian - toCartesian);

  return result.length();
}

/*!
  \brief Returns \a point as a Cartesian point.
 */
QVector3D DsaUtility::toCartesianPoint(const Point& point)
{
  // convert degrees (lat/long) to radians
  constexpr double degreesToRadians = M_PI/180.0;
  const double xRadians = point.x() * degreesToRadians;
  const double yRadians = point.y() * degreesToRadians;

  // convert to cartesian coords
  constexpr double earthRadius = 6378137.0;
  const double radius = point.z() + earthRadius;
  const double radCosLat = radius * std::cos(yRadians);

  return QVector3D(radCosLat * std::sin(xRadians), radius * std::sin(yRadians), radCosLat * std::cos(xRadians));
}

} // Dsa

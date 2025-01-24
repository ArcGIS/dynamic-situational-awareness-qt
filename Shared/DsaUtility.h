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

#ifndef UTILITY_H
#define UTILITY_H

namespace Esri::ArcGISRuntime {
  class Point;
}

// Qt headers
#include <QList>
#include <QObject>
#include <QString>
#include <QVector3D>

namespace Dsa {

class DsaUtility
{
public:

  static const QString FILE_NAME_DSA_CONFIGURATIONS;
  static QString configurationsDirectoryPath();
  static QString configurationsFilePath();
  static QString activeConfigurationPath();
  static Esri::ArcGISRuntime::Point montereyCA();
  static double distance3D(const Esri::ArcGISRuntime::Point& from, const Esri::ArcGISRuntime::Point& to);
  static QVector3D toCartesianPoint(const Esri::ArcGISRuntime::Point& point);
};

} // Dsa

#endif // UTILITY_H

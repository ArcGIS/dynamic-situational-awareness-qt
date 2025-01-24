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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// Qt headers
#include <QString>

namespace Dsa {

class Configuration
{
public:
  Configuration(const QString& name,
                const QString& url,
                bool selected,
                bool loaded,
                int percentDownloaded);
  Configuration(Configuration&) = default;
  Configuration(const Configuration&) = default;
  Configuration(Configuration&&) = default;
  Configuration& operator=(const Configuration&) = default;
  Configuration& operator=(Configuration&&) = default;

  QString name() const;
  void setName(const QString& name);

  QString url() const;
  void setUrl(const QString& url);

  bool downloaded() const;
  bool downloading() const;
  bool requiresRestart() const;
  bool canDownload() const;

  bool selected() const;
  void setSelected(bool selected);

  bool loaded() const;

  int percentDownloaded() const;
  void setPercentDownloaded(int percentDownloaded);

private:
  QString m_name;
  QString m_url;
  bool m_selected = false;
  bool m_loaded = false;
  int m_percentDownloaded = 0;
};

}

#endif // CONFIGURATION_H

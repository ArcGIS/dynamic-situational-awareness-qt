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

#ifndef CONFIGURATIONLISTMODEL_H
#define CONFIGURATIONLISTMODEL_H

// Qt headers
#include <QAbstractListModel>

// DSA headers
#include "Configuration.h"

namespace Dsa {

class ConfigurationListModel : public QAbstractListModel
{
  Q_OBJECT

public:

  enum Roles
  {
    Name = Qt::UserRole + 1,
    Url,
    Downloaded,
    Selected,
    Loaded,
    PercentDownloaded
  };

  explicit ConfigurationListModel(QObject* parent = nullptr);
  ~ConfigurationListModel();

  void select(int index);
  void cancel(int index);

  // to be implemented when additional packages can be setup for download by the end user
  // void remove(int index);
  // void download(int index);

  // QAbstractItemModel interface
  int rowCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  bool add(const QString& name, const QString& url, bool selected, bool loaded, int percentDownloaded);

  QList<Configuration>::iterator begin() { return m_configurations.begin(); }
  QList<Configuration>::iterator end() { return m_configurations.end(); }
  const QList<Configuration>::const_iterator const_begin() { return m_configurations.constBegin(); }
  const QList<Configuration>::const_iterator const_end() { return m_configurations.constEnd(); }

protected:
  QHash<int, QByteArray> roleNames() const override;

private:
  QHash<int, QByteArray> m_roles;
  QList<Configuration> m_configurations;
};
	
} // Dsa

#endif // CONFIGURATIONLISTMODEL_H

/*******************************************************************************
 *  Copyright 2012-2024 Esri
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

#include "pch.hpp"

#include "ConfigurationListModel.h"

namespace Dsa {

ConfigurationListModel::ConfigurationListModel(QObject* parent):
  QAbstractListModel(parent)
{
}

ConfigurationListModel::~ConfigurationListModel()
{

}

void ConfigurationListModel::select(int index)
{
  // TODO: ensure the index is valid
  int loc = 0;
  for (auto& configuration : m_configurations)
  {
    configuration.setSelected(index == loc);
    auto idx = createIndex(loc, 0);
    emit dataChanged(idx, idx);
    loc++;
  }
}

void ConfigurationListModel::download(int index)
{
  Q_UNUSED(index); // TODO: downloading multiple files at a time will require more management objects for the replies
}

void ConfigurationListModel::cancel(int index)
{
  // TODO: ensure the index is valid
  auto& configuration = m_configurations[index];
  configuration.setPercentDownloaded(0);
  auto idx = createIndex(index, 0);
  emit dataChanged(idx, idx);
}

void ConfigurationListModel::remove(int index)
{
  // TODO: remove the unpacked data on device
  Q_UNUSED(index);
}

int ConfigurationListModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return m_configurations.count();
}

QVariant ConfigurationListModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant{};

  const auto& configuration = m_configurations.at(index.row());
  switch (role)
  {
    case Roles::Name:
      return configuration.name();
    case Roles::Url:
      return configuration.url();
    case Roles::Downloaded:
      return configuration.downloaded();
    case Roles::Selected:
      return configuration.selected();
    case Roles::Loaded:
      return configuration.loaded();
    case Roles::PercentDownloaded:
      return configuration.percentDownloaded();
    default:
      return QVariant{};
  }

  return QVariant{};
}

bool ConfigurationListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  // TODO: better validation of the incoming index value
  if (!index.isValid())
    return false;

  auto& configuration = m_configurations[index.row()];

  switch (role)
  {
    case Roles::Name:
      configuration.setName(value.toString());
      break;
    case Roles::Url:
      configuration.setUrl(value.toString());
      break;
    case Roles::Downloaded:
      configuration.setDownloaded(value.toBool());
      break;
    case Roles::Selected:
      configuration.setSelected(value.toBool());
      break;
    case Roles::Loaded:
      configuration.setLoaded(value.toBool());
      break;
    case Roles::PercentDownloaded:
      configuration.setPercentDownloaded(value.toInt());
      break;
    default:
      break;
  }

  emit dataChanged(index, index);
  return true;
}

bool ConfigurationListModel::add(const QString& name, const QString& url, bool downloaded, bool selected, bool loaded, int percentDownloaded)
{
  beginInsertRows(QModelIndex{}, m_configurations.count(), m_configurations.count());
  m_configurations.emplaceBack(name, url, downloaded, selected, loaded, percentDownloaded);
  endInsertRows();
  return true;
}

QHash<int, QByteArray> ConfigurationListModel::roleNames() const
{
  static QHash<int, QByteArray> roles = {
    { Name, "Name" },
    { Url, "Url" },
    { Downloaded, "Downloaded" },
    { Selected, "Selected" },
    { Loaded, "Loaded" },
    { PercentDownloaded, "PercentDownloaded" }
  };

  return roles;
}

} // Dsa

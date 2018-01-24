// Copyright 2017 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

#include "AbstractAlert.h"
#include "AlertListModel.h"

#include <QUuid>

using namespace Esri::ArcGISRuntime;

AlertListModel* AlertListModel::instance()
{
  static AlertListModel s_instance;

  return &s_instance;
}

AlertListModel::AlertListModel(QObject* parent):
  QAbstractListModel(parent)
{
  m_roles[AlertListRoles::AlertId] = "alertId";
  m_roles[AlertListRoles::Message] = "message";
  m_roles[AlertListRoles::Status] = "status";
  m_roles[AlertListRoles::Position] = "position";
  m_roles[AlertListRoles::Viewed] = "viewed";
  m_roles[AlertListRoles::Description] = "description";
}

AlertListModel::~AlertListModel()
{

}

bool AlertListModel::addAlert(AbstractAlert* alert)
{
  if (!alert)
    return false;

  if (!alert->id().isNull())
    return false;

  const int size = m_alerts.size();
  const int insertIdx = size == 0 ? 0 : size -1;
  const QUuid id = QUuid::createUuid();
  alert->setId(id);

  auto handleDataChanged = [this, alert]()
  {
    if (alert->id().isNull())
      return;

    auto it = m_alerts.cbegin();
    auto itEnd = m_alerts.cend();
    int currRow = -1;
    for (; it != itEnd; ++it, ++currRow)
    {
      AbstractAlert* testAlert = *it;
      if (!testAlert)
        continue;

      if (testAlert->id() == alert->id())
      {
        const QModelIndex changedIndex = index(currRow, 0);
        emit dataChanged(changedIndex, changedIndex);
      }
    }
  };

  connect(alert, &AbstractAlert::viewedChanged, this, handleDataChanged);
  connect(alert, &AbstractAlert::positionChanged, this, handleDataChanged);
  connect(alert, &AbstractAlert::activeChanged, this, handleDataChanged);

  connect(alert, &AbstractAlert::noLongerValid, this, [this, alert]
  {
    removeAlert(alert);
  });

  beginInsertRows(QModelIndex(), insertIdx, insertIdx);
  m_alerts.append(alert);
  endInsertRows();

  return true;
}

void AlertListModel::removeAlert(AbstractAlert* alert)
{
  if (!alert)
    return;


  if (alert->id().isNull())
    return;

  const QUuid theId = alert->id();

  for (int i = 0; i < m_alerts.size(); ++i)
  {
    AbstractAlert* testAlert = m_alerts.at(i);
    if (!testAlert)
      continue;

    if (testAlert->id() != theId)
      continue;

    removeAt(i);
    break;
  }
}

AbstractAlert* AlertListModel::alertAt(int rowIndex) const
{
  return m_alerts.value(rowIndex, nullptr);
}

void AlertListModel::removeAt(int rowIndex)
{
  AbstractAlert* alert = alertAt(rowIndex);
  if (!alert)
    return;

  beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
  m_alerts.removeAt(rowIndex);
  endRemoveRows();
}

int AlertListModel::rowCount(const QModelIndex&) const
{
  return m_alerts.size();
}

QVariant AlertListModel::data(const QModelIndex& index, int role) const
{
  if (index.row() < 0 || index.row() > rowCount())
    return QVariant();

  AbstractAlert* alert = m_alerts.at(index.row());
  if (!alert)
    return QVariant();

  switch (role)
  {
  case AlertListRoles::AlertId:
    return alert->id();
  case AlertListRoles::Status:
    return static_cast<int>(alert->status());
  case AlertListRoles::Message:
  {
    return alert->message();
  }
  case AlertListRoles::Position:
  {
    return QVariant::fromValue(alert->position());
  }
  case AlertListRoles::Viewed:
  {
    return alert->viewed();
  }
  case AlertListRoles::Description:
  {
    return alert->description();
  }
  default:
    break;
  }

  return QVariant();
}

bool AlertListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (!index.isValid() || value.isNull())
    return false;

  AbstractAlert* alert = alertAt(index.row());
  if (!alert)
    return false;

  bool valueSet = false;
  switch (role)
  {
  case AlertListRoles::AlertId:
    break;
  case AlertListRoles::Status:
    break;
  case AlertListRoles::Message:
    break;
  case AlertListRoles::Position:
    break;
  case AlertListRoles::Viewed:
  {
    const bool newViewed = value.toBool();
    if (alert->viewed() != newViewed)
    {
      alert->setViewed(newViewed);
      valueSet = true;
    }
    break;
  }
  default:
    break;
  }

  if (valueSet)
    dataChanged(index, index);

  return valueSet;
}

QHash<int, QByteArray> AlertListModel::roleNames() const
{
  return m_roles;
}
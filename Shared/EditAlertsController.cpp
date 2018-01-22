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
#include "EditAlertsController.h"
#include "ProximityGraphicPairAlert.h"

#include "ToolManager.h"
#include "ToolResourceProvider.h"

#include "GeoView.h"
#include "GraphicsOverlay.h"
#include "GraphicsOverlayListModel.h"
#include "Layer.h"
#include "LayerListModel.h"

using namespace Esri::ArcGISRuntime;

EditAlertsController::EditAlertsController(QObject* parent /* = nullptr */):
  Toolkit::AbstractTool(parent),
  m_layerNames(new QStringListModel(this)),
  m_statusNames(new QStringListModel(QStringList{"Low", "Moderate", "High", "Critical"},this))
{
  Toolkit::ToolManager::instance().addTool(this);

  connect(Toolkit::ToolResourceProvider::instance(), &Toolkit::ToolResourceProvider::geoViewChanged,
          this, &EditAlertsController::onGeoviewChanged);

  connect(AlertListModel::instance(), &AlertListModel::dataChanged, this, &EditAlertsController::conditionsListChanged);

  onGeoviewChanged();
}

EditAlertsController::~EditAlertsController()
{
}

QString EditAlertsController::toolName() const
{
  return "Edit Alerts";
}

void EditAlertsController::addWithinDistanceAlert(int statusIndex, int sourceLayerIndex, double distance, int itemId, int targetLayerIndex)
{
  if (statusIndex < 0 ||
      sourceLayerIndex < 0 ||
      distance < 0.0 ||
      itemId < 0 ||
      targetLayerIndex < 0)
    return;

  if (sourceLayerIndex == targetLayerIndex)
    return;

  GeoView* geoView = Toolkit::ToolResourceProvider::instance()->geoView();
  if (!geoView)
    return;

  Layer* sourceLayer = nullptr;
  Layer* targetLayer = nullptr;
  GraphicsOverlay* sourceOverlay = nullptr;
  GraphicsOverlay* targetOverlay = nullptr;
  int currIndex = -1;
  LayerListModel* operationalLayers = Toolkit::ToolResourceProvider::instance()->operationalLayers();
  if (operationalLayers)
  {
    const int opLayersCount = operationalLayers->rowCount();
    for (int i = 0; i < opLayersCount; ++i)
    {
      currIndex++;
      Layer* layer = operationalLayers->at(i);
      if (!layer)
        continue;

      if (currIndex == sourceLayerIndex)
        sourceLayer = layer;

      if (currIndex == targetLayerIndex)
        targetLayer = layer;
    }
  }

  GraphicsOverlayListModel* graphicsOverlays = geoView->graphicsOverlays();
  if (graphicsOverlays)
  {
    const int overlaysCount = graphicsOverlays->rowCount();
    for (int i = 0; i < overlaysCount; ++i)
    {
      ++currIndex;
      GraphicsOverlay* overlay = graphicsOverlays->at(i);
      if (!overlay)
        continue;

      if (currIndex == sourceLayerIndex)
        sourceOverlay = overlay;

      if (currIndex == targetLayerIndex)
        targetOverlay = overlay;
    }
  }

  if (!sourceLayer && !sourceOverlay)
    return;

  if (!targetLayer && !targetOverlay)
    return;

  Graphic* targetGraphic = targetOverlay->graphics()->at(itemId);
  if (!targetGraphic)
    return;

  GraphicListModel* sourceGraphics = sourceOverlay->graphics();
  if (!sourceGraphics)
    return;

  AlertStatus status = static_cast<AlertStatus>(statusIndex);
  if (status > AlertStatus::Critical)
    return;

  auto createGraphicAlert = [this, sourceGraphics, targetGraphic, distance, status](int newGraphic)
  {
    Graphic* sourceGraphic = sourceGraphics->at(newGraphic);
    if (!sourceGraphic)
      return;

    ProximityGraphicPairAlert* geofenceAlert = new ProximityGraphicPairAlert(sourceGraphic, targetGraphic, distance, this);
    geofenceAlert->setStatus(status);
    geofenceAlert->setMessage("Location in geofence");
    geofenceAlert->setViewed(false);
    AlertListModel::instance()->addAlert(geofenceAlert);
  };

  for (int g = 0; g < sourceGraphics->rowCount(); ++g)
    createGraphicAlert(g);

  connect(sourceGraphics, &GraphicListModel::graphicAdded, this, createGraphicAlert);
}

void EditAlertsController::removeConditionAt(int rowIndex)
{
  AlertListModel::instance()->removeAt(rowIndex);
}

QAbstractItemModel* EditAlertsController::layerNames() const
{
  return m_layerNames;
}

QAbstractItemModel* EditAlertsController::statusNames() const
{
  return m_statusNames;
}

QAbstractItemModel* EditAlertsController::conditionsList() const
{
  return AlertListModel::instance();
}

void EditAlertsController::onGeoviewChanged()
{
  setLayerNames(QStringList());

  GeoView* geoView = Toolkit::ToolResourceProvider::instance()->geoView();
  if (!geoView)
    return;

  LayerListModel* operationalLayers = Toolkit::ToolResourceProvider::instance()->operationalLayers();
  if (operationalLayers)
  {
    connect(operationalLayers, &LayerListModel::layerAdded, this, &EditAlertsController::onLayersChanged);
    connect(operationalLayers, &LayerListModel::layerRemoved, this, &EditAlertsController::onLayersChanged);
  }

  GraphicsOverlayListModel* graphicsOverlays = geoView->graphicsOverlays();
  if (graphicsOverlays)
  {
    connect(graphicsOverlays, &GraphicsOverlayListModel::graphicsOverlayAdded, this, &EditAlertsController::onLayersChanged);
    connect(graphicsOverlays, &GraphicsOverlayListModel::graphicsOverlayRemoved, this, &EditAlertsController::onLayersChanged);
  }

  onLayersChanged();
}

void EditAlertsController::onLayersChanged()
{
  GeoView* geoView = Toolkit::ToolResourceProvider::instance()->geoView();
  if (!geoView)
  {
    setLayerNames(QStringList());
    return;
  }

  QStringList newList;
  LayerListModel* operationalLayers = Toolkit::ToolResourceProvider::instance()->operationalLayers();
  if (operationalLayers)
  {
    const int opLayersCount = operationalLayers->rowCount();
    for (int i = 0; i < opLayersCount; ++i)
    {
      Layer* lyr = operationalLayers->at(i);
      if (!lyr)
        continue;

      newList.append(lyr->name());
    }
  }

  GraphicsOverlayListModel* graphicsOverlays = geoView->graphicsOverlays();
  if (graphicsOverlays)
  {
    const int overlaysCount = graphicsOverlays->rowCount();
    for (int i = 0; i < overlaysCount; ++i)
    {
      GraphicsOverlay* overlay = graphicsOverlays->at(i);
      if (!overlay)
        continue;

      newList.append(overlay->overlayId());
    }
  }

  setLayerNames(newList);
}

void EditAlertsController::setLayerNames(const QStringList& layerNames)
{
  const QStringList existingNames = m_layerNames->stringList();
  if (existingNames == layerNames)
    return;

  m_layerNames->setStringList(layerNames);
  emit layerNamesChanged();
}


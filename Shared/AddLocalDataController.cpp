
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

#include "AddLocalDataController.h"

// C++ API headers
#include "ArcGISSceneLayer.h"
#include "ArcGISTiledElevationSource.h"
#include "ArcGISTiledLayer.h"
#include "ArcGISVectorTiledLayer.h"
#include "ElevationSource.h"
#include "ElevationSourceListModel.h"
#include "Error.h"
#include "FeatureLayer.h"
#include "GeoPackage.h"
#include "GeoPackageFeatureTable.h"
#include "GeoPackageRaster.h"
#include "Geodatabase.h"
#include "GeodatabaseFeatureTable.h"
#include "KmlDataset.h"
#include "KmlLayer.h"
#include "LayerListModel.h"
#include "LayerSceneProperties.h"
#include "Raster.h"
#include "RasterElevationSource.h"
#include "RasterLayer.h"
#include "Scene.h"
#include "SceneViewTypes.h"
#include "ServiceTypes.h"
#include "ShapefileFeatureTable.h"
#include "Surface.h"
#include "TileCache.h"
#include "TileInfo.h"
#include "VectorTileCache.h"

// Qt headers
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

// DSA headers
#include "DataItemListModel.h"
#include "DsaUtility.h"
#include "MarkupLayer.h"
#include "ToolManager.h"
#include "ToolResourceProvider.h"

using namespace Esri::ArcGISRuntime;

namespace Dsa
{

const QString AddLocalDataController::LOCAL_DATAPATHS_PROPERTYNAME = "LocalDataPaths";
const QString AddLocalDataController::DEFAULT_ELEVATION_PROPERTYNAME = "DefaultElevationSource";

const QString AddLocalDataController::s_allData = QStringLiteral("All Data (*.geodatabase *.tpk *.shp *.gpkg *.slpk *.img *.tif *.tiff *.i1, *.dt0 *.dt1 *.dt2 *.tc2 *.geotiff *.hr1 *.jpg *.jpeg *.jp2 *.ntf *.png *.i21 *.ovr *.markup *.sid *.kml *.kmz)");
const QString AddLocalDataController::s_rasterData = QStringLiteral("Raster Files (*.img *.tif *.tiff *.I1, *.dt0 *.dt1 *.dt2 *.tc2 *.geotiff *.hr1 *.jpg *.jpeg *.jp2 *.ntf *.png *.i21 *.ovr *.sid)");
const QString AddLocalDataController::s_geodatabaseData = QStringLiteral("Geodatabase (*.geodatabase)");
const QString AddLocalDataController::s_shapefileData = QStringLiteral("Shapefile (*.shp)");
const QString AddLocalDataController::s_geopackageData = QStringLiteral("GeoPackage (*.gpkg)");
const QString AddLocalDataController::s_sceneLayerData = QStringLiteral("Scene Layer Package (*.slpk)");
const QString AddLocalDataController::s_vectorTilePackageData = QStringLiteral("Vector Tile Package (*.vtpk)");
const QString AddLocalDataController::s_tilePackageData = QStringLiteral("Tile Package (*.tpk)");
const QString AddLocalDataController::s_markupData = QStringLiteral("Markup (*.markup)");
const QString AddLocalDataController::s_kmlData = QStringLiteral("KML (*.kml *.kmz)");

/*!
  \class Dsa::AddLocalDataController
  \inmodule Dsa
  \inherits Dsa::AbstractTool
  \brief Tool controller for adding local data to the app.
 */

/*!
 \brief Constructor that takes an optional \a parent.
 */
AddLocalDataController::AddLocalDataController(QObject* parent /* = nullptr */):
  AbstractTool(parent),
  m_localDataModel(new DataItemListModel(this))
{
  // add the base path to the string list
  addPathToDirectoryList(DsaUtility::activeConfigurationPath());

  // create file filter list
  m_fileFilterList = QStringList{allData(), rasterData(), geodatabaseData(),
      sceneLayerData(), tilePackageData(), shapefileData(), geopackageData(),
      markupData(), kmlData()
      /*, vectorTilePackageData()*/}; // VTPK is not supported in 3D
  emit fileFilterListChanged();
  emit localDataModelChanged();

  ToolManager::instance().addTool(this);
}

/*!
 \property AddLocalDataController::localDataModel
 \brief Returns the local data model associated with the controller.
 */
QAbstractListModel* AddLocalDataController::localDataModel() const
{
  return m_localDataModel;
}

/*!
 \brief Adds the provided \a path to the directory list.

 The directory list contains all of the directories that will be searched for local data.
 */
void AddLocalDataController::addPathToDirectoryList(const QString& path)
{
  if (m_dataPaths.contains(path))
    return;

  if (!QFileInfo::exists(path))
  {
    emit toolErrorOccurred(QStringLiteral("Failed to add local data path"), QString("Could not find %1").arg(path));
    return;
  }

  m_dataPaths << path;
  emit propertyChanged(LOCAL_DATAPATHS_PROPERTYNAME, m_dataPaths);
}

/*!
 \brief Refreshes the local data model with a given \a fileType.
 */
void AddLocalDataController::refreshLocalDataModel(const QString& fileType)
{
  QStringList fileFilters = determineFileFilters(fileType);
  m_localDataModel->clear();

  for (const QString& path : m_dataPaths)
  {
    QDir localDir(path);

    if (fileFilters.length() > 0)
      localDir.setNameFilters(fileFilters);

    for (const QString& file : localDir.entryList(QDir::Files, QDir::Name))
    {
      QFileInfo fileInfo(localDir, file);
      m_localDataModel->addDataItem(fileInfo.absoluteFilePath());
    }
  }
}

/*!
 \property AddLocalDataController::fileFilterList
 \brief Returns the file filter string list for filtering data from the QDir entrylist
 */
QStringList AddLocalDataController::determineFileFilters(const QString& fileType)
{
  QStringList fileFilter;
  QStringList rasterExtensions{"*.img", "*.tif", "*.tiff", "*.i1", "*.dt0", "*.dt1", "*.dt2", "*.tc2", "*.geotiff", "*.hr1", "*.jpg", "*.jpeg", "*.jp2", "*.ntf", "*.png", "*.i21", "*.sid"};

  if (fileType == geodatabaseData())
    fileFilter << "*.geodatabase";
  else if (fileType == tilePackageData())
    fileFilter << "*.tpk";
  else if (fileType == shapefileData())
    fileFilter << "*.shp";
  else if (fileType == geopackageData())
    fileFilter << "*.gpkg";
  else if (fileType == sceneLayerData())
    fileFilter << "*.slpk";
  else if (fileType == vectorTilePackageData())
    fileFilter << "*.vtpk";
  else if (fileType == markupData())
    fileFilter << "*.markup";
  else if (fileType == kmlData())
    fileFilter << "*.kml" << "*.kmz";
  else if (fileType == rasterData())
    fileFilter = rasterExtensions;
  else
  {
    fileFilter = rasterExtensions;
    fileFilter << "*.geodatabase" << "*.tpk" << "*.shp" << "*.gpkg" << "*.slpk" << "*.markup" << "*.kml" << "*.kmz"/* << "*.vtpk"*/; // VTPK is not supported in 3D
  }

  return fileFilter;
}

/*!
 \brief Adds the provided \a indices from the list model as elevation sources to the Scene.
 */
void AddLocalDataController::addItemAsElevationSource(const QList<int>& indices)
{
  QStringList dataPaths;

  for (const int index : indices)
  {
    const DataType dataItemType = m_localDataModel->getDataItemType(index);
    const QString dataItemPath = m_localDataModel->getDataItemPath(index);

    if (dataItemType == DataType::TilePackage)
    {
      createElevationSourceFromTpk(dataItemPath);
    }
    else if (dataItemType == DataType::Raster)
    {
      // add the path to a string list
      dataPaths << dataItemPath;
    }
    else
      continue;
  }

  if (dataPaths.isEmpty())
    return;

  createElevationSourceFromRasters(dataPaths);

  emit propertyChanged(DEFAULT_ELEVATION_PROPERTYNAME, dataPaths);
}

/*!
 \brief Adds the provided TPK \a path as an elevation source.
*/
void AddLocalDataController::createElevationSourceFromTpk(const QString& path)
{
  TileCache* tileCache = new TileCache(path, this);

  connect(tileCache, &TileCache::doneLoading, this, [this, tileCache](Error error)
  {
    if (!error.isEmpty())
      return;

    // Check if the tiles are LERC encoded
    if (tileCache->tileInfo().format() == TileImageFormat::LERC)
    {
      // create the source from the tiled source
      ArcGISTiledElevationSource* source = new ArcGISTiledElevationSource(tileCache, this);

      connect(source, &ArcGISTiledElevationSource::errorOccurred, this, &AddLocalDataController::errorOccurred);

      auto scene = ToolResourceProvider::instance()->scene();
      if (scene)
        scene->baseSurface()->elevationSources()->append(source);

      emit elevationSourceSelected(source);
      emit propertyChanged(DEFAULT_ELEVATION_PROPERTYNAME, tileCache->path());
    }
  });

  tileCache->load();
}

/*!
 \brief Adds the provided Raster \a paths as an elevation source.
*/
void AddLocalDataController::createElevationSourceFromRasters(const QStringList& paths)
{
  RasterElevationSource* source = new RasterElevationSource(paths, this);

  connect(source, &RasterElevationSource::errorOccurred, this, &AddLocalDataController::errorOccurred);

  auto scene = ToolResourceProvider::instance()->scene();
  if (scene)
    scene->baseSurface()->elevationSources()->append(source);

  emit elevationSourceSelected(source);
}

/*!
 \brief Adds the the markup from the provided path as a MarkupLayer.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createMarkupLayer(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  MarkupLayer* markupLayer = MarkupLayer::createFromPath(path, this);
  if (!markupLayer)
    return;

  markupLayer->setVisible(visible);
  connect(markupLayer, &MarkupLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

  if (autoAdd)
  {
    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
    operationalLayers->append(markupLayer);
  }
  else
    emit layerCreated(layerIndex, markupLayer);
}

/*!
 \brief Adds the provided \a indices from the list model as layers.
 */
void AddLocalDataController::addItemAsLayer(const QList<int>& indices)
{
  for (const int index : indices)
  {
    DataType dataItemType = m_localDataModel->getDataItemType(index);
    QString dataItemPath = m_localDataModel->getDataItemPath(index);

    switch (dataItemType) {
    case DataType::Geodatabase:
      createFeatureLayerGeodatabase(dataItemPath);
      break;
    case DataType::GeoPackage:
      createLayerGeoPackage(dataItemPath);
      break;
    case DataType::Raster:
      createRasterLayer(dataItemPath);
      break;
    case DataType::SceneLayerPackage:
      createSceneLayer(dataItemPath);
      break;
    case DataType::Shapefile:
      createFeatureLayerShapefile(dataItemPath);
      break;
    case DataType::TilePackage:
      createTiledLayer(dataItemPath);
      break;
    case DataType::VectorTilePackage:
      createVectorTiledLayer(dataItemPath);
      break;
    case DataType::Markup:
      createMarkupLayer(dataItemPath);
      break;
    case DataType::Kml:
      createKmlLayer(dataItemPath);
      break;
    default:
      break;
    }
  }
}

/*!
 \brief Determines data type from the given \a path, and calls the appropriate helper.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::addLayerFromPath(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  QFileInfo fileInfo(path);
  if (!fileInfo.exists())
  {
    emit toolErrorOccurred(QString("Failed to add %1").arg(fileInfo.fileName()), QString("File not found %1").arg(path));
    return;
  }

  QStringList rasterExtensions{"img", "tif", "tiff", "i1", "dt0", "dt1", "dt2", "tc2", "geotiff", "hr1", "jpg", "jpeg", "jp2", "ntf", "png", "i21", "sid"};

  // determine the layer type
  QString fileExtension = fileInfo.completeSuffix();

  if (fileExtension == "geodatabase")
    createFeatureLayerGeodatabaseWithId(path, layerIndex, visible, autoAdd);
  else if (fileExtension.compare("tpk", Qt::CaseInsensitive) == 0)
    createTiledLayer(path, layerIndex, visible, autoAdd);
  else if (fileExtension.compare("shp", Qt::CaseInsensitive) == 0)
    createFeatureLayerShapefile(path, layerIndex, visible, autoAdd);
  else if (fileExtension.compare("gpkg", Qt::CaseInsensitive) == 0)
    createLayerGeoPackage(path);
  else if (fileExtension.compare("slpk", Qt::CaseInsensitive) == 0)
    createSceneLayer(path, layerIndex, visible, autoAdd);
  else if (fileExtension.compare("vtpk", Qt::CaseInsensitive) == 0)
    createVectorTiledLayer(path, layerIndex, visible, autoAdd);
  else if (fileExtension.compare("markup", Qt::CaseInsensitive) == 0)
    createMarkupLayer(path, layerIndex, visible, autoAdd);
  else if ((fileExtension.compare("kml", Qt::CaseInsensitive) == 0) || (fileExtension.compare("kmz", Qt::CaseInsensitive) == 0))
    createKmlLayer(path, layerIndex, visible, autoAdd);
  else if (rasterExtensions.contains(fileExtension.toLower()))
    createRasterLayer(path, layerIndex, visible, autoAdd);
}

/*!
 \brief Creates a Geodatabase from the given \a path, and creates a FeatureLayer for each table in the Geodatabase.
 */
void AddLocalDataController::createFeatureLayerGeodatabase(const QString& path)
{
  Geodatabase* gdb = new Geodatabase(path, this);
  connect(gdb, &Geodatabase::doneLoading, this, [this, gdb](Error e)
  {
    if (!e.isEmpty())
    {
      emit errorOccurred(e);
      return;
    }

    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();

    for (FeatureTable* featureTable : gdb->geodatabaseFeatureTables())
    {
      FeatureLayer* featureLayer = new FeatureLayer(featureTable, this);

      connect(featureLayer, &FeatureLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

      connect(featureLayer, &FeatureLayer::doneLoading, this, [featureLayer](Error loadError)
      {
        if (!loadError.isEmpty())
          return;

        if (featureLayer->featureTable()->hasZ() && featureLayer->featureTable()->geometryType() == GeometryType::Point)
          featureLayer->setSceneProperties(LayerSceneProperties(SurfacePlacement::Absolute));
      });

      if (operationalLayers)
        operationalLayers->append(featureLayer);

      emit layerSelected(featureLayer);
    }
  });
  gdb->load();
}

/*!
 \brief Creates a FeatureLayer with the feature table at the given id of a Geodatabase

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a serviceLayerId - The service layer ID of the GeodatabaseFeatureTable.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createFeatureLayerGeodatabaseWithId(const QString& path, int layerIndex, int serviceLayerId, bool visible, bool autoAdd)
{
  Geodatabase* gdb = new Geodatabase(path, this);
  connect(gdb, &Geodatabase::doneLoading, this, [this, gdb, serviceLayerId, visible, autoAdd, layerIndex](Error e)
  {
    if (!e.isEmpty())
    {
      emit errorOccurred(e);
      return;
    }

    FeatureLayer* featureLayer = new FeatureLayer(gdb->geodatabaseFeatureTable(serviceLayerId), this);
    featureLayer->setVisible(visible);
    connect(featureLayer, &FeatureLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

    connect(featureLayer, &FeatureLayer::doneLoading, this, [featureLayer](Error loadError)
    {
      if (!loadError.isEmpty())
        return;

      if (featureLayer->featureTable()->hasZ() && featureLayer->featureTable()->geometryType() == GeometryType::Point)
        featureLayer->setSceneProperties(LayerSceneProperties(SurfacePlacement::Absolute));
    });

    if (autoAdd)
    {
      auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
      if (operationalLayers)
        operationalLayers->append(featureLayer);

      emit layerSelected(featureLayer);
    }
    else
    {
      emit layerCreated(layerIndex, featureLayer);
    }
  });
  gdb->load();
}

/*!
 \brief Creates a FeatureLayer with the feature table at the given index of a GeoPackage

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a id - The index of the feature table in the GeoPackage feature table list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createFeatureLayerGeoPackage(const QString& path, int layerIndex, int id, bool visible, bool autoAdd)
{
  GeoPackage* geoPackage = new GeoPackage(path, this);

  connect(geoPackage, &GeoPackage::doneLoading, this, [this, geoPackage, id, visible, autoAdd, layerIndex](Error e)
  {
    if (!e.isEmpty())
    {
      emit errorOccurred(e);
      return;
    }

    FeatureLayer* featureLayer = new FeatureLayer(geoPackage->geoPackageFeatureTables().at(id), this);
    featureLayer->setVisible(visible);
    connect(featureLayer, &FeatureLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

    connect(featureLayer, &FeatureLayer::doneLoading, this, [featureLayer](Error loadError)
    {
      if (!loadError.isEmpty())
        return;

      if (featureLayer->featureTable()->hasZ() && featureLayer->featureTable()->geometryType() == GeometryType::Point)
        featureLayer->setSceneProperties(LayerSceneProperties(SurfacePlacement::Absolute));
    });

    if (autoAdd)
    {
      auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
      operationalLayers->append(featureLayer);
      emit layerSelected(featureLayer);
    }
    else
    {
      emit layerCreated(layerIndex, featureLayer);
    }
  });

  geoPackage->load();
}

/*!
 \brief Creates a RasterLayer with the raster at the given index of a GeoPackage

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a id - The index of the raster in the GeoPackage raster list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createRasterLayerGeoPackage(const QString& path, int layerIndex, int id, bool visible, bool autoAdd)
{
  GeoPackage* geoPackage = new GeoPackage(path, this);

  connect(geoPackage, &GeoPackage::doneLoading, this, [this, geoPackage, id, visible, autoAdd, layerIndex](Error e)
  {
    if (!e.isEmpty())
    {
      emit errorOccurred(e);
      return;
    }

    RasterLayer* rasterLayer = new RasterLayer(geoPackage->geoPackageRasters().at(id), this);
    connect(rasterLayer, &RasterLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);
    rasterLayer->setVisible(visible);

    if (autoAdd)
    {
      auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
      if (!operationalLayers)
        return;

      operationalLayers->append(rasterLayer);
      emit layerSelected(rasterLayer);
    }
    else
    {
      emit layerCreated(layerIndex, rasterLayer);
    }
  });

  geoPackage->load();
}

/*!
 \brief Creates a GeoPackage from the given \a path, and adds a Layer for each table and raster in the GeoPackage
 */
void AddLocalDataController::createLayerGeoPackage(const QString& path)
{
  GeoPackage* geoPackage = new GeoPackage(path, this);

  connect(geoPackage, &GeoPackage::doneLoading, this, [this, geoPackage](Error e)
  {
    if (!e.isEmpty())
    {
      emit errorOccurred(e);
      return;
    }

    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();

    for (const auto& table : geoPackage->geoPackageFeatureTables())
    {
      FeatureLayer* featureLayer = new FeatureLayer(table, this);
      connect(featureLayer, &FeatureLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

      connect(featureLayer, &FeatureLayer::doneLoading, this, [featureLayer](Error loadError)
      {
        if (!loadError.isEmpty())
          return;

        if (featureLayer->featureTable()->hasZ() && featureLayer->featureTable()->geometryType() == GeometryType::Point)
          featureLayer->setSceneProperties(LayerSceneProperties(SurfacePlacement::Absolute));
      });

      if (operationalLayers)
        operationalLayers->append(featureLayer);

      emit layerSelected(featureLayer);
    }

    for (const auto& raster : geoPackage->geoPackageRasters())
    {
      RasterLayer* rasterLayer = new RasterLayer(raster, this);
      connect(rasterLayer, &RasterLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);
      if (operationalLayers)
        operationalLayers->append(rasterLayer);

      emit layerSelected(rasterLayer);
    }
  });

  geoPackage->load();
}

/*!
 \brief Creates a FeatureLayer with the provided shapefile path.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createFeatureLayerShapefile(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  ShapefileFeatureTable* shpFt = new ShapefileFeatureTable(path, this);
  FeatureLayer* featureLayer = new FeatureLayer(shpFt, this);
  featureLayer->setVisible(visible);
  connect(featureLayer, &FeatureLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

  connect(featureLayer, &FeatureLayer::doneLoading, this, [featureLayer](Error loadError)
  {
    if (!loadError.isEmpty())
      return;

    if (featureLayer->featureTable()->hasZ() && featureLayer->featureTable()->geometryType() == GeometryType::Point)
    {
      featureLayer->setSceneProperties(LayerSceneProperties(SurfacePlacement::Absolute));
    }
  });

  if (autoAdd)
  {
    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
    if (operationalLayers)
      operationalLayers->append(featureLayer);

    emit layerSelected(featureLayer);
  }
  else
  {
    emit layerCreated(layerIndex, featureLayer);
  }
}

/*!
 \brief Creates a Rasterlayer with the provided raster path.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createRasterLayer(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  Raster* raster = new Raster(path, this);
  RasterLayer* rasterLayer = new RasterLayer(raster, this);
  rasterLayer->setVisible(visible);
  connect(rasterLayer, &RasterLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

  if (autoAdd)
  {
    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
    if (operationalLayers)
      operationalLayers->append(rasterLayer);

    emit layerSelected(rasterLayer);
  }
  else
  {
    emit layerCreated(layerIndex, rasterLayer);
  }
}

/*!
 \brief Creates an ArcGISSceneLayer with the provided scene layer package path.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createSceneLayer(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  ArcGISSceneLayer* sceneLayer = new ArcGISSceneLayer(QUrl::fromLocalFile(path), this);
  sceneLayer->setVisible(visible);
  connect(sceneLayer, &ArcGISSceneLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

  if (autoAdd)
  {
    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
    if (operationalLayers)
      operationalLayers->append(sceneLayer);

    emit layerSelected(sceneLayer);
  }
  else
  {
    emit layerCreated(layerIndex, sceneLayer);
  }
}

/*!
 \brief Creates an ArcGISTiledLayer with the provided TPK path.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/
void AddLocalDataController::createTiledLayer(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  TileCache* tileCache = new TileCache(path, this);
  ArcGISTiledLayer* tiledLayer = new ArcGISTiledLayer(tileCache, this);
  tiledLayer->setVisible(visible);
  connect(tiledLayer, &ArcGISTiledLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

  if (autoAdd)
  {
    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
    if (operationalLayers)
      operationalLayers->append(tiledLayer);

    emit layerSelected(tiledLayer);
  }
  else
  {
    emit layerCreated(layerIndex, tiledLayer);
  }
}

/*!
 \brief Creates an ArcGISVectorTiledLayer with the provided VTPK path.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/void AddLocalDataController::createVectorTiledLayer(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  VectorTileCache* tileCache = new VectorTileCache(path, this);
  connect(tileCache, &VectorTileCache::errorOccurred, this, &AddLocalDataController::errorOccurred);
  ArcGISVectorTiledLayer* vectorTiledLayer = new ArcGISVectorTiledLayer(tileCache, this);
  vectorTiledLayer->setVisible(visible);
  connect(vectorTiledLayer, &ArcGISVectorTiledLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

  if (autoAdd)
  {
    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
    if (operationalLayers)
      operationalLayers->append(vectorTiledLayer);

    emit layerSelected(vectorTiledLayer);
  }
  else
  {
    emit layerCreated(layerIndex, vectorTiledLayer);
  }
}

/*!
 \brief Creates a KmlLayer with the provided KML path.

 \list
   \li \a path - The path to the local data source.
   \li \a layerIndex - The index for which the layer will be added to the operational layer list.
   \li \a visible - Whether the layer should be visible by default.
   \li \a autoAdd - Whether the layer will be automatically added to the operational layer list.
        If \c false, it will not add automatically. Instead, a signal will emit once the Layer has
        been constructed.
 \endlist
*/void AddLocalDataController::createKmlLayer(const QString& path, int layerIndex, bool visible, bool autoAdd)
{
  KmlDataset* kmlDataset = new KmlDataset(QUrl::fromLocalFile(path), this);
  connect(kmlDataset, &KmlDataset::errorOccurred, this, &AddLocalDataController::errorOccurred);
  KmlLayer* kmlLayer = new KmlLayer(kmlDataset, this);
  kmlLayer->setVisible(visible);
  connect(kmlLayer, &KmlLayer::errorOccurred, this, &AddLocalDataController::errorOccurred);

  if (autoAdd)
  {
    auto operationalLayers = ToolResourceProvider::instance()->operationalLayers();
    if (operationalLayers)
      operationalLayers->append(kmlLayer);

    emit layerSelected(kmlLayer);
  }
  else
  {
    emit layerCreated(layerIndex, kmlLayer);
  }
}

/*!
 \brief Returns the tool's name.
*/
QString AddLocalDataController::toolName() const
{
  return QStringLiteral("add local data");
}

/*
 \brief Sets \a properties, such as the directories to search for local data.
*/
void AddLocalDataController::setProperties(const QVariantMap& properties)
{
  const QStringList filePaths = properties[LOCAL_DATAPATHS_PROPERTYNAME].toStringList();
  if (filePaths.empty())
    return;

  bool anyNewPaths = false;
  for (const QString& path : filePaths)
  {
    if (m_dataPaths.contains(path))
      continue;

    anyNewPaths = true;
    break;
  }

  if (!anyNewPaths)
    return;

  for (const QString& filePath : filePaths)
    addPathToDirectoryList(filePath);

  refreshLocalDataModel();
}

} // Dsa

// Signal Documentation

/*!
  \fn void AddLocalDataController::localDataModelChanged();

  \brief Signal emitted when the LocalDataModel associated with this class changes.
 */

/*!
  \fn void AddLocalDataController::layerSelected(Esri::ArcGISRuntime::Layer* layer);

  \brief Signal emitted when a \a layer is selected.
 */

/*!
  \fn void AddLocalDataController::elevationSourceSelected(Esri::ArcGISRuntime::ElevationSource* source);

  \brief Signal emitted when an elevation \a source is selected.
 */

/*!
  \fn void AddLocalDataController::layerCreated(int i, Esri::ArcGISRuntime::Layer* layer);

  \brief Signal emitted when a \a layer is created.

  The index of the layer in the operational layer list is passed through as \a i.
 */

/*!
  \fn void AddLocalDataController::toolErrorOccurred(const QString& errorMessage, const QString& additionalMessage);

  \brief Signal emitted when an error occurs.

  An \a errorMessage and \a additionalMessage are passed through as parameters, describing
  the error that occurred.
 */


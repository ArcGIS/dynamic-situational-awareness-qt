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

#ifndef GEOMETRYQUADTREE_H
#define GEOMETRYQUADTREE_H

#include <memory>

#include <QList>
#include <QObject>

namespace Esri {
namespace ArcGISRuntime {
class Envelope;
class GeoElement;
class Geometry;
class Point;
}
}

class GeometryQuadtree : public QObject
{
  Q_OBJECT

public:
  explicit GeometryQuadtree(const Esri::ArcGISRuntime::Envelope& extent,
                            const QList<Esri::ArcGISRuntime::GeoElement*>& geoElements,
                            int maxLevels,
                            QObject* parent = nullptr);
  ~GeometryQuadtree();

  void appendGeoElment(Esri::ArcGISRuntime::GeoElement* newGeoElement);

  QList<Esri::ArcGISRuntime::Geometry> candidateIntersections(const Esri::ArcGISRuntime::Geometry& geometry) const;
  QList<Esri::ArcGISRuntime::Geometry> candidateIntersections(const Esri::ArcGISRuntime::Envelope& extent) const;
  QList<Esri::ArcGISRuntime::Geometry> candidateIntersections(const Esri::ArcGISRuntime::Point& location) const;

signals:
  void treeChanged();

private:
  void buildTree(const Esri::ArcGISRuntime::Envelope& extent);
  void cacheGeometry();
  void handleGeometryChange();

  struct QuadTree;

  int m_maxLevels;
  QList<Esri::ArcGISRuntime::GeoElement*> m_elements;
  std::unique_ptr<QuadTree> m_tree;
  QList<Esri::ArcGISRuntime::Geometry> m_geometry;
};

#endif // GEOMETRYQUADTREE_H
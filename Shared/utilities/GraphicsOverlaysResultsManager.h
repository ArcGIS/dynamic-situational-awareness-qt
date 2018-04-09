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

#ifndef GRAPHICSOVERLAYSRESULTSMANAGER_H
#define GRAPHICSOVERLAYSRESULTSMANAGER_H


// Qt headers
#include <QList>

namespace Esri {
namespace ArcGISRuntime {
  class IdentifyGraphicsOverlayResult;
}
}

namespace Dsa {
namespace Utilities {

struct GraphicsOverlaysResultsManager {

  QList<Esri::ArcGISRuntime::IdentifyGraphicsOverlayResult*> m_results;

  explicit GraphicsOverlaysResultsManager(const QList<Esri::ArcGISRuntime::IdentifyGraphicsOverlayResult*>& results);

  ~GraphicsOverlaysResultsManager();
};

} // Utilities
} // Dsa

#endif // GRAPHICSOVERLAYSRESULTSMANAGER_H

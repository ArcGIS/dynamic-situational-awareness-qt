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

#ifndef DSA_PCH_HPP
#define DSA_PCH_HPP

#ifdef __cplusplus

// dsa app headers
#include "AlertCondition.h"
#include "AlertFilter.h"
#include "AlertLevel.h"
#include "AlertTarget.h"

// toolkit headers
#include "AbstractTool.h"
#include "ToolManager.h"
#include "ToolResourceProvider.h"

// C++ API headers
#include "GeoElement.h"
#include "GeometryTypes.h"
#include "Point.h"
#include "Scene.h"
#include "SceneView.h"

// Qt headers
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

#endif // __cplusplus

#endif // DSA_PCH_HPP

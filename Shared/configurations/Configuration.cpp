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

#include "pch.hpp"

#include "Configuration.h"

namespace Dsa {

Configuration::Configuration(const QString& name, const QString& url, bool selected, bool loaded, int percentDownloaded):
  m_name(name),
  m_url(url),
  m_selected(selected),
  m_loaded(loaded),
  m_percentDownloaded(percentDownloaded)
{

}

QString Configuration::name() const
{
  return m_name;
}

void Configuration::setName(const QString& name)
{
  m_name = name;
}

QString Configuration::url() const
{
  return m_url;
}

void Configuration::setUrl(const QString& url)
{
  m_url = url;
}

bool Configuration::downloaded() const
{
  return m_percentDownloaded == 100;
}

bool Configuration::selected() const
{
  return m_selected;
}

void Configuration::setSelected(bool selected)
{
  m_selected = selected;
}

bool Configuration::loaded() const
{
  return m_loaded;
}

int Configuration::percentDownloaded() const
{
  return m_percentDownloaded;
}

void Configuration::setPercentDownloaded(int percentDownloaded)
{
  m_percentDownloaded = percentDownloaded;
}

} // Dsa

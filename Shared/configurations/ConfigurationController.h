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

#ifndef CONFIGURATIONCONTROLLER_H
#define CONFIGURATIONCONTROLLER_H

// Qt headers
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// DSA headers
#include "AbstractTool.h"
#include "ZipHelper.h"

Q_MOC_INCLUDE("QAbstractListModel")
class QAbstractListModel;

namespace Dsa {

class ConfigurationListModel;

class ConfigurationController : public AbstractTool
{
  inline static const QString DEFAULT_DOWNLOAD_URL{"https://usdanrcswmx.esri.com/arcgis/sharing/rest/content/items/46c2b274325c4418833624d48cb2a44a/data"};
  inline static const QString DEFAULT_DOWNLOAD_NAME{"Default"};

  Q_OBJECT

  Q_PROPERTY(QAbstractListModel* configurations READ configurations NOTIFY configurationsChanged)
  Q_PROPERTY(bool downloading READ downloading NOTIFY configurationsChanged)
  Q_PROPERTY(bool requiresRestart READ requiresRestart NOTIFY configurationsChanged)
  Q_PROPERTY(bool configurationIsAvailable READ configurationIsAvailable)

public:
  explicit ConfigurationController(QObject* parent = nullptr);

  Q_INVOKABLE void select(int index);
  Q_INVOKABLE void download(int index);
  Q_INVOKABLE void cancel(int index);
  Q_INVOKABLE void remove(int index);
  Q_INVOKABLE void downloadDefaultData();
  static bool createDefaultConfigurationsFile();

  QString toolName() const override;
  bool downloading();
  bool requiresRestart();
  bool configurationIsAvailable();

  QAbstractListModel* configurations() const;

signals:
  void toolErrorOccurred(const QString& errorMessage, const QString& additionalMessage);
  void configurationsChanged();

private slots:
  void downloadProgress(quint64 bytesReceived, quint64 bytesTotal);
  void readyRead();
  void finished();
  void downloadErrorOccurred(QNetworkReply::NetworkError);
  void extractCompleted();
  void extractError(const QString& fileName, const QString& outputFileName, ZipHelper::Result result);

private:
  void setPercentComplete(int percentComplete);
  void fetchConfigurations();
  void storeConfigurations();
  ConfigurationListModel* m_configurationListModel = nullptr;
  QNetworkAccessManager m_networkAccessManager;
  int m_activeDownloadIndex;
  QDir m_downloadFolder;
  QString m_downloadFileName;
  bool m_aborted = false;
  bool m_downloading = false;
  QNetworkReply* m_networkReply = nullptr;
  ZipHelper* m_zipHelper = nullptr;
};

}

#endif // CONFIGURATIONCONTROLLER_H

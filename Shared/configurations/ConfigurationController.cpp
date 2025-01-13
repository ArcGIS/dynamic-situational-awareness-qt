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

// PCH header
#include "pch.hpp"

#include "ConfigurationController.h"

// Qt headers
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUuid>

// DSA headers
#include "Configuration.h"
#include "ConfigurationListModel.h"
#include "DsaUtility.h"

namespace Dsa {

const QString ConfigurationController::URL_DEFAULT_DOWNLOAD = QString("https://usdanrcswmx.esri.com/arcgis/sharing/rest/content/items/46c2b274325c4418833624d48cb2a44a/data");

ConfigurationController::ConfigurationController(QObject* parent /* = nullptr */):
  AbstractTool(parent),
  m_configurationListModel(new ConfigurationListModel{this})
{
  connect(m_configurationListModel, &ConfigurationListModel::dataChanged, this, [this]() { emit configurationsChanged(); });

  ToolManager::instance().addTool(this);

  // set the downloads folder based on the system type
#if defined Q_OS_IOS || defined Q_OS_ANDROID
  m_downloadFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
  m_downloadFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#endif

  // populate the configurations from disk
  fetchConfigurations();
}

void ConfigurationController::select(int index)
{
  m_configurationListModel->select(index);
  storeConfigurations();
  emit configurationsChanged();
}

void ConfigurationController::download(int index)
{
  // prevent calling the download function if something is already in progress
  if (m_downloading)
    return;

  // get the url from the selected item
  auto i = m_configurationListModel->index(index, 0);
  auto url = m_configurationListModel->data(i, ConfigurationListModel::Roles::Url);
  m_activeDownloadIndex = index;

  // generate a unique name for the download
  QDir downloadFolder(m_downloadFolder);
  m_downloadFileName = downloadFolder.absoluteFilePath((QString("dsa_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))));
  QNetworkRequest networkRequest{QUrl{url.toString()}};

  // make a head request to check the availabiity of the network as well as get the final size of the package to be downloaded
  m_aborted = false;
  auto* headReply = m_networkAccessManager.head(networkRequest);
  connect(headReply, &QNetworkReply::finished, this, [this, headReply, networkRequest]()
  {
    // get the total bytes that will be downloaded
    bool convertedOk = false;
    int bytesToDownload = 0;
    auto contentLengthVariant = headReply->header(QNetworkRequest::ContentLengthHeader);
    if (contentLengthVariant.isValid())
      bytesToDownload = contentLengthVariant.toInt(&convertedOk);

    // clean up the head reply
    headReply->deleteLater();

    // abort if the download size could not be fetched
    if (!convertedOk || bytesToDownload <= 0)
    {
      emit toolErrorOccurred("Unable to fetch the download size.", "Network Error");
      return;
    }

    // make sure the device has enough room for the download
    QStorageInfo storageInfo(m_downloadFolder);
    auto bytesAvailable = storageInfo.bytesAvailable() * 0.95;
    if (bytesToDownload > bytesAvailable)
    {
      emit toolErrorOccurred("The total download size exceeds storage available on the device", "Download Error");
      return;
    }

    // reset the properties for download status and use the network manager to download the file
    m_networkReply = m_networkAccessManager.get(networkRequest);
    m_downloading = true;
    connect(m_networkReply, &QNetworkReply::downloadProgress, this, &ConfigurationController::downloadProgress);
    connect(m_networkReply, &QNetworkReply::readyRead, this, &ConfigurationController::readyRead);
    connect(m_networkReply, &QNetworkReply::finished, this, &ConfigurationController::finished);
    connect(m_networkReply, &QNetworkReply::errorOccurred, this, &ConfigurationController::downloadErrorOccurred);
  });
}

void ConfigurationController::cancel(int index)
{
  Q_UNUSED(index); // TODO: supporting multiple downloads at a time will require additional management
  if (m_networkReply)
  {
    if (m_networkReply != nullptr)
    {
      if (m_networkReply->isOpen())
      {
        m_configurationListModel->cancel(index);
        m_aborted = true;
        m_downloading = false;
        m_networkReply->abort();
        m_networkReply->deleteLater();
        m_networkReply = nullptr;
      }
    }
  }
}

void ConfigurationController::remove(int index)
{
  // make extracted packages removeable from device
  Q_UNUSED(index);
}

void ConfigurationController::storeConfigurations()
{
  QJsonArray array;
  for (const auto& cfg : *m_configurationListModel)
  {
    QJsonObject obj;
    obj["selected"] = cfg.selected();
    obj["name"] = cfg.name();
    obj["url"] = cfg.url();
    array.append(obj);
  }
  QJsonDocument doc{array};
  QFile file{DsaUtility::configurationsFilePath()};
  if (file.open(QIODevice::WriteOnly))
    file.write(doc.toJson());
}

bool ConfigurationController::createDefaultConfigurationsFile()
{
  // create a single entry for the default configuration
  QJsonObject obj;
  obj["selected"] = true;
  obj["name"] = "Default";
  obj["url"] = URL_DEFAULT_DOWNLOAD;

  // create a new document as a top level array and append the default object
  QJsonArray array;
  array.append(obj);
  QJsonDocument doc{array};

  // write out the document to disk
  QFile file{DsaUtility::configurationsFilePath()};
  if (!file.open(QIODevice::WriteOnly))
    return false;
  file.write(doc.toJson());
  return true;
}

QString ConfigurationController::toolName() const
{
  return QStringLiteral("configurations");
}

void ConfigurationController::setProperties(const QVariantMap& properties)
{
  Q_UNUSED(properties);
}

bool ConfigurationController::downloading()
{
  return m_downloading;
}

QAbstractListModel* ConfigurationController::configurations() const
{
  return m_configurationListModel;
}

void ConfigurationController::downloadProgress(quint64 bytesReceived, quint64 bytesTotal)
{
  setPercentComplete(bytesReceived * 1.0 / bytesTotal * 1.0 * 100);
}

void ConfigurationController::readyRead()
{
  // make sure the download file can be opened for appending
  QFile file(m_downloadFileName);
  if (!file.open(QIODevice::Append))
  {
    m_aborted = true;
    return;
  }

  // append the latest chunk to the download file in progress
  file.write(m_networkReply->readAll());
}

void ConfigurationController::finished()
{
  // turn off the downloading state and clean up any aborted download files remaining
  m_downloading = false;
  if (m_aborted)
  {
    QFile fileAbort(m_downloadFileName);
    if (fileAbort.exists())
      fileAbort.remove();
    m_aborted = false;
    setPercentComplete(0);
    return;
  }

  // scope block to cleanup qfile to finish the writing of the stream
  {
    QFile fileFinish{m_downloadFileName};
    if (!fileFinish.open(QIODevice::Append))
    {
      emit toolErrorOccurred("Unable to complete download", "Error Downloading");
      return;
    }

    fileFinish.write(m_networkReply->readAll());
    fileFinish.flush();
    m_networkReply->deleteLater();
    m_networkReply = nullptr;
  }
  setPercentComplete(100);

  // extract the downloaded file with a new instance of the ziphelper class
  m_zipHelper = new ZipHelper(m_downloadFileName, this);
  connect(m_zipHelper, &ZipHelper::extractCompleted, this, &ConfigurationController::extractCompleted);
  connect(m_zipHelper, &ZipHelper::extractError, this, &ConfigurationController::extractError);

  // extract the file to the associated configuration folder
  auto activeDownloadIndex = m_configurationListModel->index(m_activeDownloadIndex);
  auto activeDownloadName = m_configurationListModel->data(activeDownloadIndex, ConfigurationListModel::Roles::Name).toString();
  QDir dirConfigurations{DsaUtility::configurationsDirectoryPath()};
  QDir dirActiveDownloadConfiguration{dirConfigurations.filePath(activeDownloadName)};
  if (!dirActiveDownloadConfiguration.exists())
    dirConfigurations.mkdir(activeDownloadName);
  m_zipHelper->extractAll(dirActiveDownloadConfiguration.absolutePath());
}

void ConfigurationController::downloadErrorOccurred(QNetworkReply::NetworkError error)
{
  m_aborted = true;
  if (error != QNetworkReply::NetworkError::OperationCanceledError)
    emit toolErrorOccurred(QString("Network Error (%1)").arg(error), "Error Downloading");
}

void ConfigurationController::extractCompleted()
{
  // delete the temporary download from the device
  QFile::remove(m_downloadFileName);
  setPercentComplete(100);
  m_zipHelper->deleteLater();
}

void ConfigurationController::extractError(const QString& fileName, const QString& outputFileName, ZipHelper::Result result)
{
  Q_UNUSED(result);
  emit toolErrorOccurred(QString("error %1, %2").arg(fileName, outputFileName), "Error Unzipping");
  QFile::remove(m_downloadFileName);
  setPercentComplete(0);
  m_zipHelper->deleteLater();
}

void ConfigurationController::setPercentComplete(int percentComplete)
{
  auto i = m_configurationListModel->index(m_activeDownloadIndex, 0);
  m_configurationListModel->setData(i, percentComplete, ConfigurationListModel::Roles::PercentDownloaded);
  emit configurationsChanged();
}

void ConfigurationController::fetchConfigurations()
{
  // check for the default configurations file and create if it doesn't exist
  m_configurationListModel->removeRows(0, m_configurationListModel->rowCount(QModelIndex{}));
  QFile fileConfigurations{DsaUtility::configurationsFilePath()};
  QDir dirConfigurations{DsaUtility::configurationsDirectoryPath()};
  if (fileConfigurations.open(QIODevice::ReadOnly))
  {
    QJsonParseError parseError;
    const auto docConfigurations = QJsonDocument::fromJson(fileConfigurations.readAll(), &parseError);
    bool alreadySelected = false;
    if (!docConfigurations.isNull() && !docConfigurations.isEmpty() && docConfigurations.isArray())
    {
      const auto configurationsArray = docConfigurations.array();
      for (const auto& configurationNode : configurationsArray)
      {
        if (configurationNode.isObject())
        {
          // add a new item
          const auto configurationObject = configurationNode.toObject();

          // determine if the configuration has been downloaded by checking for the configuration file and at least
          // some other files in the directory
          bool downloaded = false;
          auto configurationName = configurationObject["name"].toString();
          auto configurationDirectoryPath = dirConfigurations.filePath(configurationName);
          QDir dirConfiguration{configurationDirectoryPath};
          if (dirConfiguration.exists())
          {
            QFile fileSettings{dirConfiguration.filePath("DsaAppConfig.json")};
            if (fileSettings.exists())
            {
              QDirIterator dirIt(configurationDirectoryPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
              auto filesInConfigurationFolder = 0;
              while (dirIt.hasNext())
              {
                filesInConfigurationFolder++;
                QFile f{dirIt.next()};
                qDebug() << f.fileName();
                if (filesInConfigurationFolder > 1)
                {
                  downloaded = true;
                  break;
                }
              }
            }
          }

          // select the first item marked as selected
          bool selected = false;
          if (!alreadySelected)
          {
            selected = configurationObject["selected"].toBool();
            alreadySelected = selected;
          }

          // add the new entry
          m_configurationListModel->add(configurationName,
                                        configurationObject["url"].toString(),
                                        selected,
                                        selected && downloaded,
                                        downloaded ? 100 : 0);
        }
      }
    }
  }
  emit configurationsChanged();
}

}

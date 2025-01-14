/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qtversionmanager.h"

#include "qtkitinformation.h"
#include "qtversionfactory.h"
#include "baseqtversion.h"
#include "qtsupportconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/filesystemwatcher.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QStringList>
#include <QTimer>

using namespace Utils;

namespace QtSupport {

using namespace Internal;

const char QTVERSION_DATA_KEY[] = "QtVersion.";
const char QTVERSION_TYPE_KEY[] = "QtVersion.Type";
const char QTVERSION_FILE_VERSION_KEY[] = "Version";
const char QTVERSION_FILENAME[] = "/qtcreator/qtversion.xml";
const char QTVERSION_LEGACY_FILENAME[] = "/qtversion.xml"; // TODO: pre 2.6, remove later

static QMap<int, BaseQtVersion *> m_versions;
static int m_idcount = 0;
// managed by QtProjectManagerPlugin
static QtVersionManager *m_instance = 0;
static FileSystemWatcher *m_configFileWatcher = 0;
static QTimer *m_fileWatcherTimer = 0;
static PersistentSettingsWriter *m_writer = 0;

enum { debug = 0 };

static FileName globalSettingsFileName()
{
    QSettings *globalSettings = ExtensionSystem::PluginManager::globalSettings();
    return FileName::fromString(QFileInfo(globalSettings->fileName()).absolutePath()
                                       + QLatin1String(QTVERSION_FILENAME));
}

static FileName settingsFileName(const QString &path)
{
    QSettings *settings = ExtensionSystem::PluginManager::settings();
    QFileInfo settingsLocation(settings->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + path);
}


// prefer newer qts otherwise compare on id
bool qtVersionNumberCompare(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->qtVersion() > b->qtVersion() || (a->qtVersion() == b->qtVersion() && a->uniqueId() < b->uniqueId());
}
static bool restoreQtVersions();
static void findSystemQt();
static void saveQtVersions();
static void updateDocumentation();


// --------------------------------------------------------------------------
// QtVersionManager
// --------------------------------------------------------------------------

QtVersionManager::QtVersionManager()
{
    m_instance = this;
    m_configFileWatcher = 0;
    m_fileWatcherTimer = new QTimer(this);
    m_writer = 0;
    m_idcount = 1;

    qRegisterMetaType<FileName>();

    // Give the file a bit of time to settle before reading it...
    m_fileWatcherTimer->setInterval(2000);
    connect(m_fileWatcherTimer, SIGNAL(timeout()), SLOT(updateFromInstaller()));
}

void QtVersionManager::triggerQtVersionRestore()
{
    disconnect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainsLoaded()),
               this, SLOT(triggerQtVersionRestore()));

    bool success = restoreQtVersions();
    m_instance->updateFromInstaller(false);
    if (!success) {
        // We did neither restore our settings or upgraded
        // in that case figure out if there's a qt in path
        // and add it to the Qt versions
        findSystemQt();
    }

    emit m_instance->qtVersionsLoaded();
    emit m_instance->qtVersionsChanged(m_versions.keys(), QList<int>(), QList<int>());
    saveQtVersions();

    const FileName configFileName = globalSettingsFileName();
    if (configFileName.exists()) {
        m_configFileWatcher = new FileSystemWatcher(m_instance);
        connect(m_configFileWatcher, SIGNAL(fileChanged(QString)),
                m_fileWatcherTimer, SLOT(start()));
        m_configFileWatcher->addFile(configFileName.toString(),
                                     FileSystemWatcher::WatchModifiedDate);
    } // exists
}

bool QtVersionManager::delayedInitialize()
{
    updateDocumentation();
    return true;
}

bool QtVersionManager::isLoaded()
{
    return m_writer;
}

QtVersionManager::~QtVersionManager()
{
    delete m_writer;
    qDeleteAll(m_versions);
    m_versions.clear();
}

void QtVersionManager::initialized()
{
    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainsLoaded()),
            QtVersionManager::instance(), SLOT(triggerQtVersionRestore()));
}

QtVersionManager *QtVersionManager::instance()
{
    return m_instance;
}

static bool restoreQtVersions()
{
    QTC_ASSERT(!m_writer, return false);
    m_writer = new PersistentSettingsWriter(settingsFileName(QLatin1String(QTVERSION_FILENAME)),
                                                   QLatin1String("QtCreatorQtVersions"));

    QList<QtVersionFactory *> factories = ExtensionSystem::PluginManager::getObjects<QtVersionFactory>();

    PersistentSettingsReader reader;
    FileName filename = settingsFileName(QLatin1String(QTVERSION_FILENAME));

    // Read Qt Creator 2.5 qtversions.xml once:
    if (!filename.exists())
        filename = settingsFileName(QLatin1String(QTVERSION_LEGACY_FILENAME));
    if (!reader.load(filename))
        return false;
    QVariantMap data = reader.restoreValues();

    // Check version:
    int version = data.value(QLatin1String(QTVERSION_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return false;

    const QString keyPrefix = QLatin1String(QTVERSION_DATA_KEY);
    const QVariantMap::ConstIterator dcend = data.constEnd();
    for (QVariantMap::ConstIterator it = data.constBegin(); it != dcend; ++it) {
        const QString &key = it.key();
        if (!key.startsWith(keyPrefix))
            continue;
        bool ok;
        int count = key.mid(keyPrefix.count()).toInt(&ok);
        if (!ok || count < 0)
            continue;

        const QVariantMap qtversionMap = it.value().toMap();
        const QString type = qtversionMap.value(QLatin1String(QTVERSION_TYPE_KEY)).toString();

        bool restored = false;
        foreach (QtVersionFactory *f, factories) {
            if (f->canRestore(type)) {
                if (BaseQtVersion *qtv = f->restore(type, qtversionMap)) {
                    if (m_versions.contains(qtv->uniqueId())) {
                        // This shouldn't happen, we are restoring the same id multiple times?
                        qWarning() << "A Qt version with id"<<qtv->uniqueId()<<"already exists";
                        delete qtv;
                    } else {
                        m_versions.insert(qtv->uniqueId(), qtv);
                        m_idcount = qtv->uniqueId() > m_idcount ? qtv->uniqueId() : m_idcount;
                        restored = true;
                        break;
                    }
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore Qt version '%s' stored in %s.",
                     qPrintable(type),
                     qPrintable(filename.toUserOutput()));
    }
    ++m_idcount;

    return true;
}

void QtVersionManager::updateFromInstaller(bool emitSignal)
{
    m_fileWatcherTimer->stop();

    const FileName path = globalSettingsFileName();
    // Handle overwritting of data:
    if (m_configFileWatcher) {
        m_configFileWatcher->removeFile(path.toString());
        m_configFileWatcher->addFile(path.toString(), FileSystemWatcher::WatchModifiedDate);
    }

    QList<int> added;
    QList<int> removed;
    QList<int> changed;

    QList<QtVersionFactory *> factories = ExtensionSystem::PluginManager::getObjects<QtVersionFactory>();
    PersistentSettingsReader reader;
    QVariantMap data;
    if (reader.load(path))
        data = reader.restoreValues();

    if (debug) {
        qDebug()<< "======= Existing Qt versions =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
        qDebug()<< "======= Adding sdk versions =======";
    }

    QStringList sdkVersions;

    const QString keyPrefix = QLatin1String(QTVERSION_DATA_KEY);
    const QVariantMap::ConstIterator dcend = data.constEnd();
    for (QVariantMap::ConstIterator it = data.constBegin(); it != dcend; ++it) {
        const QString &key = it.key();
        if (!key.startsWith(keyPrefix))
            continue;
        bool ok;
        int count = key.mid(keyPrefix.count()).toInt(&ok);
        if (!ok || count < 0)
            continue;

        QVariantMap qtversionMap = it.value().toMap();
        const QString type = qtversionMap.value(QLatin1String(QTVERSION_TYPE_KEY)).toString();
        const QString autoDetectionSource = qtversionMap.value(QLatin1String("autodetectionSource")).toString();
        sdkVersions << autoDetectionSource;
        int id = -1; // see BaseQtVersion::fromMap()
        QtVersionFactory *factory = 0;
        foreach (QtVersionFactory *f, factories) {
            if (f->canRestore(type))
                factory = f;
        }
        if (!factory) {
            if (debug)
                qDebug("Warning: Unable to find factory for type '%s'", qPrintable(type));
            continue;
        }
        // First try to find a existing Qt version to update
        bool restored = false;
        foreach (BaseQtVersion *v, m_versions) {
            if (v->autodetectionSource() == autoDetectionSource) {
                id = v->uniqueId();
                if (debug)
                    qDebug() << " Qt version found with same autodetection source" << autoDetectionSource << " => Migrating id:" << id;
                m_versions.remove(id);
                qtversionMap[QLatin1String(Constants::QTVERSIONID)] = id;
                qtversionMap[QLatin1String(Constants::QTVERSIONNAME)] = v->unexpandedDisplayName();
                delete v;

                if (BaseQtVersion *qtv = factory->restore(type, qtversionMap)) {
                    Q_ASSERT(qtv->isAutodetected());
                    m_versions.insert(id, qtv);
                    restored = true;
                }
                if (restored)
                    changed << id;
                else
                    removed << id;
            }
        }
        // Create a new qtversion
        if (!restored) { // didn't replace any existing versions
            if (debug)
                qDebug() << " No Qt version found matching" << autoDetectionSource << " => Creating new version";
            if (BaseQtVersion *qtv = factory->restore(type, qtversionMap)) {
                Q_ASSERT(qtv->isAutodetected());
                m_versions.insert(qtv->uniqueId(), qtv);
                added << qtv->uniqueId();
                restored = true;
            }
        }
        if (!restored)
            if (debug)
                qDebug("Warning: Unable to update qtversion '%s' from sdk installer.",
                       qPrintable(autoDetectionSource));
    }

    if (debug) {
        qDebug() << "======= Before removing outdated sdk versions =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
    }
    foreach (BaseQtVersion *qtVersion, m_versions) {
        if (qtVersion->autodetectionSource().startsWith(QLatin1String("SDK."))) {
            if (!sdkVersions.contains(qtVersion->autodetectionSource())) {
                if (debug)
                    qDebug() << "  removing version"<<qtVersion->autodetectionSource();
                m_versions.remove(qtVersion->uniqueId());
                removed << qtVersion->uniqueId();
            }
        }
    }

    if (debug) {
        qDebug()<< "======= End result =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
    }
    if (emitSignal)
        emit qtVersionsChanged(added, removed, changed);
}

static void saveQtVersions()
{
    if (!m_writer)
        return;

    QVariantMap data;
    data.insert(QLatin1String(QTVERSION_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (BaseQtVersion *qtv, m_versions) {
        QVariantMap tmp = qtv->toMap();
        if (tmp.isEmpty())
            continue;
        tmp.insert(QLatin1String(QTVERSION_TYPE_KEY), qtv->type());
        data.insert(QString::fromLatin1(QTVERSION_DATA_KEY) + QString::number(count), tmp);
        ++count;

    }
    m_writer->save(data, Core::ICore::mainWindow());
}

// Executes qtchooser with arguments in a process and returns its output
static QList<QByteArray> runQtChooser(const QString &qtchooser, const QStringList &arguments)
{
    QProcess p;
    p.start(qtchooser, arguments);
    p.waitForFinished();
    const bool success = p.exitCode() == 0;
    return success ? p.readAllStandardOutput().split('\n') : QList<QByteArray>();
}

// Asks qtchooser for the qmake path of a given version
static QString qmakePath(const QString &qtchooser, const QString &version)
{
    QList<QByteArray> outputs = runQtChooser(qtchooser, QStringList()
                                             << QStringLiteral("-qt=%1").arg(version)
                                             << QStringLiteral("-print-env"));
    foreach (const QByteArray &output, outputs) {
        if (output.startsWith("QTTOOLDIR=\"")) {
            QByteArray withoutVarName = output.mid(11); // remove QTTOOLDIR="
            withoutVarName.chop(1); // remove trailing quote
            return QStandardPaths::findExecutable(QStringLiteral("qmake"), QStringList()
                                                  << QString::fromLocal8Bit(withoutVarName));
        }
    }
    return QString();
}

static FileNameList gatherQmakePathsFromQtChooser()
{
    const QString qtchooser = QStandardPaths::findExecutable(QStringLiteral("qtchooser"));
    if (qtchooser.isEmpty())
        return FileNameList();

    QList<QByteArray> versions = runQtChooser(qtchooser, QStringList() << QStringLiteral("-l"));
    QSet<FileName> foundQMakes;
    foreach (const QByteArray &version, versions) {
        FileName possibleQMake = FileName::fromString(
                    qmakePath(qtchooser, QString::fromLocal8Bit(version)));
        if (!possibleQMake.isEmpty())
            foundQMakes << possibleQMake;
    }
    return foundQMakes.toList();
}

static void findSystemQt()
{
    FileNameList systemQMakes;
    FileName systemQMakePath = BuildableHelperLibrary::findSystemQt(Environment::systemEnvironment());
    if (!systemQMakePath.isEmpty())
        systemQMakes << systemQMakePath;

    systemQMakes.append(gatherQmakePathsFromQtChooser());
    systemQMakes.removeDuplicates();

    foreach (const FileName &qmakePath, systemQMakes) {
        BaseQtVersion *version
                = QtVersionFactory::createQtVersionFromQMakePath(qmakePath, false, QLatin1String("PATH"));
        if (version) {
            version->setUnexpandedDisplayName(BaseQtVersion::defaultUnexpandedDisplayName(qmakePath, true));
            m_versions.insert(version->uniqueId(), version);
        }
    }
}

void QtVersionManager::addVersion(BaseQtVersion *version)
{
    QTC_ASSERT(m_writer, return);
    QTC_ASSERT(version != 0, return);
    if (m_versions.contains(version->uniqueId()))
        return;

    int uniqueId = version->uniqueId();
    m_versions.insert(uniqueId, version);

    emit m_instance->qtVersionsChanged(QList<int>() << uniqueId, QList<int>(), QList<int>());
    saveQtVersions();
}

void QtVersionManager::removeVersion(BaseQtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    m_versions.remove(version->uniqueId());
    emit m_instance->qtVersionsChanged(QList<int>(), QList<int>() << version->uniqueId(), QList<int>());
    saveQtVersions();
    delete version;
}

static void updateDocumentation()
{
    QStringList files;
    foreach (BaseQtVersion *v, m_versions) {
        const QStringList docPaths = QStringList() << v->documentationPath() + QLatin1Char('/')
                                                   << v->documentationPath() + QLatin1String("/qch/");
        foreach (const QString &docPath, docPaths) {
            const QDir versionHelpDir(docPath);
            foreach (const QString &helpFile,
                     versionHelpDir.entryList(QStringList() << QLatin1String("*.qch"), QDir::Files))
                files << docPath + helpFile;
        }
    }
    Core::HelpManager::registerDocumentation(files);
}

void QtVersionManager::updateDumpFor(const FileName &qmakeCommand)
{
    foreach (BaseQtVersion *v, unsortedVersions()) {
        if (v->qmakeCommand() == qmakeCommand)
            v->recheckDumper();
    }
    emit dumpUpdatedFor(qmakeCommand);
}

int QtVersionManager::getUniqueId()
{
    return m_idcount++;
}

QList<BaseQtVersion *> QtVersionManager::unsortedVersions()
{
    QList<BaseQtVersion *> versions;
    QTC_ASSERT(isLoaded(), return versions);
    return m_versions.values();
}

QList<BaseQtVersion *> QtVersionManager::versions()
{
    QList<BaseQtVersion *> versions;
    QTC_ASSERT(isLoaded(), return versions);
    foreach (BaseQtVersion *version, m_versions)
        versions << version;
    Utils::sort(versions, qtVersionNumberCompare);
    return versions;
}

QList<BaseQtVersion *> QtVersionManager::validVersions()
{
    QList<BaseQtVersion *> results;
    QTC_ASSERT(isLoaded(), return results);
    foreach (BaseQtVersion *v, m_versions) {
        if (v->isValid())
            results.append(v);
    }
    Utils::sort(results, qtVersionNumberCompare);
    return results;
}

bool QtVersionManager::isValidId(int id)
{
    QTC_ASSERT(isLoaded(), return false);
    return m_versions.contains(id);
}

BaseQtVersion *QtVersionManager::version(int id)
{
    QTC_ASSERT(isLoaded(), return 0);
    QMap<int, BaseQtVersion *>::const_iterator it = m_versions.constFind(id);
    if (it == m_versions.constEnd())
        return 0;
    return it.value();
}

// This function is really simplistic...
static bool equals(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->equals(b);
}

void QtVersionManager::setNewQtVersions(QList<BaseQtVersion *> newVersions)
{
    // We want to preserve the same order as in the settings dialog
    // so we sort a copy
    QList<BaseQtVersion *> sortedNewVersions = newVersions;
    Utils::sort(sortedNewVersions, [](const BaseQtVersion *l, const BaseQtVersion *r) {
        return l->uniqueId() < r->uniqueId();
    });

    QList<int> addedVersions;
    QList<int> removedVersions;
    QList<int> changedVersions;
    // So we trying to find the minimal set of changed versions,
    // iterate over both sorted list

    // newVersions and oldVersions iterator
    QList<BaseQtVersion *>::const_iterator nit, nend;
    QMap<int, BaseQtVersion *>::const_iterator oit, oend;
    nit = sortedNewVersions.constBegin();
    nend = sortedNewVersions.constEnd();
    oit = m_versions.constBegin();
    oend = m_versions.constEnd();

    while (nit != nend && oit != oend) {
        int nid = (*nit)->uniqueId();
        int oid = (*oit)->uniqueId();
        if (nid < oid) {
            addedVersions.push_back(nid);
            ++nit;
        } else if (oid < nid) {
            removedVersions.push_back(oid);
            ++oit;
        } else {
            if (!equals(*oit, *nit))
                changedVersions.push_back(oid);
            ++oit;
            ++nit;
        }
    }

    while (nit != nend) {
        addedVersions.push_back((*nit)->uniqueId());
        ++nit;
    }

    while (oit != oend) {
        removedVersions.push_back((*oit)->uniqueId());
        ++oit;
    }

    qDeleteAll(m_versions);
    m_versions.clear();
    foreach (BaseQtVersion *v, sortedNewVersions)
        m_versions.insert(v->uniqueId(), v);

    if (!changedVersions.isEmpty() || !addedVersions.isEmpty() || !removedVersions.isEmpty())
        updateDocumentation();

    saveQtVersions();

    if (!changedVersions.isEmpty() || !addedVersions.isEmpty() || !removedVersions.isEmpty())
        emit m_instance->qtVersionsChanged(addedVersions, removedVersions, changedVersions);
}

BaseQtVersion *QtVersionManager::qtVersionForQMakeBinary(const FileName &qmakePath)
{
    return Utils::findOrDefault(unsortedVersions(), Utils::equal(&BaseQtVersion::qmakeCommand, qmakePath));
}

} // namespace QtVersion

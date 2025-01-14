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

#include "qtwizard.h"

#include "modulespage.h"

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanager.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <coreplugin/icore.h>

#include <cpptools/cpptoolsconstants.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/targetsetuppage.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QCoreApplication>
#include <QVariant>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace QtSupport;

// -------------------- QtWizard
QtWizard::QtWizard()
{
    setWizardKind(Core::IWizardFactory::ProjectWizard);
}

QString QtWizard::sourceSuffix()
{
    return preferredSuffix(QLatin1String(ProjectExplorer::Constants::CPP_SOURCE_MIMETYPE));
}

QString QtWizard::headerSuffix()
{
    return preferredSuffix(QLatin1String(ProjectExplorer::Constants::CPP_HEADER_MIMETYPE));
}

QString QtWizard::formSuffix()
{
    return preferredSuffix(QLatin1String(ProjectExplorer::Constants::FORM_MIMETYPE));
}

QString QtWizard::profileSuffix()
{
    return preferredSuffix(QLatin1String(Constants::PROFILE_MIMETYPE));
}

bool QtWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l, errorMessage);
}

bool QtWizard::qt4ProjectPostGenerateFiles(const QWizard *w,
                                           const Core::GeneratedFiles &generatedFiles,
                                           QString *errorMessage)
{
    const BaseQmakeProjectWizardDialog *dialog = qobject_cast<const BaseQmakeProjectWizardDialog *>(w);

    // Generate user settings
    foreach (const Core::GeneratedFile &file, generatedFiles)
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            dialog->writeUserFile(file.path());
            break;
        }

    // Post-Generate: Open the projects/editors
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(generatedFiles ,errorMessage);
}

QString QtWizard::templateDir()
{
    QString rc = Core::ICore::resourcePath();
    rc += QLatin1String("/templates/qt4project");
    return rc;
}

bool QtWizard::lowerCaseFiles()
{
    QString lowerCaseSettingsKey = QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP);
    lowerCaseSettingsKey += QLatin1Char('/');
    lowerCaseSettingsKey += QLatin1String(CppTools::Constants::LOWERCASE_CPPFILES_KEY);
    const bool lowerCaseDefault = CppTools::Constants::lowerCaseFilesDefault;
    return Core::ICore::settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

// ------------ CustomQmakeProjectWizard
CustomQmakeProjectWizard::CustomQmakeProjectWizard()
{
}

Core::BaseFileWizard *CustomQmakeProjectWizard::create(QWidget *parent,
                                          const Core::WizardDialogParameters &parameters) const
{
    BaseQmakeProjectWizardDialog *wizard = new BaseQmakeProjectWizardDialog(this, false, parent,
                                                                            parameters);

    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        wizard->addTargetSetupPage(targetPageId);

    initProjectWizardDialog(wizard, parameters.defaultPath(), wizard->extensionPages());
    return wizard;
}

bool CustomQmakeProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                                                 QString *errorMessage) const
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l, errorMessage);
}

// ----------------- BaseQmakeProjectWizardDialog
BaseQmakeProjectWizardDialog::BaseQmakeProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                           bool showModulesPage, QWidget *parent,
                                                           const Core::WizardDialogParameters &parameters) :
    ProjectExplorer::BaseProjectWizardDialog(factory, parent, parameters),
    m_modulesPage(0),
    m_targetSetupPage(0),
    m_profileIds(parameters.extraValues().value(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS))
                 .value<QList<Core::Id> >())
{
    init(showModulesPage);
}

BaseQmakeProjectWizardDialog::BaseQmakeProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                           bool showModulesPage,
                                                           Utils::ProjectIntroPage *introPage,
                                                           int introId, QWidget *parent,
                                                           const Core::WizardDialogParameters &parameters) :
    ProjectExplorer::BaseProjectWizardDialog(factory, introPage, introId, parent, parameters),
    m_modulesPage(0),
    m_targetSetupPage(0),
    m_profileIds(parameters.extraValues().value(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS))
                 .value<QList<Core::Id> >())
{
    init(showModulesPage);
}

BaseQmakeProjectWizardDialog::~BaseQmakeProjectWizardDialog()
{
    if (m_targetSetupPage && !m_targetSetupPage->parent())
        delete m_targetSetupPage;
    if (m_modulesPage && !m_modulesPage->parent())
        delete m_modulesPage;
}

void BaseQmakeProjectWizardDialog::init(bool showModulesPage)
{
    if (showModulesPage)
        m_modulesPage = new ModulesPage;
    connect(this, &BaseProjectWizardDialog::projectParametersChanged,
            this, &BaseQmakeProjectWizardDialog::generateProfileName);
}

int BaseQmakeProjectWizardDialog::addModulesPage(int id)
{
    if (!m_modulesPage)
        return -1;
    if (id >= 0) {
        setPage(id, m_modulesPage);
        return id;
    }
    const int newId = addPage(m_modulesPage);
    return newId;
}

int BaseQmakeProjectWizardDialog::addTargetSetupPage(int id)
{
    m_targetSetupPage = new ProjectExplorer::TargetSetupPage;
    const QString platform = selectedPlatform();
    Core::FeatureSet features = Core::FeatureSet(QtSupport::Constants::FEATURE_DESKTOP);
    if (platform.isEmpty())
        m_targetSetupPage->setPreferredKitMatcher(QtKitInformation::qtVersionMatcher(features));
    else
        m_targetSetupPage->setPreferredKitMatcher(QtKitInformation::platformMatcher(platform));

    m_targetSetupPage->setRequiredKitMatcher(QtKitInformation::qtVersionMatcher(requiredFeatures()));

    resize(900, 450);
    if (id >= 0)
        setPage(id, m_targetSetupPage);
    else
        id = addPage(m_targetSetupPage);

    return id;
}

QStringList BaseQmakeProjectWizardDialog::selectedModulesList() const
{
    return m_modulesPage ? m_modulesPage->selectedModulesList() : m_selectedModules;
}

void BaseQmakeProjectWizardDialog::setSelectedModules(const QString &modules, bool lock)
{
    const QStringList modulesList = modules.split(QLatin1Char(' '));
    if (m_modulesPage) {
        foreach (const QString &module, modulesList) {
            m_modulesPage->setModuleSelected(module, true);
            m_modulesPage->setModuleEnabled(module, !lock);
        }
    } else {
        m_selectedModules = modulesList;
    }
}

QStringList BaseQmakeProjectWizardDialog::deselectedModulesList() const
{
    return m_modulesPage ? m_modulesPage->deselectedModulesList() : m_deselectedModules;
}

void BaseQmakeProjectWizardDialog::setDeselectedModules(const QString &modules)
{
    const QStringList modulesList = modules.split(QLatin1Char(' '));
    if (m_modulesPage) {
        foreach (const QString &module, modulesList)
            m_modulesPage->setModuleSelected(module, false);
    } else {
        m_deselectedModules = modulesList;
    }
}

bool BaseQmakeProjectWizardDialog::writeUserFile(const QString &proFileName) const
{
    if (!m_targetSetupPage)
        return false;

    QmakeManager *manager = ExtensionSystem::PluginManager::getObject<QmakeManager>();
    Q_ASSERT(manager);

    QmakeProject *pro = new QmakeProject(manager, proFileName);
    bool success = m_targetSetupPage->setupProject(pro);
    if (success)
        pro->saveSettings();
    delete pro;
    return success;
}

bool BaseQmakeProjectWizardDialog::setupProject(QmakeProject *project) const
{
    if (!m_targetSetupPage)
        return true;
    return m_targetSetupPage->setupProject(project);
}

bool BaseQmakeProjectWizardDialog::isQtPlatformSelected(const QString &platform) const
{
    QList<Core::Id> selectedKitList = selectedKits();

    foreach (Kit *k, KitManager::matchingKits(QtKitInformation::platformMatcher(platform)))
        if (selectedKitList.contains(k->id()))
            return true;

    return false;
}

QList<Core::Id> BaseQmakeProjectWizardDialog::selectedKits() const
{
    if (!m_targetSetupPage)
        return m_profileIds;
    return m_targetSetupPage->selectedKits();
}

void BaseQmakeProjectWizardDialog::addExtensionPages(const QList<QWizardPage *> &wizardPageList)
{
    foreach (QWizardPage *p,wizardPageList)
        addPage(p);
}

void BaseQmakeProjectWizardDialog::generateProfileName(const QString &name, const QString &path)
{
    if (!m_targetSetupPage)
        return;

    const QString proFile =
        QDir::cleanPath(path + QLatin1Char('/') + name + QLatin1Char('/')
                        + name + QLatin1String(".pro"));

    m_targetSetupPage->setProjectPath(proFile);
}

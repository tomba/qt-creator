/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion
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

#include "clonewizard.h"
#include "clonewizardpage.h"
#include "mercurialclient.h"
#include "mercurialplugin.h"
#include "mercurialsettings.h"

#include <coreplugin/iversioncontrol.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/wizard/vcsconfigurationpage.h>

#include <utils/qtcassert.h>

using namespace VcsBase;

namespace Mercurial {
namespace Internal {
// --------------------------------------------------------------------
// CloneWizard:
// --------------------------------------------------------------------

CloneWizard::CloneWizard(const Utils::FileName &path, QWidget *parent) :
    BaseCheckoutWizard(Constants::VCS_ID_MERCURIAL, parent)
{
    setTitle(tr("Cloning"));
    setStartedStatus(tr("Cloning started..."));

    auto page = new CloneWizardPage;
    page->setPath(path.toString());
    addPage(page);
}

VcsCommand *CloneWizard::createCommand(Utils::FileName *checkoutDir)
{
    const CloneWizardPage *cwp = find<CloneWizardPage>();
    QTC_ASSERT(cwp, return 0);

    const Utils::FileName path = Utils::FileName::fromString(cwp->path());
    const QString directory = cwp->directory();

    *checkoutDir = Utils::FileName::fromString(path.toString() + QLatin1Char('/') + directory);

    return createCommandImpl(cwp->repository(), path, directory, QStringList());
}

} // namespace Internal
} // namespace Mercurial
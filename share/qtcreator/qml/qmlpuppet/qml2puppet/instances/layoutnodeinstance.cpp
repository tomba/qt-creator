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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/
#include "layoutnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

LayoutNodeInstance::LayoutNodeInstance(QQuickItem *item)
    : QuickItemNodeInstance(item)
{
}

bool LayoutNodeInstance::isLayoutable() const
{
    return true;
}

bool LayoutNodeInstance::isResizable() const
{
    return true;
}

LayoutNodeInstance::Pointer LayoutNodeInstance::create(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem*>(object);

    Q_ASSERT(item);

    Pointer instance(new LayoutNodeInstance(item));

    instance->setHasContent(anyItemHasContent(item));
    item->setFlag(QQuickItem::ItemHasContents, true);

    static_cast<QQmlParserStatus*>(item)->classBegin();

    instance->populateResetHashes();

    return instance;
}

void LayoutNodeInstance::refreshLayoutable()
{
    if (quickItem()->parent())
        QCoreApplication::postEvent(quickItem(), new QEvent(QEvent::LayoutRequest));

}

PropertyNameList LayoutNodeInstance::ignoredProperties() const
{
    return PropertyNameList() << "move" << "add" << "populate";
}

}
} // namespace QmlDesigner

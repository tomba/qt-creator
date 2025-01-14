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

#ifndef CHANGEIMPORTSVISITOR_H
#define CHANGEIMPORTSVISITOR_H

#include "import.h"
#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class ChangeImportsVisitor: public QMLRewriter
{
public:
    ChangeImportsVisitor(QmlDesigner::TextModifier &textModifier, const QString &source);

    bool add(QmlJS::AST::UiProgram *ast, const Import &import);
    bool remove(QmlJS::AST::UiProgram *ast, const Import &import);

private:
    static bool equals(QmlJS::AST::UiImport *ast, const Import &import);

    QString m_source;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // CHANGEIMPORTSVISITOR_H

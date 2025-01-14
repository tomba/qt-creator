/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QMT_DUPDATEVISITOR_H
#define QMT_DUPDATEVISITOR_H

#include "qmt/model/mconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"


namespace qmt {

class DElement;


class QMT_EXPORT DUpdateVisitor :
        public MConstVisitor
{
public:
    DUpdateVisitor(DElement *target, const MDiagram *diagram, bool check_needs_update = false);

public:

    bool updateNeeded() const { return _update_needed; }

    void setCheckNeedsUpdate(bool check_needs_update);

    void visitMElement(const MElement *element);

    void visitMObject(const MObject *object);

    void visitMPackage(const MPackage *package);

    void visitMClass(const MClass *klass);

    void visitMComponent(const MComponent *component);

    void visitMDiagram(const MDiagram *diagram);

    void visitMCanvasDiagram(const MCanvasDiagram *diagram);

    void visitMItem(const MItem *item);

    void visitMRelation(const MRelation *relation);

    void visitMDependency(const MDependency *dependency);

    void visitMInheritance(const MInheritance *inheritance);

    void visitMAssociation(const MAssociation *association);

private:

    bool isUpdating(bool value_changed);

private:

    DElement *_target;

    const MDiagram *_diagram;

    bool _check_needs_update;

    bool _update_needed;
};

}

#endif // QMT_DUPDATEVISITOR_H

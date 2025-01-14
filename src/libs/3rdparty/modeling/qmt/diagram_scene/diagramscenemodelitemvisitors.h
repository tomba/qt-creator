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

#ifndef QMT_DIAGRAMSCENEMODELITEMVISITORS_H
#define QMT_DIAGRAMSCENEMODELITEMVISITORS_H

#include "diagramscenemodel.h"

#include "qmt/diagram/dvisitor.h"

namespace qmt {

class QMT_EXPORT DiagramSceneModel::CreationVisitor :
        public DVisitor
{
public:

    CreationVisitor(DiagramSceneModel *diagram_scene_model);

public:

    QGraphicsItem *getCreatedGraphicsItem() const { return _graphics_item; }

public:

    void visitDElement(DElement *element);

    void visitDObject(DObject *object);

    void visitDPackage(DPackage *package);

    void visitDClass(DClass *klass);

    void visitDComponent(DComponent *component);

    void visitDDiagram(DDiagram *diagram);

    void visitDItem(DItem *item);

    void visitDRelation(DRelation *relation);

    void visitDInheritance(DInheritance *inheritance);

    void visitDDependency(DDependency *dependency);

    void visitDAssociation(DAssociation *association);

    void visitDAnnotation(DAnnotation *annotation);

    void visitDBoundary(DBoundary *boundary);

private:

    DiagramSceneModel *_diagram_scene_model;

    QGraphicsItem *_graphics_item;
};

class DiagramSceneModel::UpdateVisitor :
        public DVisitor
{
public:

    UpdateVisitor(QGraphicsItem *item, DiagramSceneModel *diagram_scene_model, DElement *related_element = 0);

public:

    void visitDElement(DElement *element);

    void visitDObject(DObject *object);

    void visitDPackage(DPackage *package);

    void visitDClass(DClass *klass);

    void visitDComponent(DComponent *component);

    void visitDDiagram(DDiagram *diagram);

    void visitDItem(DItem *item);

    void visitDRelation(DRelation *relation);

    void visitDInheritance(DInheritance *inheritance);

    void visitDDependency(DDependency *dependency);

    void visitDAssociation(DAssociation *association);

    void visitDAnnotation(DAnnotation *annotation);

    void visitDBoundary(DBoundary *boundary);

private:

    QGraphicsItem *_graphics_item;

    DiagramSceneModel *_diagram_scene_model;

    DElement *_related_element;
};

}

#endif // QMT_DIAGRAMSCENEMODELITEMVISITORS_H

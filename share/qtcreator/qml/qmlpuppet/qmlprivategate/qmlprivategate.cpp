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

#include "qmlprivategate.h"

#include "metaobject.h"

#include <objectnodeinstance.h>

#include <QQuickItem>
#include <QQmlComponent>

#include <private/qquicktransition_p.h>
#include <private/qquickanimation_p.h>
#include <private/qqmltimer_p.h>
#include <designersupport.h>

namespace QmlDesigner {

namespace Internal {

namespace QmlPrivateGate {

bool isPropertyBlackListed(const QmlDesigner::PropertyName &propertyName)
{
    if (propertyName.contains(".") && propertyName.contains("__"))
        return true;

    if (propertyName.count(".") > 1)
        return true;

    return false;
}

static void addToPropertyNameListIfNotBlackListed(PropertyNameList *propertyNameList, const PropertyName &propertyName)
{
    if (!isPropertyBlackListed(propertyName))
        propertyNameList->append(propertyName);
}

PropertyNameList propertyNameListForWritableProperties(QObject *object,
                                                       const PropertyName &baseName,
                                                       QObjectList *inspectedObjects)
{
    PropertyNameList propertyNameList;

    if (inspectedObjects == 0 || inspectedObjects->contains(object))
        return propertyNameList;

    inspectedObjects->append(object);

    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QQmlProperty declarativeProperty(object, QLatin1String(metaProperty.name()));
        if (declarativeProperty.isValid() && !declarativeProperty.isWritable() && declarativeProperty.propertyTypeCategory() == QQmlProperty::Object) {
            if (declarativeProperty.name() != "parent") {
                QObject *childObject = QQmlMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(propertyNameListForWritableProperties(childObject, baseName +  PropertyName(metaProperty.name()) + '.', inspectedObjects));
            }
        } else if (QQmlValueTypeFactory::valueType(metaProperty.userType())) {
            QQmlValueType *valueType = QQmlValueTypeFactory::valueType(metaProperty.userType());
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(propertyNameListForWritableProperties(valueType, baseName +  PropertyName(metaProperty.name()) + '.', inspectedObjects));
        }

        if (metaProperty.isReadable() && metaProperty.isWritable()) {
            addToPropertyNameListIfNotBlackListed(&propertyNameList, baseName + PropertyName(metaProperty.name()));
        }
    }

    return propertyNameList;
}

static void stopAnimation(QObject *object)
{
    if (object == 0)
        return;

    QQuickTransition *transition = qobject_cast<QQuickTransition*>(object);
    QQuickAbstractAnimation *animation = qobject_cast<QQuickAbstractAnimation*>(object);
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(object);
    if (transition) {
       transition->setFromState("");
       transition->setToState("");
    } else if (animation) {
//        QQuickScriptAction *scriptAimation = qobject_cast<QQuickScriptAction*>(animation);
//        if (scriptAimation) FIXME
//            scriptAimation->setScript(QQmlScriptString());
        animation->setLoops(1);
        animation->complete();
        animation->setDisableUserControl();
    } else if (timer) {
        timer->blockSignals(true);
    }
}

static void allSubObject(QObject *object, QObjectList &objectList)
{
    // don't add null pointer and stop if the object is already in the list
    if (!object || objectList.contains(object))
        return;

    objectList.append(object);

    for (int index = QObject::staticMetaObject.propertyOffset();
         index < object->metaObject()->propertyCount();
         index++) {
        QMetaProperty metaProperty = object->metaObject()->property(index);

        // search recursive in property objects
        if (metaProperty.isReadable()
                && metaProperty.isWritable()
                && QQmlMetaType::isQObject(metaProperty.userType())) {
            if (metaProperty.name() != QLatin1String("parent")) {
                QObject *propertyObject = QQmlMetaType::toQObject(metaProperty.read(object));
                allSubObject(propertyObject, objectList);
            }

        }

        // search recursive in property object lists
        if (metaProperty.isReadable()
                && QQmlMetaType::isList(metaProperty.userType())) {
            QQmlListReference list(object, metaProperty.name());
            if (list.canCount() && list.canAt()) {
                for (int i = 0; i < list.count(); i++) {
                    QObject *propertyObject = list.at(i);
                    allSubObject(propertyObject, objectList);

                }
            }
        }
    }

    // search recursive in object children list
    foreach (QObject *childObject, object->children()) {
        allSubObject(childObject, objectList);
    }

    // search recursive in quick item childItems list
    QQuickItem *quickItem = qobject_cast<QQuickItem*>(object);
    if (quickItem) {
        foreach (QQuickItem *childItem, quickItem->childItems()) {
            allSubObject(childItem, objectList);
        }
    }
}

static void fixResourcePathsForObject(QObject *object)
{
    if (qgetenv("QMLDESIGNER_RC_PATHS").isEmpty())
        return;

    PropertyNameList propertyNameList = propertyNameListForWritableProperties(object);

    foreach (const PropertyName &propertyName, propertyNameList) {
        QQmlProperty property(object, propertyName, QQmlEngine::contextForObject(object));

        const QVariant value  = property.read();
        const QVariant fixedValue = ObjectNodeInstance::fixResourcePaths(value);
        if (value != fixedValue) {
            property.write(fixedValue);
        }
    }
}

void tweakObjects(QObject *object)
{
    QObjectList objectList;
    allSubObject(object, objectList);
    foreach (QObject* childObject, objectList) {
        stopAnimation(childObject);
        fixResourcePathsForObject(childObject);
    }
}

static QObject *createDummyWindow(QQmlEngine *engine)
{
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/qtquickplugin/mockfiles/Window.qml")));
    return component.create();
}

static bool isWindowMetaObject(const QMetaObject *metaObject)
{
    if (metaObject) {
        if (metaObject->className() == QByteArrayLiteral("QWindow"))
            return true;

        return isWindowMetaObject(metaObject->superClass());
    }

    return false;
}

static bool isWindow(QObject *object) {
    if (object)
        return isWindowMetaObject(object->metaObject());

    return false;
}

static QQmlType *getQmlType(const QString &typeName, int majorNumber, int minorNumber)
{
     return QQmlMetaType::qmlType(typeName.toUtf8(), majorNumber, minorNumber);
}

static bool isCrashingType(QQmlType *type)
{
    if (type) {
        if (type->qmlTypeName() == QStringLiteral("QtMultimedia/MediaPlayer"))
            return true;

        if (type->qmlTypeName() == QStringLiteral("QtMultimedia/Audio"))
            return true;

        if (type->qmlTypeName() == QStringLiteral("QtQuick.Controls/MenuItem"))
            return true;

        if (type->qmlTypeName() == QStringLiteral("QtQuick.Controls/Menu"))
            return true;

        if (type->qmlTypeName() == QStringLiteral("QtQuick/Timer"))
            return true;
    }

    return false;
}

void readPropertyValue(QObject *object, const QByteArray &propertyName, QQmlContext *qmlContext, bool *ok)
{
#if QT_VERSION >= 0x056000
    DesignerSupport::readPropertyValue(object, propertyName, qmlContext, ok);
#else
    //DesignerSupport::readPropertyValue(object, propertyName, qmlContext, ok); //### Move to Qt
#endif
}

void createNewDynamicProperty(const ObjectNodeInstancePointer &nodeInstance, const QString &name)
{
    MetaObject::getNodeInstanceMetaObject(nodeInstance)->createNewDynamicProperty(name);
}

void registerNodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance)
{
    // we just create one and the ownership goes automatically to the object in nodeinstance see init method
    MetaObject::getNodeInstanceMetaObject(nodeInstance);
}

QObject *createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context)
{
    ComponentCompleteDisabler disableComponentComplete;

    Q_UNUSED(disableComponentComplete)

    QObject *object = 0;
    QQmlType *type = getQmlType(typeName, majorNumber, minorNumber);

    if (isCrashingType(type)) {
        object = new QObject;
    } else if (type) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)) // TODO remove hack later if we only support >= 5.2
        if ( type->isComposite()) {
             object = ObjectNodeInstance::createComponent(type->sourceUrl(), context);
        } else
#endif
        {
            if (type->typeName() == "QQmlComponent") {
                object = new QQmlComponent(context->engine(), 0);
            } else  {
                object = type->create();
            }
        }

        if (isWindow(object)) {
            delete object;
            object = createDummyWindow(context->engine());
        }

    }

    if (!object) {
        qWarning() << "QuickDesigner: Cannot create an object of type"
                   << QString("%1 %2,%3").arg(typeName).arg(majorNumber).arg(minorNumber)
                   << "- type isn't known to declarative meta type system";
    }

    tweakObjects(object);

    if (object && QQmlEngine::contextForObject(object) == 0)
        QQmlEngine::setContextForObject(object, context);

    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    return object;
}

}

ComponentCompleteDisabler::ComponentCompleteDisabler()
{
    DesignerSupport::disableComponentComplete();
}

ComponentCompleteDisabler::~ComponentCompleteDisabler()
{
    DesignerSupport::enableComponentComplete();
}

// namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner
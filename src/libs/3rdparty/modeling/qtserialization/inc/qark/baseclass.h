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

#ifndef QARK_BASECLASS_H
#define QARK_BASECLASS_H

#include "typeregistry.h"
#include "parameters.h"

#include <QString>


namespace qark {

template<class BASE, class DERIVED>
class Base {
public:
    Base(const QString &qualified_name, DERIVED &obj)
        : _qualified_name(qualified_name),
          _base(obj)
    {
    }

    Base(const QString &qualified_name, DERIVED &obj, const Parameters &parameters)
        : _qualified_name(qualified_name),
          _base(obj),
          _parameters(parameters)
    {
    }

    const QString &getQualifiedName() const { return _qualified_name; }

    const BASE &getBase() const { return _base; }

    BASE &getBase() { return _base; }

    Parameters getParameters() const { return _parameters; }

private:
    QString _qualified_name;
    BASE &_base;
    Parameters _parameters;
};

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualified_name, DERIVED &obj)
{
    return Base<BASE, DERIVED>(qualified_name, obj);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualified_name, DERIVED &obj, const Parameters &parameters)
{
    return Base<BASE, DERIVED>(qualified_name, obj, parameters);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualified_name, DERIVED *&obj)
{
    return Base<BASE, DERIVED>(qualified_name, *obj);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(const QString &qualified_name, DERIVED *&obj, const Parameters &parameters)
{
    return Base<BASE, DERIVED>(qualified_name, *obj, parameters);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(DERIVED &obj)
{
    return Base<BASE, DERIVED>(QString(QStringLiteral("base-%1")).arg(get_type_uid<BASE>()), obj);
}

template<class BASE, class DERIVED>
Base<BASE, DERIVED> base(DERIVED &obj, const Parameters &parameters)
{
    return Base<BASE, DERIVED>(QString(QStringLiteral("base-%1")).arg(get_type_uid<BASE>()), obj, parameters);
}

}

#endif // QARK_BASECLASS_H

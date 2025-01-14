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

#ifndef ONEDIMENSIONALCLUSTER_H
#define ONEDIMENSIONALCLUSTER_H

#include <QList>

namespace QmlDesigner {

class OneDimensionalCluster
{
    friend inline OneDimensionalCluster operator+(const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster);
    friend inline bool operator < (const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster);
public:
    static QList<double> reduceLines(const QList<double> & oneDimensionalCoordinateList, double maximumDistance);

private:

    OneDimensionalCluster(const QList<double> & coordinateList );
    double mean() const;
    double first() const;



    static QList<OneDimensionalCluster> createOneDimensionalClusterList(const QList<double> & oneDimensionalCoordinateList);
    static QList<OneDimensionalCluster> reduceOneDimensionalClusterList(const QList<OneDimensionalCluster> & unreducedClusterList, double maximumDistance);

    QList<double> m_coordinateList;
};

inline bool operator < (const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster)
{
    return firstCluster.mean() < secondCluster.mean();
}

inline OneDimensionalCluster operator+(const OneDimensionalCluster & firstCluster, const OneDimensionalCluster & secondCluster)
{

    return OneDimensionalCluster(firstCluster.m_coordinateList + secondCluster.m_coordinateList);
}

}
#endif // ONEDIMENSIONALCLUSTER_H

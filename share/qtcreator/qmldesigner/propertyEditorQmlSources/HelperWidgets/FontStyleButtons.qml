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

import QtQuick 2.1
import HelperWidgets 2.0

ButtonRow {

    property variant bold: backendValues.font_bold
    property variant italic: backendValues.font_italic
    property variant underline: backendValues.font_underline
    property variant strikeout: backendValues.font_strikeout

    BoolButtonRowButton {
        standardIconSource: "images/bold-icon.png"
        highlightedIconSource: "images/bold-h-icon.png"
        backendValue: bold
    }
    BoolButtonRowButton {
        standardIconSource: "images/italic-icon.png"
        highlightedIconSource: "images/italic-h-icon.png"
        backendValue: italic
    }
    BoolButtonRowButton {
        standardIconSource: "images/underline-icon.png"
        highlightedIconSource: "images/underline-h-icon.png"
        backendValue: underline
    }

    BoolButtonRowButton {
        standardIconSource: "images/strikeout-icon.png"
        highlightedIconSource: "images/strikeout-h-icon.png"
        backendValue: strikeout
    }

}

import QtQuick
import QtQuick.Controls

Item {
    required property string sender
    required property string body
    required property string timestamp
    required property bool system
    required property bool mine

    width: ListView.view.width
    height: bubble.implicitHeight + 8

    Rectangle {
        id: bubble

        radius: 14
        border.width: 1

        color: system ? "#eeeeee" : (mine ? "#7c3aed" : "#ffffff")
        border.color: system ? "#d6d6d6" : (mine ? "#7c3aed" : "#dddddd")

        anchors.horizontalCenter: system ? parent.horizontalCenter : undefined
        anchors.right: (!system && mine) ? parent.right : undefined
        anchors.left: (!system && !mine) ? parent.left : undefined

        width: Math.min(parent.width * 0.70, Math.max(80, contentColumn.implicitWidth + 22))
        implicitHeight: contentColumn.implicitHeight + 14

        Column {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Label {
                text: system ? "SYSTEM" : sender
                visible: system || !mine
                font.bold: true
                font.pixelSize: 11
                color: system ? "#555555" : "#666666"
                elide: Text.ElideRight
                width: parent.width
            }

            Label {
                id: messageText
                text: body
                wrapMode: Text.WordWrap
                font.pixelSize: 15
                color: mine && !system ? "white" : "#202020"
                width: parent.width
            }

            Label {
                text: timestamp
                font.pixelSize: 10
                horizontalAlignment: Text.AlignRight
                color: mine && !system ? "#e8dcff" : "#888888"
                width: parent.width
            }
        }
    }
}
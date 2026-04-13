import QtQuick
import QtQuick.Controls

Rectangle {
    required property string sender
    required property string body
    required property string timestamp
    required property bool system

    width: ListView.view.width
    radius: 8
    border.width: 1
    color: system ? "#f4f4f4" : "#eaeaea"
    implicitHeight: contentColumn.implicitHeight + 12

    Column {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4

        Label {
            text: sender + " [" + timestamp + "]"
            font.bold: true
        }

        Label {
            width: parent.width
            text: body
            wrapMode: Text.Wrap
        }
    }
}
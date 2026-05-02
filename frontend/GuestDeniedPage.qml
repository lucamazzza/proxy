import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import App 1.0

Page {
    signal backRequested()
    ColumnLayout {
        anchors.centerIn: parent
        width: 420
        spacing: 16

        Label {
            text: "Guest access denied"
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Guest users don't have permissions to interact with the chat. Therefore they cannot view or write in the chat."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Button {
            text: "Back to login"
            Layout.fillWidth: true

            onClicked: {
                backRequested()
            }
        }
    }
}
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import App 1.0

Page {
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
            text: "Guest users can authenticate, but they are not assigned to a channel. Therefore they cannot open the chat."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Button {
            text: "Back to login"
            Layout.fillWidth: true

            onClicked: {
                AppController.logout()
                StackView.view.clear()
                StackView.view.push("LoginChoicePage.qml")
            }
        }
    }
}
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import App 1.0

Page {
    signal emailLoginRequested()

    ColumnLayout {
        anchors.centerIn: parent
        width: 320
        spacing: 14

        Label {
            text: "AppComm Client Demo"
            font.pixelSize: 28
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            text: AppController.busy ? "Signing in..." : "Login as guest"
            enabled: !AppController.busy
            Layout.fillWidth: true

            onClicked: {
                AppController.loginAsGuest()
            }
        }

        Button {
            text: "Login with email"
            enabled: !AppController.busy
            Layout.fillWidth: true

            onClicked: {
                emailLoginRequested()
            }
        }

        Label {
            text: AppController.errorMessage
            color: "red"
            visible: text.length > 0
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
}
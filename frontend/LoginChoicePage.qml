import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

        Label {
            text: "Choose how to authenticate"
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            text: "Login as guest"
            Layout.fillWidth: true
            onClicked: {
                controller.loginAsGuest()
            }
        }

        Button {
            text: "Login with email"
            Layout.fillWidth: true
            onClicked: {
                emailLoginRequested()
            }
        }

        Label {
            text: controller.errorMessage
            color: "red"
            visible: text.length > 0
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
}
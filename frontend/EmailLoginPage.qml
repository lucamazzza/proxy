import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import App 1.0

Page {
    signal backRequested()
    ColumnLayout {
        anchors.centerIn: parent
        width: 320
        spacing: 14

        Label {
            text: "Login with email"
            font.pixelSize: 26
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: emailField
            placeholderText: "Email"
            Layout.fillWidth: true
        }

        TextField {
            id: passwordField
            placeholderText: "Password"
            echoMode: TextInput.Password
            Layout.fillWidth: true

            Keys.onReturnPressed: {
                AppController.loginWithEmail(emailField.text, passwordField.text)
            }
        }

        Button {
            text: AppController.busy ? "Logging in..." : "Login"
            enabled: !AppController.busy
            Layout.fillWidth: true

            onClicked: {
                AppController.loginWithEmail(emailField.text, passwordField.text)
            }
        }

        Button {
            text: "Back"
            enabled: !AppController.busy
            Layout.fillWidth: true

            onClicked: {
                AppController.clearError()
                backRequested()
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
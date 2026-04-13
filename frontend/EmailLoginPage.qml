import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
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
        }

        Button {
            text: "Login"
            Layout.fillWidth: true
            onClicked: {
                controller.loginWithEmail(emailField.text, passwordField.text)
            }
        }

        Button {
            text: "Back"
            Layout.fillWidth: true
            onClicked: {
                StackView.view.pop()
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
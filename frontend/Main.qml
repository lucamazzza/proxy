import QtQuick
import QtQuick.Controls
import App 1.0

ApplicationWindow {
    width: 1000
    height: 700
    visible: true
    title: "AppComm Demo"

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: LoginChoicePage {
            onEmailLoginRequested: {
                AppController.clearError()
                stackView.push(emailLoginPageComponent)
            }
        }
    }

    Component {
        id: emailLoginPageComponent

        EmailLoginPage {
            onBackRequested: {
                stackView.pop()
            }
        }
    }

    Connections {
        target: AppController

        function onLoginSucceeded() {
            stackView.clear()
            stackView.push("ChatPage.qml")
        }

        function onGuestAccessDenied() {
            stackView.clear()
            stackView.push("GuestDeniedPage.qml")
        }
    }
}
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

        initialItem: loginChoicePageComponent
    }

    Component {
        id: loginChoicePageComponent

        LoginChoicePage {
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

    Component {
        id: chatPageComponent

        ChatPage {
            onLogoutFinished: {
                stackView.clear()
                stackView.push(loginChoicePageComponent)
            }
        }
    }

    Component {
        id: guestDeniedPageComponent

        GuestDeniedPage {
            onBackRequested: {
                AppController.logout()
                stackView.clear()
                stackView.push(loginChoicePageComponent)
            }
        }
    }

    Connections {
        target: AppController

        function onLoginSucceeded() {
            stackView.clear()
            stackView.push(chatPageComponent)
        }

        function onGuestAccessDenied() {
            stackView.clear()
            stackView.push(guestDeniedPageComponent)
        }
    }
}
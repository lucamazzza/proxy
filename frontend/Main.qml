import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 1000
    height: 700
    visible: true
    title: "Appcomm Demo"

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: LoginChoicePage {
            onEmailLoginRequested: {
                stackView.push("EmailLoginPage.qml")
            }
        }
    }

    Connections {
        target: controller

        function onLoginSucceeded() {
            stackView.push("ChatPage.qml")
        }
    }
}
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import App 1.0

Page {
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8

            Label {
                text: "AppComm Chat"
                font.pixelSize: 20
                Layout.fillWidth: true
            }

            Label {
                text: "Channel: " + AppController.currentChannel
            }

            Label {
                text: "State: " + AppController.connectionState
            }

            Button {
                text: "Logout"

                onClicked: {
                    AppController.logout()
                    StackView.view.clear()
                    StackView.view.push("LoginChoicePage.qml")
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        GroupBox {
            title: "Messages"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: messageListView
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                spacing: 8
                model: AppController.messagesModel
                delegate: MessageDelegate { }

                onCountChanged: {
                    positionViewAtEnd()
                }
            }
        }

        GroupBox {
            title: "Send message"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                spacing: 8

                TextField {
                    id: messageField
                    placeholderText: "Write a message"
                    Layout.fillWidth: true
                    enabled: AppController.currentChannel !== ""

                    onAccepted: {
                        sendButton.clicked()
                    }
                }

                Button {
                    id: sendButton
                    text: "Send"
                    enabled: AppController.currentChannel !== "" &&
                             messageField.text.trim().length > 0

                    onClicked: {
                        AppController.sendMessage(messageField.text)
                        messageField.clear()
                    }
                }
            }
        }
    }
}
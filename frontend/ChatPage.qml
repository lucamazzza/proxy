import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8

            Button {
                text: "Back"
                onClicked: StackView.view.pop()
            }

            Label {
                text: "AppComm Chat"
                font.pixelSize: 20
                Layout.fillWidth: true
            }

            Label {
                text: "State: " + controller.connectionState
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        GroupBox {
            title: "Channel"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: channelField
                        placeholderText: "Insert channel ID"
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Join"
                        onClicked: controller.joinChannel(channelField.text)
                    }

                    Button {
                        text: "Leave"
                        onClicked: controller.leaveChannel()
                    }
                }

                Label {
                    text: controller.currentChannel === ""
                          ? "Current channel: none"
                          : "Current channel: " + controller.currentChannel
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
                model: controller.messagesModel
                delegate: MessageDelegate { }
                onCountChanged: positionViewAtEnd()
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
                    placeholderText: controller.currentChannel === ""
                                     ? "Join a channel first"
                                     : "Write a message"
                    Layout.fillWidth: true
                    enabled: controller.currentChannel !== ""

                    onAccepted: {
                        sendButton.clicked()
                    }
                }

                Button {
                    id: sendButton
                    text: "Send"
                    enabled: controller.currentChannel !== "" &&
                             messageField.text.trim().length > 0
                    onClicked: {
                        controller.sendMessage(messageField.text)
                        messageField.clear()
                    }
                }
            }
        }
    }
}
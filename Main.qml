import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            TextField {
                id: tickerField
                placeholderText: qsTr("Enter stock ticker")
            }

            Button {
                text: "Search"
                onClicked: {
                    result.text = tickerField.text
                }
            }
        }

        Label {
            id: result
            text: "Label"
        }
    }

    

    
}

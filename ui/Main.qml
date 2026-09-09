import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ValueApp

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    StockData {
        id: stockData
    }

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
                    stockData.fetch(tickerField.text)
                }
            }
        }

        Label {
            id: result
            text: stockData.resultText
        }
    }

    

    
}

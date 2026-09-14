import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item
{
    StockData
    {
        id: stockData
    }

    ColumnLayout
    {
        anchors.fill: parent

        RowLayout
        {
            TextField
            {
                id: tickerField
                placeholderText: qsTr("Enter stock ticker")
            }

            Button
            {
                text: "Search"
                onClicked:
                {
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
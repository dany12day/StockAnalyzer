import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ValueApp

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    id: mainWindow
    property var destinations: ["DCF", "Fundamentals"]

    AppDrawer {
        id: appDrawer
        model: mainWindow.destinations
    }

    header: ToolBar {
        ToolButton {
            text: "\u2630"
            onClicked: appDrawer.open()
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: appDrawer.currentIndex

        DcfPage { }
        FundamentalsPage { }
    }
    
}

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Drawer
{
    id: root
    property alias model: listView.model
    property alias currentIndex: listView.currentIndex
    width: Math.min((parent ? parent.width : 0) * 0.66, 300)
    height: parent ? parent.height : 0
    edge: Qt.LeftEdge
    dragMargin: 30
    modal: true
    closePolicy: Drawer.CloseOnPressOutside | Drawer.CloseOnEscape
    
    ListView
    {
        id: listView
        anchors.fill: parent
        clip: true

        delegate: ItemDelegate
        {
            required property var modelData
            required property int index

            text: modelData
            width: ListView.view.width
            highlighted: ListView.isCurrentItem

            onClicked: {
                ListView.view.currentIndex = index
                root.close()
            }
        }
    }
}
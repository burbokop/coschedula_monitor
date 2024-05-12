import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import coschedula_monitor 1.0

Window {
    id: window
    width: 1040
    height: 480
    visible: true
    title: qsTr("Hello World")

    required property Monitor monitor

    Item {
        anchors.fill: parent

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            property real scaleX: window.width / monitor.totalEndTime
            property real transX: 0

            onWheel: {
                const scaleDivision = Math.pow(1.1, wheel.angleDelta.y / 120)
                const scaleAndTrans = monitor.scaleAndTrans(
                                        mouseArea.transX,
                                        mouseArea.scaleX,
                                        scaleDivision,
                                        wheel.x);

                mouseArea.transX = scaleAndTrans.x
                mouseArea.scaleX = scaleAndTrans.y
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                Slider {
                    id: durationSlider

                    Layout.fillWidth: true
                    from: 0
                    value: 0
                    to: 1
                }
                Slider {
                    id: xSlider

                    Layout.fillWidth: true
                    from: 0
                    value: 0
                    to: monitor.totalEndTime * durationSlider.value
                }

                //Flickable {
                //    id: flickable

                //    boundsMovement: Flickable.StopAtBounds
                //    Layout.fillWidth: true
                //    implicitHeight: layout.implicitHeight
                //    contentWidth: layout.implicitWidth
                //    contentHeight: layout.implicitHeight
                //    contentX: flickable.contentWidth - xSlider.value
                //    interactive: true
                Item {
                    ColumnLayout {
                        id: layout

                        x: mouseArea.transX

                        Repeater {
                            id: repeater

                            model: window.monitor.tasks

                            Rectangle {
                                id: taskDelegate
                                readonly property Task task: modelData

                                implicitWidth: taskLayout.implicitWidth + taskLayout.anchors.leftMargin + taskLayout.anchors.rightMargin
                                implicitHeight: taskLayout.implicitHeight + taskLayout.anchors.topMargin + taskLayout.anchors.bottomMargin

                                border.width: 1
                                border.color: "#88000000";
                                radius: 2

                                ColumnLayout {
                                    id: taskLayout

                                    anchors.fill: parent
                                    anchors.margins: 1
                                    spacing: 0

                                    Text {
                                        Layout.leftMargin: mouseArea.transX < 0 ? -mouseArea.transX : 0
                                        text: taskDelegate.task.location
                                    }
                                    Item {
                                        id: container

                                        readonly property real xscale: mouseArea.scaleX

                                        implicitWidth: monitor.totalEndTime * container.xscale
                                        implicitHeight: 25
                                        Repeater {
                                            model: taskDelegate.task.log
                                            Rectangle {
                                                id: logDelegate

                                                readonly property LogItem item: modelData

                                                anchors.top: parent.top
                                                anchors.bottom: parent.bottom
                                                x: logDelegate.item.startTime * container.xscale
                                                implicitWidth: logDelegate.item.state === LogItem.Finished
                                                                 ? taskLayout.width - logDelegate.x
                                                                 : (logDelegate.item.endTime - logDelegate.item.startTime) * container.xscale
                                                implicitHeight: logText.implicitHeight
                                                clip: true
                                                color: {
                                                    switch(logDelegate.item.state) {
                                                    case LogItem.Started: return '#ffaaaaaa'
                                                    case LogItem.Suspended: return '#ffffff00'
                                                    case LogItem.Resumed: return '#ff00ff00'
                                                    case LogItem.Finished: return '#ff444444'
                                                    }
                                                }

                                                //Image {
                                                //    anchors.top: parent.top
                                                //    anchors.bottom: parent.bottom
                                                //    anchors.right: parent.right
                                                //    width: height

                                                //    visible: logDelegate.item.state === LogItem.Finished
                                                //    source: 'qtlogo.png'
                                                //    sourceSize.width: 100
                                                //    sourceSize.height: 100
                                                //}

                                                Text {
                                                    id: logText

                                                    function formatTime(ns) {
                                                        if(ns < 1000) {
                                                            return `${ns.toFixed(0)} nanos`
                                                        } else if(ns < 1000 * 1000) {
                                                            return `${(ns / 1000).toFixed(0)} micros`
                                                        } else if(ns < 1000 * 1000 * 1000) {
                                                            return `${(ns / 1000 / 1000).toFixed(0)} millis`
                                                        } else {
                                                            return `${(ns / 1000 / 1000 / 1000).toFixed(0)} secs`
                                                        }
                                                    }

                                                    color: logDelegate.item.state === LogItem.Finished ? '#ffffff' : '#000000'

                                                    text: (() => {
                                                               switch(logDelegate.item.state) {
                                                                   case LogItem.Started: return 'started'
                                                                   case LogItem.Suspended: return 'suspended'
                                                                   case LogItem.Resumed: return 'resumed'
                                                                   case LogItem.Finished: return 'finished'
                                                               }
                                                           })()
                                                          + ` ${logText.formatTime(logDelegate.item.endTime - logDelegate.item.startTime)}`
                                                          + (logDelegate.item.state === LogItem.Finished
                                                             ? ` (total: ${logText.formatTime(taskDelegate.task.endTime - taskDelegate.task.startTime)}`
                                                               + `, work time: ${logText.formatTime(taskDelegate.task.workTime)})`
                                                             : '')
                                                }

                                                Text {
                                                    anchors.bottom: parent.bottom
                                                    anchors.left: parent.left
                                                    anchors.leftMargin: 12
                                                    text: logDelegate.item.startTime / 1000
                                                    font.pointSize: 6
                                                }
                                                Text {
                                                    anchors.bottom: parent.bottom
                                                    anchors.right: parent.right
                                                    anchors.rightMargin: 12
                                                    text: logDelegate.item.endTime / 1000
                                                    font.pointSize: 6
                                                }


                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //Text {
    //    anchors.centerIn: parent
    //    font.pointSize: 40
    //    text: `${window.monitor} : ${repeater.count}`
    //}
}

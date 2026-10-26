import QtQuick

// The voiceovers laid over the edit, in a lane under the clips, on the same
// scale as the edit bar above it. Drag one to move it along the edit, click
// it to go to its start, and hover it to remove it (a short take shows its ×
// beside it). While recording, the take so far grows from where it began to
// the playhead.
Item {
    id: root
    implicitHeight: 26
    clip: true

    // The EditBar whose stretch of the sequence this lane shows.
    required property Item bar
    readonly property var timeline: backend.timeline
    readonly property var voiceovers: timeline.voiceovers
    property color accent: "#FFD60A"
    property color accentForeground: "black"
    // Where the take being recorded began, or -1 when not recording.
    property real recordingFrom: -1
    readonly property real frameRadius: 6

    signal seek(real seconds)

    // A dragged voiceover's start or end catches on clip edges and on the
    // playhead, so it lines up with a cut.
    function snap(at, length) {
        var best = at, bestDistance = 8
        function consider(edge, offset) {
            var d = Math.abs(bar.xForTime(edge) - bar.xForTime(at + offset))
            if (d < bestDistance) { best = edge - offset; bestDistance = d }
        }
        var clips = timeline.clips
        for (var i = 0; i < clips.length; ++i) {
            consider(clips[i].start, 0); consider(clips[i].end, 0)
            consider(clips[i].start, length); consider(clips[i].end, length)
        }
        consider(bar.playheadSec, 0); consider(bar.playheadSec, length)
        return best
    }

    Rectangle {
        anchors.fill: parent
        radius: root.frameRadius; color: "#1c1c1e"
    }

    Repeater {
        model: root.voiceovers.length
        Rectangle {
            id: block
            required property int index
            // Briefly past the end while the voiceover count changes.
            readonly property var voiceover: root.voiceovers[index] || { at: 0, end: 0, length: 0 }
            readonly property bool hovered: area.containsMouse || removeArea.containsMouse
            // Wide takes hold their × inside; narrow ones get it alongside,
            // on whichever side the lane has room.
            readonly property bool roomy: width > 50
            x: Math.round(root.bar.xForTime(voiceover.at))
            width: Math.max(6, Math.round(root.bar.xForTime(voiceover.end)) - x)
            height: root.height
            radius: root.frameRadius
            color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, area.pressed ? 0.45 : 0.28)
            border.color: root.accent; border.width: 2
            z: area.pressed || hovered ? 1 : 0

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left; anchors.leftMargin: 8
                anchors.right: parent.right; anchors.rightMargin: block.hovered ? 28 : 8
                visible: block.width > 60
                text: "Voiceover"
                color: "white"; font.pixelSize: 11; font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                property real pressX: 0
                property real pressAt: 0
                property bool moved: false
                cursorShape: pressed && moved ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                // Measured on the lane, which holds still while the block moves.
                function laneX(mouse) { return mapToItem(root, mouse.x, 0).x }

                onPressed: mouse => {
                    pressX = laneX(mouse)
                    pressAt = block.voiceover.at
                    moved = false
                    root.timeline.beginGesture()
                }
                onPositionChanged: mouse => {
                    if (!pressed) return
                    var x = laneX(mouse)
                    if (!moved && Math.abs(x - pressX) <= 4) return
                    moved = true
                    var at = pressAt + (x - pressX) / Math.max(root.bar.pxPerSec, 0.001)
                    root.timeline.moveVoiceover(block.index, root.snap(at, block.voiceover.length))
                }
                onReleased: {
                    root.timeline.endGesture()
                    if (!moved) root.seek(block.voiceover.at)
                }
                onCanceled: root.timeline.endGesture()
            }

            Rectangle {
                id: remove
                objectName: "removeVoiceover"
                visible: block.hovered && !area.pressed
                readonly property bool fitsRight: block.x + block.width + width <= root.width
                x: block.roomy ? block.width - width - 4
                   : fitsRight ? block.width : -width
                anchors.verticalCenter: parent.verticalCenter
                width: 18; height: 18; radius: 9
                color: removeArea.containsMouse ? root.accent : "#e0202023"
                Text {
                    anchors.centerIn: parent; anchors.verticalCenterOffset: -1
                    text: "×"; font.pixelSize: 14; font.weight: Font.DemiBold
                    color: removeArea.containsMouse ? root.accentForeground : "white"
                }
                MouseArea {
                    id: removeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.timeline.removeVoiceover(block.index)
                }
            }
        }
    }

    // The take being recorded.
    Rectangle {
        visible: root.recordingFrom >= 0
        x: Math.round(root.bar.xForTime(root.recordingFrom))
        width: Math.max(6, Math.round(root.bar.xForTime(root.bar.playheadSec)) - x)
        height: root.height
        radius: root.frameRadius
        color: "#59ff453a"
        border.color: "#ff453a"; border.width: 2
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left; anchors.leftMargin: 8
            visible: parent.width > 80
            text: "Recording"
            color: "white"; font.pixelSize: 11; font.weight: Font.DemiBold
        }
    }
}

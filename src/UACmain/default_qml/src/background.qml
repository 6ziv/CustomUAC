import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Particles
Image {
	source: "image://screenshot/0.5"
	anchors.fill: parent
	ParticleSystem {
        anchors.fill: parent
        ImageParticle {
            anchors.fill: parent
            source: "qrc:///particleresources/star.png"
            color:"#B0E0E6"
            colorVariation:0.4
        }

        Emitter {
			x:mouse_area.mouseX
			y:mouse_area.mouseY
            id:emitter
            emitRate: mouse_area.containsMouse?500:0
            lifeSpan: 500
            size: 50
            sizeVariation: 8
            width: 20
            height: 20
        }
    }
    MouseArea{
		id:mouse_area
        hoverEnabled: true
        anchors.fill: parent
    }
}



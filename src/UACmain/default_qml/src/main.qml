import QtQuick 
import QtQuick.Window
import QtQuick.Controls
import QtQuick.LocalStorage
Window {
	property int prevX: 0
	property int prevY: 0
	
	id:uac_window
    width: 640
	height:300
    visible: true
	flags: Qt.Window | Qt.FramelessWindowHint
	color: "transparent"
	
	Component.onCompleted:()=>{
		uac_window.requestActivate();
		uac_window.raise();
	}
	Item{
		x:0
		y:0
		width:childrenRect.width
		height:childrenRect.height
		Rectangle{
			x: 0
			y: 0
			width:80
			height:20
			color:Qt.rgba(0,0,0,0.9)
			Text {
				anchors.centerIn:parent
				text: qsTr("UAC")
				color:"white"
			}
			MouseArea{
				acceptedButtons: Qt.LeftButton | Qt.RightButton
				anchors.fill: parent
				onPressed: (mouse)=>{
					uac_window.prevX=mouse.x;
					uac_window.prevY=mouse.y;
					if (mouse.button == Qt.RightButton){
						contextMenu.popup()
					}
				}
				onPositionChanged: (mouse)=>{
					var deltaX = mouse.x - uac_window.prevX;
					uac_window.x += deltaX;
					prevX = mouse.x - deltaX;

					var deltaY = mouse.y - uac_window.prevY
					uac_window.y += deltaY;
					prevY = mouse.y - deltaY;
				}
			}
		}
		Rectangle{
			x:0
			y:25
			width:600
			height:childrenRect.height + 20
			color:Qt.rgba(0,0,0,0.9)
		
			MouseArea{
				acceptedButtons: Qt.LeftButton | Qt.RightButton
				anchors.fill: parent
				onPressed: (mouse)=>{
					uac_window.prevX=mouse.x;
					uac_window.prevY=mouse.y;
					if (mouse.button == Qt.RightButton){
						contextMenu.popup()
					}
				}
				onPositionChanged: (mouse)=>{
					var deltaX = mouse.x - uac_window.prevX;
					uac_window.x += deltaX;
					prevX = mouse.x - deltaX;

					var deltaY = mouse.y - uac_window.prevY
					uac_window.y += deltaY;
					prevY = mouse.y - deltaY;
				}
			}
		
			Text {
				id:main_text_content
				width:parent.width - 20
				height:contentHeight
				anchors.left:parent.left
				anchors.top:parent.top
				anchors.margins:10
				text:
					uac.type==2?
					qsTr("Are you sure to run installer %1 %2, whose version is %3?")
						.arg((uac.file_description.length==0)?uac.description:uac.file_description)
						.arg((uac.signer.length==0)?"":(qsTr("signed by %1").arg(uac.signer)))
						.arg(uac.MSIVersion)
					:
					qsTr("Are you sure to %1 %2 %3 %4, which is located at %5?")
						.arg((uac.type==1)?qsTr("load"):qsTr("run"))
						.arg((uac.type==1)?qsTr("dynamic library"):qsTr("application"))
						.arg((uac.file_description.length==0)?uac.description:uac.file_description)
						.arg((uac.signer.length==0)?"":(qsTr("signed by %1").arg(uac.signer)))
						.arg(uac.path)
				color:"white"
				wrapMode:Text.Wrap
				font.pointSize:10
			}
			Text{
				visible:(!uac.isadmin)
				id:"label_username"
				width:(uac.isadmin?0:50)
				height:20
				anchors.left:parent.left
				anchors.top:main_text_content.bottom
				anchors.margins:(uac.isadmin?0:15)
				font.pointSize:10
				color:"white"
				text:(uac.administrators.length>1)?qsTr("Username:"):(qsTr("Username:")+uac.administrators[0])
				ComboBox {
					id:select_user
					x:60
					width:200
					anchors.top:parent.top
					anchors.bottom:parent.bottom
					model: uac.administrators
					visible:(uac.administrators.length>1)
				}
			}
			Text{
				visible:(!uac.isadmin)
				id:label_password
				height:(uac.isadmin?0:20)
				anchors.left:parent.left
				anchors.right:parent.right
				anchors.top:label_username.bottom
				anchors.margins:(uac.isadmin?0:15)
				font.pointSize:10
				color:"white"
				text:qsTr("Password:")
				TextField {
					x:40
					width:300
					anchors.top:parent.top
					anchors.bottom:parent.bottom
					anchors.margins:0
					id: password_field
					placeholderText: qsTr("Input password")
					echoMode: TextInput.Password
				}
			}
			Text{
				x:200
				width:25
				height:20
				anchors.top:label_password.bottom
				anchors.margins:10
				text:qsTr("Yes")
				color:"white"
				font.pointSize:12.5
				MouseArea{
					anchors.fill:parent
					hoverEnabled: true
					onEntered: parent.font.weight=Font.Bold;
					onExited:  parent.font.weight=Font.Normal;
					onClicked: {
						if(uac.isadmin)
							uac.login();
						else
							uac.login(select_user.currentText,password_field.text)
					}
				}
			}
			Text{
				x:350
				width:25
				height:20
				anchors.top:label_password.bottom
				anchors.margins:10
				text:qsTr("No")
				color:"white"
				font.pointSize:12.5
				MouseArea{
					anchors.fill:parent
					hoverEnabled: true
					onEntered: parent.font.weight=Font.Bold;
					onExited:  parent.font.weight=Font.Normal;
					onClicked: uac.reject();
				}
			}
		}
	}
	Menu {
        id: contextMenu
		Action {
			enabled:true
            text: qsTr("Add to allow list")
            onTriggered: uac.allowlist()
        }
    }
}


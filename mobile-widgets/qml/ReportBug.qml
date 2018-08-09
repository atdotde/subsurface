// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.2 as Kirigami
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2

Kirigami.Page {
	id: reportBug
	title: qsTr("Report a Bug")
//	leftPadding: 0
//	topPadding: 0
//	rightPadding: 0
//	bottomPadding: 0
	property bool firstRun: true

	ColumnLayout {
		width: gridWidth

		Controls.Label {
			//		Layout.alignment: Qt.AlignRight
			text: qsTr("Bug description:")
			font.pointSize: subsurfaceTheme.smallPointSize
		}
		Controls.TextField {
			id: txtBug;
//			textFormat: TextEdit.RichText
			focus: true
			Layout.fillWidth: true
			Layout.fillHeight: true
			Layout.minimumHeight: Kirigami.Units.gridUnit * 6
			selectByMouse: true
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			onEditingFinished: {
				focus = false
			}
		}
		Controls.Label {
			text: qsTr("Email address:")
			font.pointSize: subsurfaceTheme.smallPointSize
		}
		Controls.TextField {
			id: txtEmail;
			//		Layout.fillWidth: true
			onEditingFinished: {
				focus = false
			}
		}
		Controls.Label {
			text: qsTr("Include runtime and libdivecomputer information (recommended):")
			font.pointSize: subsurfaceTheme.smallPointSize
		}
		Controls.CheckBox {
			id: includeLog;
			checked: true
			//		Layout.fillWidth: true
		}
		SsrfButton {
			id: sendBugReport
			//		Layout.alignment: Qt.AlignHCenter
			text: qsTr("Send Bug Report")
			onClicked: {
				manager.sendBugReport(txtBug.text, txtEmail.text, includeLog.checked);
				rootItem.returnTopPage()
			}
		}

	}
}

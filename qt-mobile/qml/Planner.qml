import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

MobileComponents.Page {
	id: page
	objectName: "Planner"
	color: MobileComponents.Theme.viewBackgroundColor

	StartPage {
		id: startPage
		anchors.fill: parent
		opacity: (planner.count == 0) ? 1.0 : 0
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: MobileComponents.Units.shortDuration } }
	}
	Text {
		id: diveplan
		textFormat: Text.RichText
		text: plan.plan
	}

}

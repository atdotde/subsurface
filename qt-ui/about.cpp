#include "about.h"
#include "ssrf-version.h"
#include "mainwindow.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QNetworkReply>
#include <qdesktopservices.h>

SubsurfaceAbout *SubsurfaceAbout::instance()
{
	static SubsurfaceAbout *self = new SubsurfaceAbout(mainWindow());
	self->setAttribute(Qt::WA_QuitOnClose, false);
	return self;
}

SubsurfaceAbout::SubsurfaceAbout(QWidget* parent, Qt::WindowFlags f)
{
	ui.setupUi(this);
	ui.aboutLabel->setText(tr("<span style='font-size: 18pt; font-weight: bold;'>" \
		"Subsurface %1 </span><br><br>" \
		"Multi-platform divelog software<br>" \
		"<span style='font-size: 8pt'>Linus Torvalds, Dirk Hohndel, and others, 2011-2014</span>").arg(VERSION_STRING));
	licenseButton = new QPushButton(tr("&License"));
	websiteButton = new QPushButton(tr("&Website"));
	ui.buttonBox->addButton(licenseButton, QDialogButtonBox::ActionRole);
	ui.buttonBox->addButton(websiteButton, QDialogButtonBox::ActionRole);
	connect(licenseButton, SIGNAL(clicked(bool)), this, SLOT(licenseClicked()));
	connect(websiteButton, SIGNAL(clicked(bool)), this, SLOT(websiteClicked()));
}

void SubsurfaceAbout::licenseClicked(void)
{
	QDesktopServices::openUrl(QUrl("http://www.gnu.org/licenses/gpl-2.0.txt"));
}

void SubsurfaceAbout::websiteClicked(void)
{
	QDesktopServices::openUrl(QUrl("http://subsurface.hohndel.org"));
}

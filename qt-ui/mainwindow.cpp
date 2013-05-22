/*
 * mainwindow.cpp
 *
 * classes for the main UI window in Subsurface
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QDateTime>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QFontMetrics>

#include "divelistview.h"
#include "starwidget.h"

#include "glib.h"
#include "../dive.h"
#include "../divelist.h"
#include "../pref.h"
#include "modeldelegates.h"
#include "models.h"
#include "downloadfromdivecomputer.h"

static MainWindow* instance = 0;

MainWindow* mainWindow()
{
	return instance;
}

MainWindow::MainWindow() : ui(new Ui::MainWindow())
{
	ui->setupUi(this);
	setWindowIcon(QIcon(":subsurface-icon"));
	connect(ui->ListWidget, SIGNAL(currentDiveChanged(int)), this, SLOT(current_dive_changed(int)));
	ui->globeMessage->hide();
	ui->mainErrorMessage->hide();
	ui->globe->setMessageWidget(ui->globeMessage);
	ui->globeMessage->setCloseButtonVisible(false);
	ui->ProfileWidget->setFocusProxy(ui->ListWidget);
	ui->ListWidget->reload();
	readSettings();
	ui->ListWidget->setFocus();
	ui->globe->reload();
	instance = this;
}

void MainWindow::current_dive_changed(int divenr)
{
	select_dive(divenr);
	ui->globe->centerOn(get_dive(selected_dive));
	redrawProfile();
	ui->InfoWidget->updateDiveInfo(divenr);
}

void MainWindow::redrawProfile()
{
	ui->ProfileWidget->plot(get_dive(selected_dive));
}

void MainWindow::on_actionNew_triggered()
{
	qDebug("actionNew");
}

void MainWindow::on_actionOpen_triggered()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(), filter());
	if (filename.isEmpty())
		return;

	// Needed to convert to char*
	QByteArray fileNamePtr = filename.toLocal8Bit();

	on_actionClose_triggered();

	char *error = NULL;
	parse_file(fileNamePtr.data(), &error);
	set_filename(fileNamePtr.data(), TRUE);

	if (error != NULL) {
		showError(error);
		free(error);
	}
	process_dives(FALSE, FALSE);

	ui->InfoWidget->reload();
	ui->globe->reload();
	ui->ListWidget->reload();
	ui->ListWidget->setFocus();
}

void MainWindow::on_actionSave_triggered()
{
	file_save();
}

void MainWindow::on_actionSaveAs_triggered()
{
	file_save_as();
}
void MainWindow::on_actionClose_triggered()
{
	if (unsaved_changes() && (askSaveChanges() == FALSE))
		return;

	/* free the dives and trips */
	while (dive_table.nr)
		delete_single_dive(0);

	/* clear the selection and the statistics */
	selected_dive = -1;

	//WARNING: Port this to Qt.
	//process_selected_dives();

	ui->InfoWidget->clearStats();
	ui->InfoWidget->clearInfo();
	ui->InfoWidget->clearEquipment();
	ui->InfoWidget->updateDiveInfo(-1);
	ui->ProfileWidget->clear();
	ui->ListWidget->reload();
	ui->globe->reload();

	clear_events();
}

void MainWindow::on_actionImport_triggered()
{
	qDebug("actionImport");
}

void MainWindow::on_actionExportUDDF_triggered()
{
	qDebug("actionExportUDDF");
}

void MainWindow::on_actionPrint_triggered()
{
	qDebug("actionPrint");
}

void MainWindow::on_actionPreferences_triggered()
{
	qDebug("actionPreferences");
}

void MainWindow::on_actionQuit_triggered()
{
	if (unsaved_changes() && (askSaveChanges() == FALSE))
		return;
	writeSettings();
	QApplication::quit();
}

void MainWindow::on_actionDownloadDC_triggered()
{
	DownloadFromDCWidget* downloadWidget = new DownloadFromDCWidget();
	downloadWidget->show();
}

void MainWindow::on_actionDownloadWeb_triggered()
{
	qDebug("actionDownloadWeb");}

void MainWindow::on_actionEditDeviceNames_triggered()
{
	qDebug("actionEditDeviceNames");}

void MainWindow::on_actionAddDive_triggered()
{
	qDebug("actionAddDive");
}

void MainWindow::on_actionRenumber_triggered()
{
	qDebug("actionRenumber");
}

void MainWindow::on_actionAutoGroup_triggered()
{
	qDebug("actionAutoGroup");
}

void MainWindow::on_actionToggleZoom_triggered()
{
	qDebug("actionToggleZoom");
}

void MainWindow::on_actionYearlyStatistics_triggered()
{
	qDebug("actionYearlyStatistics");
}

void MainWindow::on_actionViewList_triggered()
{
	ui->InfoWidget->setVisible(false);
	ui->ListWidget->setVisible(true);
	ui->ProfileWidget->setVisible(false);
}

void MainWindow::on_actionViewProfile_triggered()
{
	ui->InfoWidget->setVisible(false);
	ui->ListWidget->setVisible(false);
	ui->ProfileWidget->setVisible(true);
}

void MainWindow::on_actionViewInfo_triggered()
{
	ui->InfoWidget->setVisible(true);
	ui->ListWidget->setVisible(false);
	ui->ProfileWidget->setVisible(false);
}

void MainWindow::on_actionViewAll_triggered()
{
	ui->InfoWidget->setVisible(true);
	ui->ListWidget->setVisible(true);
	ui->ProfileWidget->setVisible(true);
}

void MainWindow::on_actionPreviousDC_triggered()
{
	dc_number--;
	redrawProfile();
}

void MainWindow::on_actionNextDC_triggered()
{
	dc_number++;
	redrawProfile();
}

void MainWindow::on_actionSelectEvents_triggered()
{
	qDebug("actionSelectEvents");
}

void MainWindow::on_actionInputPlan_triggered()
{
	qDebug("actionInputPlan");
}

void MainWindow::on_actionAboutSubsurface_triggered()
{
	qDebug("actionAboutSubsurface");
}

void MainWindow::on_actionUserManual_triggered()
{
	qDebug("actionUserManual");
}

QString MainWindow::filter()
{
	QString f;
	f += "ALL ( *.xml *.XML *.uddf *.udcf *.UDFC *.jlb *.JLB ";
#ifdef LIBZIP
	f += "*.sde *.SDE *.dld *.DLD ";
#endif
#ifdef SQLITE3
	f += "*.db";
#endif
	f += ");;";

	f += "XML (*.xml *.XML);;";
	f += "UDDF (*.uddf);;";
	f += "UDCF (*.udcf *.UDCF);;";
	f += "JLB  (*.jlb *.JLB);;";

#ifdef LIBZIP
	f += "SDE (*.sde *.SDE);;";
	f += "DLD (*.dld *.DLD);;";
#endif
#ifdef SQLITE3
	f += "DB (*.db)";
#endif

	return f;
}

bool MainWindow::askSaveChanges()
{
	QString message;
	QMessageBox::StandardButton response;

	if (existing_filename)
		message = tr("You have unsaved changes to file: %1\nDo you really want to close the file without saving?").arg(existing_filename);
	else
		message = tr("You have unsaved changes\nDo you really want to close the datafile without saving?");

	response = QMessageBox::question(this, tr("Save Changes?"), message,
					QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Save);
	if (response == QMessageBox::Save) {
		file_save();
		return true;
	} else if (response == QMessageBox::Ok) {
		return true;
	}
	return false;
}

#define GET_UNIT(v, name, field, f, t)				\
	v = settings.value(QString(name));			\
	if (v.isValid())					\
		prefs.units.field = (v.toInt() == (t)) ? (t) : (f)

#define GET_BOOL(v, name, field)				\
	v = settings.value(QString(name));			\
	if (v.isValid() && v.toInt())				\
		field = TRUE;					\
	else							\
		field = FALSE

void MainWindow::readSettings()
{
	int i;
	QVariant v;
	QSettings settings;

	settings.beginGroup("MainWindow");
	QSize sz = settings.value("size").value<QSize>();
	resize(sz);
	ui->mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
	ui->infoProfileSplitter->restoreState(settings.value("infoProfileSplitter").toByteArray());
	settings.endGroup();

	settings.beginGroup("ListWidget");
	/* if no width are set, use the calculated width for each column;
	 * for that to work we need to temporarily expand all rows */
	ui->ListWidget->expandAll();
	for (i = TreeItemDT::NR; i < TreeItemDT::COLUMNS; i++) {
		QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			ui->ListWidget->setColumnWidth(i, width.toInt());
		else
			ui->ListWidget->resizeColumnToContents(i);
	}
	ui->ListWidget->collapseAll();
	ui->ListWidget->expand(ui->ListWidget->model()->index(0,0));
	ui->ListWidget->scrollTo(ui->ListWidget->model()->index(0,0), QAbstractItemView::PositionAtCenter);

	settings.endGroup();
	settings.beginGroup("Units");
	GET_UNIT(v, "feet", length, units::METERS, units::FEET);
	GET_UNIT(v, "psi", pressure, units::BAR, units::PSI);
	GET_UNIT(v, "cuft", volume, units::LITER, units::CUFT);
	GET_UNIT(v, "fahrenheit", temperature, units::CELSIUS, units::FAHRENHEIT);
	GET_UNIT(v, "lbs", weight, units::KG, units::LBS);
	settings.endGroup();
	settings.beginGroup("DisplayListColumns");
	GET_BOOL(v, "CYLINDER", prefs.visible_cols.cylinder);
	GET_BOOL(v, "TEMPERATURE", prefs.visible_cols.temperature);
	GET_BOOL(v, "TOTALWEIGHT", prefs.visible_cols.totalweight);
	GET_BOOL(v, "SUIT", prefs.visible_cols.suit);
	GET_BOOL(v, "NITROX", prefs.visible_cols.nitrox);
	GET_BOOL(v, "OTU", prefs.visible_cols.otu);
	GET_BOOL(v, "MAXCNS", prefs.visible_cols.maxcns);
	GET_BOOL(v, "SAC", prefs.visible_cols.sac);
	GET_BOOL(v, "po2graph", prefs.pp_graphs.po2);
	GET_BOOL(v, "pn2graph", prefs.pp_graphs.pn2);
	GET_BOOL(v, "phegraph", prefs.pp_graphs.phe);
	settings.endGroup();
	settings.beginGroup("TecDetails");
	v = settings.value(QString("po2threshold"));
	if (v.isValid())
		prefs.pp_graphs.po2_threshold = v.toDouble();
	v = settings.value(QString("pn2threshold"));
	if (v.isValid())
		prefs.pp_graphs.pn2_threshold = v.toDouble();
	v = settings.value(QString("phethreshold"));
	if (v.isValid())
		prefs.pp_graphs.phe_threshold = v.toDouble();
	GET_BOOL(v, "mod", prefs.mod);
	v = settings.value(QString("modppO2"));
	if (v.isValid())
		prefs.mod_ppO2 = v.toDouble();
	GET_BOOL(v, "ead", prefs.ead);
	GET_BOOL(v, "redceiling", prefs.profile_red_ceiling);
	GET_BOOL(v, "calcceiling", prefs.profile_calc_ceiling);
	GET_BOOL(v, "calcceiling3m", prefs.calc_ceiling_3m_incr);
	v = settings.value(QString("gflow"));
	if (v.isValid())
		prefs.gflow = v.toInt() / 100.0;
	v = settings.value(QString("gfhigh"));
	if (v.isValid())
		prefs.gfhigh = v.toInt() / 100.0;
	set_gf(prefs.gflow, prefs.gfhigh);
	settings.endGroup();

#if ONCE_WE_CAN_SET_FONTS
	settings.beginGroup("Display");
	v = settings.value(QString("divelist_font"));
	if (v.isValid())
		/* I don't think this is right */
		prefs.divelist_font = strdup(v.toString);
#endif

#if ONCE_WE_HAVE_MAPS
	v = settings.value(QString_int("map_provider"));
	if(v.isValid())
		prefs.map_provider = v.toInt();
#endif
}

#define SAVE_VALUE(name, field)				\
	if (prefs.field != default_prefs.field)		\
		settings.setValue(name, prefs.field)

void MainWindow::writeSettings()
{
	int i;
	QSettings settings;

	settings.beginGroup("MainWindow");
	settings.setValue("size",size());
	settings.setValue("mainSplitter", ui->mainSplitter->saveState());
	settings.setValue("infoProfileSplitter", ui->infoProfileSplitter->saveState());
	settings.endGroup();

	settings.beginGroup("ListWidget");
	for (i = TreeItemDT::NR; i < TreeItemDT::COLUMNS; i++)
		settings.setValue(QString("colwidth%1").arg(i), ui->ListWidget->columnWidth(i));
	settings.endGroup();
	settings.beginGroup("Units");
	SAVE_VALUE("feet", units.length);
	SAVE_VALUE("psi", units.pressure);
	SAVE_VALUE("cuft", units.volume);
	SAVE_VALUE("fahrenheit", units.temperature);
	SAVE_VALUE("lbs", units.weight);
	settings.endGroup();
	settings.beginGroup("DisplayListColumns");
	SAVE_VALUE("TEMPERATURE", visible_cols.temperature);
	SAVE_VALUE("TOTALWEIGHT", visible_cols.totalweight);
	SAVE_VALUE("SUIT", visible_cols.suit);
	SAVE_VALUE("CYLINDER", visible_cols.cylinder);
	SAVE_VALUE("NITROX", visible_cols.nitrox);
	SAVE_VALUE("SAC", visible_cols.sac);
	SAVE_VALUE("OTU", visible_cols.otu);
	SAVE_VALUE("MAXCNS", visible_cols.maxcns);
	settings.endGroup();
	settings.beginGroup("TecDetails");
	SAVE_VALUE("po2graph", pp_graphs.po2);
	SAVE_VALUE("pn2graph", pp_graphs.pn2);
	SAVE_VALUE("phegraph", pp_graphs.phe);
	SAVE_VALUE("po2threshold", pp_graphs.po2_threshold);
	SAVE_VALUE("pn2threshold", pp_graphs.pn2_threshold);
	SAVE_VALUE("phethreshold", pp_graphs.phe_threshold);
	SAVE_VALUE("mod", mod);
	SAVE_VALUE("modppO2", mod_ppO2);
	SAVE_VALUE("ead", ead);
	SAVE_VALUE("redceiling", profile_red_ceiling);
	SAVE_VALUE("calcceiling", profile_calc_ceiling);
	SAVE_VALUE("calcceiling3m", calc_ceiling_3m_incr);
	SAVE_VALUE("gflow", gflow);
	SAVE_VALUE("gfhigh", gfhigh);
	settings.endGroup();
	settings.beginGroup("GeneralSettings");
	SAVE_VALUE("default_filename", default_filename);
	settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (unsaved_changes() && (askSaveChanges() == FALSE)) {
		event->ignore();
		return;
	}
	event->accept();
	writeSettings();
}

DiveListView* MainWindow::dive_list()
{
	return ui->ListWidget;
}

GlobeGPS* MainWindow::globe()
{
	return ui->globe;
}

ProfileGraphicsView* MainWindow::graphics()
{
	return ui->ProfileWidget;
}

MainTab* MainWindow::information()
{
	return ui->InfoWidget;
}

void MainWindow::file_save_as(void)
{
	QString filename;
	const char *default_filename;

	if (existing_filename)
		default_filename = existing_filename;
	else
		default_filename = prefs.default_filename;
	filename = QFileDialog::getSaveFileName(this, tr("Save File as"), default_filename,
						tr("Subsurface XML files (*.ssrf *.xml *.XML)"));
	if (!filename.isNull() && !filename.isEmpty()) {
		save_dives(filename.toUtf8().data());
		set_filename(filename.toUtf8().data(), TRUE);
		mark_divelist_changed(FALSE);
	}
}

void MainWindow::file_save(void)
{
	const char *current_default;

	if (!existing_filename)
		return file_save_as();

	current_default = prefs.default_filename;
	if (strcmp(existing_filename, current_default) ==  0) {
		/* if we are using the default filename the directory
		 * that we are creating the file in may not exist */
		QDir current_def_dir = QFileInfo(current_default).absoluteDir();
		if (!current_def_dir.exists())
			current_def_dir.mkpath(current_def_dir.absolutePath());
	}
	save_dives(existing_filename);
	mark_divelist_changed(FALSE);
}

void MainWindow::showError(QString message)
{
	if (message.isEmpty())
		return;
	ui->mainErrorMessage->setText(message);
	ui->mainErrorMessage->setCloseButtonVisible(true);
	ui->mainErrorMessage->setMessageType(KMessageWidget::Error);
	ui->mainErrorMessage->animatedShow();
}

#include <QtDebug>
#include <QFileDialog>
#include "divelogimportdialog.h"
#include "mainwindow.h"
#include "ui_divelogimportdialog.h"

const DiveLogImportDialog::CSVAppConfig DiveLogImportDialog::CSVApps[CSVAPPS] = {
		{"", },
		{"APD Log Viewer", 1, 2, 16, 7, 18, 19, "Tab"},
		{"XP5", 1, 2, 10, -1, -1, -1, "Tab"},
		{NULL,}
};

DiveLogImportDialog::DiveLogImportDialog(QWidget *parent) :
	QDialog(parent),
	selector(true),
	ui(new Ui::DiveLogImportDialog)
{
	ui->setupUi(this);

	for (int i = 0; !CSVApps[i].name.isNull(); ++i)
		ui->knownImports->addItem(CSVApps[i].name);

	ui->CSVSeparator->addItem("Tab");
	ui->CSVSeparator->addItem(",");
	ui->knownImports->setCurrentIndex(1);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(ui->CSVDepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->CSVTime, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->CSVTemperature, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->temperatureCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
	connect(ui->CSVpo2, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->po2CheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
	connect(ui->CSVcns, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->cnsCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
	connect(ui->CSVstopdepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->stopdepthCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
}

DiveLogImportDialog::~DiveLogImportDialog()
{
	delete ui;
}

#define VALUE_IF_CHECKED(x) (ui->x->isEnabled() ? ui->x->value() - 1: -1)
void DiveLogImportDialog::on_buttonBox_accepted()
{
	char *error = NULL;

	if (ui->tabWidget->currentIndex() == 0) {
		QStringList fileNames = ui->DiveLogFile->text().split(";");

		/*
	if (ui->ImportAdvanced->isChecked()) {
		for (int i = 0; i < fileNames.size(); ++i) {
			parse_xml_file_units(fileNames.at(i).toUtf8().data(),
					     ui->XMLImportFormat->currentIndex(), ui->XMLImportUnits->currentIndex(),
					     &error);
		}
		if (error != NULL) {

			mainWindow()->showError(error);
			free(error);
			error = NULL;
		}
	} else {
	*/
		mainWindow()->importFiles(fileNames);
		return;
		//}

	} else {
		parse_csv_file(ui->CSVFile->text().toUtf8().data(), ui->CSVTime->value() - 1,
				ui->CSVDepth->value() - 1, VALUE_IF_CHECKED(CSVTemperature),
				VALUE_IF_CHECKED(CSVpo2),
				VALUE_IF_CHECKED(CSVcns),
				VALUE_IF_CHECKED(CSVstopdepth),
				ui->CSVSeparator->currentIndex(),
				&error);
		if (error != NULL) {
			mainWindow()->showError(error);
			free(error);
			error = NULL;
		}
	}
	process_dives(TRUE, FALSE);

	mainWindow()->refreshDisplay();
}

void DiveLogImportDialog::on_CSVFileSelector_clicked()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open CSV Log File"), ".", tr("CSV Files (*.csv);;All Files(*)"));
	ui->CSVFile->setText(filename);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!filename.isEmpty());
}

#define SET_VALUE_AND_CHECKBOX(CSV, BOX, VAL) ({\
		ui->CSV->blockSignals(true);\
		ui->CSV->setValue(VAL);\
		ui->CSV->setEnabled(VAL >= 0);\
		ui->BOX->setChecked(VAL >= 0);\
		ui->CSV->blockSignals(false);\
		})
void DiveLogImportDialog::on_knownImports_currentIndexChanged(int index)
{
	if (index == 0)
		return;

	ui->CSVTime->blockSignals(true);
	ui->CSVDepth->blockSignals(true);
	ui->CSVTime->setValue(CSVApps[index].time);
	ui->CSVDepth->setValue(CSVApps[index].depth);
	ui->CSVTime->blockSignals(false);
	ui->CSVDepth->blockSignals(false);
	SET_VALUE_AND_CHECKBOX(CSVTemperature, temperatureCheckBox, CSVApps[index].temperature);
	SET_VALUE_AND_CHECKBOX(CSVpo2, po2CheckBox, CSVApps[index].po2);
	SET_VALUE_AND_CHECKBOX(CSVcns, cnsCheckBox, CSVApps[index].cns);
	SET_VALUE_AND_CHECKBOX(CSVstopdepth, stopdepthCheckBox, CSVApps[index].stopdepth);
}

void DiveLogImportDialog::unknownImports(bool arg1)
{
	unknownImports();
}

void DiveLogImportDialog::unknownImports(int arg1)
{
	unknownImports();
}

void DiveLogImportDialog::unknownImports()
{
	ui->knownImports->setCurrentIndex(0);
}

void DiveLogImportDialog::on_CSVFile_textEdited()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!ui->CSVFile->text().isEmpty());
}

void DiveLogImportDialog::on_DiveLogFileSelector_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Dive Log File"), ".", tr("XML Files (*.xml);;UDDF/UDCF Files(*.uddf *.udcf);;All Files(*)"));
	ui->DiveLogFile->setText(fileNames.join(";"));
	if (fileNames.size())
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	else
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void DiveLogImportDialog::on_DiveLogFile_editingFinished()
{
	if (ui->DiveLogFile->text().size())
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	else
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

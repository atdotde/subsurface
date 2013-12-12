#ifndef DOWNLOADFROMDIVECOMPUTER_H
#define DOWNLOADFROMDIVECOMPUTER_H

#include <QDialog>
#include <QThread>
#include <QHash>
#include <QMap>
#include "../libdivecomputer.h"
#include "ui_downloadfromdivecomputer.h"

class DownloadThread : public QThread{
	Q_OBJECT
public:
	DownloadThread(QObject* parent, device_data_t* data);
	virtual void run();

	QString error;
private:
	device_data_t *data;
};

class QStringListModel;
class DownloadFromDCWidget : public QDialog{
	Q_OBJECT
public:
	explicit DownloadFromDCWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	static DownloadFromDCWidget *instance();
	void reject();

	enum states {
		INITIAL,
		DOWNLOADING,
		CANCELLING,
		CANCELLED,
		ERROR,
		DONE,
	};

public slots:
	void on_ok_clicked();
	void on_cancel_clicked();
	void on_vendor_currentIndexChanged(const QString& vendor);

	void onDownloadThreadFinished();
	void updateProgressBar();
	void runDialog();

private:
	void markChildrenAsDisabled();
	void markChildrenAsEnabled();

	Ui::DownloadFromDiveComputer ui;
	DownloadThread *thread;
	bool downloading;

	QStringList vendorList;
	QHash<QString, QStringList> productList;
	QMap<QString, dc_descriptor_t *> descriptorLookup;
	device_data_t data;
	int previousLast;

	QStringListModel *vendorModel;
	QStringListModel *productModel;
	void fill_computer_list();
	void fill_device_list();

	QTimer *timer;

public:
	bool preferDownloaded();
	void updateState(states state);
	states currentState;

};

#endif

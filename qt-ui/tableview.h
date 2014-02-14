#ifndef TABLEVIEW_H
#define TABLEVIEW_H

/* This TableView is prepared to have the CSS,
 * the methods to restore / save the state of
 * the column widths and the 'plus' button.
 */
#include <QWidget>

#include "ui_tableview.h"

class QPushButton;
class QAbstractItemModel;
class QModelIndex;
class QTableView;

class TableView : public QWidget {
Q_OBJECT
public:
	TableView(QWidget *parent = 0);
	virtual ~TableView();
	void setTitle(const QString& title);
	/* The model is expected to have a 'remove' slot, that takes a QModelIndex as parameter.
	 * It's also expected to have the column '1' as a trash icon. I most probably should create a
	 * proxy model and add that column, will mark that as TODO. see? marked.
	 */
	void setModel(QAbstractItemModel* model);
	void setBtnToolTip(const QString& tooltip);
	void fixPlusPosition();
	void edit(const QModelIndex& index);
	QTableView *view();
protected:
	virtual void showEvent(QShowEvent* );
	virtual void resizeEvent(QResizeEvent* );

signals:
	void addButtonClicked();

private:
	Ui::TableView ui;
	QPushButton *plusBtn;
};

#endif // TABLEVIEW_H

/*
 * divelistview.cpp
 *
 * classes for the divelist of Subsurface
 *
 */
#include "divelistview.h"
#include "models.h"
#include "modeldelegates.h"
#include "mainwindow.h"
#include "subsurfacewebservices.h"
#include "../display.h"
#include <QApplication>
#include <QHeaderView>
#include <QDebug>
#include <QSettings>
#include <QKeyEvent>
#include <QSortFilterProxyModel>
#include <QAction>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QFileDialog>
#include <string>
#include <iostream>
#include <exiv2/exiv2.hpp>

DiveListView::DiveListView(QWidget *parent) : QTreeView(parent), mouseClickSelection(false),
	sortColumn(0), currentOrder(Qt::DescendingOrder), searchBox(new QLineEdit(this))
{
	setItemDelegate(new DiveListDelegate );
	setUniformRowHeights(true);
	setItemDelegateForColumn(DiveTripModel::RATING, new StarWidgetsDelegate());
	QSortFilterProxyModel *model = new QSortFilterProxyModel(this);
	model->setSortRole(DiveTripModel::SORT_ROLE);
	model->setFilterKeyColumn(-1); // filter all columns
	model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	setModel(model);
	connect(model, SIGNAL(layoutChanged()), this, SLOT(fixMessyQtModelBehaviour()));

	setSortingEnabled(false);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	header()->setContextMenuPolicy(Qt::ActionsContextMenu);
	const QFontMetrics metrics(defaultModelFont());
	header()->setMinimumHeight(metrics.height() + 10);
	header()->setStretchLastSection(true);
	QAction *showSearchBox = new QAction(tr("Show Search Box"), this);
	showSearchBox->setShortcut( Qt::CTRL + Qt::Key_F);
	showSearchBox->setShortcutContext(Qt::WindowShortcut);
	addAction(showSearchBox);

	searchBox->installEventFilter(this);
	searchBox->hide();
	connect(showSearchBox, SIGNAL(triggered(bool)), this, SLOT(showSearchEdit()));
	connect(searchBox, SIGNAL(textChanged(QString)), model, SLOT(setFilterFixedString(QString)));
	setupUi();
}

DiveListView::~DiveListView()
{
	QSettings settings;
	settings.beginGroup("ListWidget");
	for (int i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++){
		if (isColumnHidden(i))
			continue;
		settings.setValue(QString("colwidth%1").arg(i), columnWidth(i));
	}
	settings.endGroup();
}

void DiveListView::setupUi(){
	QSettings settings;
	static bool firstRun = true;
	if(firstRun)
		backupExpandedRows();
	settings.beginGroup("ListWidget");
	/* if no width are set, use the calculated width for each column;
	 * for that to work we need to temporarily expand all rows */
	expandAll();
	for (int i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++) {
		if(isColumnHidden(i))
			continue;
		QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			setColumnWidth(i, width.toInt());
		else
			setColumnWidth(i, 100);
	}
	settings.endGroup();
	if(firstRun)
		restoreExpandedRows();
	else
		collapseAll();
	firstRun = false;
	setColumnWidth(lastVisibleColumn(), 10);
}

int DiveListView::lastVisibleColumn()
{
	int lastColumn = -1;
	for (int i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++) {
		if(isColumnHidden(i))
			continue;
		lastColumn = i;
	}
	return lastColumn;
}

void DiveListView::backupExpandedRows(){
	expandedRows.clear();
	for(int i = 0; i < model()->rowCount(); i++)
		if(isExpanded( model()->index(i, 0) ))
			expandedRows.push_back(i);
}

void DiveListView::restoreExpandedRows(){
	setAnimated(false);
	Q_FOREACH(const int &i, expandedRows)
		setExpanded( model()->index(i, 0), true );
	setAnimated(true);
}
void DiveListView::fixMessyQtModelBehaviour()
{
	QAbstractItemModel *m = model();
	for(int i = 0; i < model()->rowCount(); i++)
		if (m->rowCount( m->index(i, 0) ) != 0)
			setFirstColumnSpanned(i, QModelIndex(), true);
}

// this only remembers dives that were selected, not trips
void DiveListView::rememberSelection()
{
	selectedDives.clear();
	QItemSelection selection = selectionModel()->selection();
	Q_FOREACH(const QModelIndex& index , selection.indexes()) {
		if (index.column() != 0) // We only care about the dives, so, let's stick to rows and discard columns.
			continue;
		struct dive *d = (struct dive *) index.data(DiveTripModel::DIVE_ROLE).value<void*>();
		if (d)
			selectedDives.insert(d->divetrip, get_divenr(d));
	}
}

void DiveListView::restoreSelection()
{
	unselectDives();
	Q_FOREACH(dive_trip_t *trip, selectedDives.keys()){
		QList<int> divesOnTrip = getDivesInTrip(trip);
		QList<int> selectedDivesOnTrip = selectedDives.values(trip);

		// Trip was not selected, let's select single-dives.
		if (trip == NULL || divesOnTrip.count() != selectedDivesOnTrip.count()){
			Q_FOREACH(int i, selectedDivesOnTrip){
				selectDive(i);
			}
		}else{
			selectTrip(trip);
			Q_FOREACH(int i, selectedDivesOnTrip){
				selectDive(i);
			}
		}
	}
}

void DiveListView::selectTrip ( dive_trip_t* trip )
{
	if (!trip)
		return;

	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	QModelIndexList match = m->match(m->index(0,0), DiveTripModel::TRIP_ROLE, QVariant::fromValue<void*>(trip), 2, Qt::MatchRecursive);
	QItemSelectionModel::SelectionFlags flags;
	if (!match.count())
		return;
	QModelIndex idx = match.first();
	flags =  QItemSelectionModel::Select;
	flags |= QItemSelectionModel::Rows;
	selectionModel()->select(idx, flags);
	expand(idx);
}

void DiveListView::unselectDives()
{
	selectionModel()->clearSelection();
}

QList< dive_trip_t* > DiveListView::selectedTrips()
{
	QModelIndexList indexes = selectionModel()->selectedRows();
	QList<dive_trip_t*> ret;
	Q_FOREACH(const QModelIndex& index, indexes){
		dive_trip_t *trip = static_cast<dive_trip_t*>(index.data(DiveTripModel::TRIP_ROLE).value<void*>());
		if(!trip)
			continue;
		ret.push_back(trip);
	}
	return ret;
}

void DiveListView::selectDive(int i, bool scrollto, bool toggle)
{
	if( i == -1)
		return;
	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	QModelIndexList match = m->match(m->index(0,0), DiveTripModel::DIVE_IDX, i, 2, Qt::MatchRecursive);
	QItemSelectionModel::SelectionFlags flags;
	QModelIndex idx = match.first();
	flags = toggle ? QItemSelectionModel::Toggle : QItemSelectionModel::Select;
	flags |= QItemSelectionModel::Rows;
	selectionModel()->setCurrentIndex(idx, flags);
	if(idx.parent().isValid()){
		setAnimated(false);
		expand(idx.parent());
		setAnimated(true);
	}
	if (scrollto)
		scrollTo(idx, PositionAtCenter);
}

void DiveListView::selectDives(const QList< int >& newDiveSelection)
{
	if(!newDiveSelection.count())
		return;

	disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
		   this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
		   this, SLOT(currentChanged(QModelIndex,QModelIndex)));

	setAnimated(false);
	collapseAll();
	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Select | QItemSelectionModel::Rows;

	QItemSelection newDeselected = selectionModel()->selection();
	QModelIndexList diveList;

	int firstSelectedDive = -1;
	/* context for temp. variables. */{
		int i = 0;
		struct dive *dive;
		for_each_dive(i, dive){
			dive->selected = newDiveSelection.contains(i) == true;
			if(firstSelectedDive == -1 && dive->selected ){
				firstSelectedDive = i;
			}
		}
	}
	select_dive(firstSelectedDive);
	Q_FOREACH(int i, newDiveSelection){
		diveList.append(m->match(m->index(0,0), DiveTripModel::DIVE_IDX,
			i, 2, Qt::MatchRecursive).first());
	}
	Q_FOREACH(const QModelIndex& idx, diveList){
		selectionModel()->select(idx, flags);
		if(idx.parent().isValid() && !isExpanded(idx.parent())){
			expand(idx.parent());
		}
	}
	setAnimated(true);
	QTreeView::selectionChanged(selectionModel()->selection(), newDeselected);
	connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
		this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
		this, SLOT(currentChanged(QModelIndex,QModelIndex)));
	Q_EMIT currentDiveChanged(selected_dive);
	const QModelIndex& idx = m->match(m->index(0,0), DiveTripModel::DIVE_IDX,selected_dive, 2, Qt::MatchRecursive).first();
	scrollTo(idx);
}

void DiveListView::showSearchEdit()
{
	searchBox->show();
	searchBox->setFocus();
}

bool DiveListView::eventFilter(QObject* , QEvent* event)
{
	if(event->type() != QEvent::KeyPress)
		return false;
	QKeyEvent *keyEv = static_cast<QKeyEvent*>(event);
	if (keyEv->key() != Qt::Key_Escape)
		return false;

	searchBox->clear();
	searchBox->hide();
	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	m->setFilterFixedString(QString());
	return true;
}

// NOTE! This loses trip selection, because while we remember the
// dives, we don't remember the trips (see the "currentSelectedDives"
// list). I haven't figured out how to look up the trip from the
// index. TRIP_ROLE vs DIVE_ROLE?
void DiveListView::headerClicked(int i)
{
	DiveTripModel::Layout newLayout = i == (int) DiveTripModel::NR ? DiveTripModel::TREE : DiveTripModel::LIST;
	rememberSelection();
	unselectDives();
	/* No layout change? Just re-sort, and scroll to first selection, making sure all selections are expanded */
	if (currentLayout == newLayout) {
		currentOrder = (currentOrder == Qt::DescendingOrder) ? Qt::AscendingOrder : Qt::DescendingOrder;
		sortByColumn(i, currentOrder);
	} else {
		// clear the model, repopulate with new indexes.
		if(currentLayout == DiveTripModel::TREE){
			backupExpandedRows();
		}
		reload(newLayout, false);
		currentOrder = Qt::DescendingOrder;
		sortByColumn(i, currentOrder);
		if (newLayout == DiveTripModel::TREE){
			restoreExpandedRows();
		}
	}
	restoreSelection();
}

void DiveListView::reload(DiveTripModel::Layout layout, bool forceSort)
{
	if (layout == DiveTripModel::CURRENT)
		layout = currentLayout;
	else
		currentLayout = layout;
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	header()->setClickable(true);
#else
	header()->setSectionsClickable(true);
#endif
	connect(header(), SIGNAL(sectionPressed(int)), this, SLOT(headerClicked(int)), Qt::UniqueConnection);

	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	QAbstractItemModel *oldModel = m->sourceModel();
	if (oldModel){
		oldModel->deleteLater();
	}
	DiveTripModel *tripModel = new DiveTripModel(this);
	tripModel->setLayout(layout);

	m->setSourceModel(tripModel);

	if(!forceSort)
		return;

	sortByColumn(sortColumn, currentOrder);
	if (amount_selected && current_dive != NULL) {
		selectDive(selected_dive, true);
	} else {
		QModelIndex firstDiveOrTrip = m->index(0,0);
		if (firstDiveOrTrip.isValid()) {
			if (m->index(0,0, firstDiveOrTrip).isValid())
				setCurrentIndex(m->index(0,0, firstDiveOrTrip));
			else
				setCurrentIndex(firstDiveOrTrip);
		}
	}
	setupUi();
	if(selectedIndexes().count()){
		QModelIndex curr = selectedIndexes().first();
		curr = curr.parent().isValid() ? curr.parent() : curr;
		if(!isExpanded(curr)){
			setAnimated(false);
			expand(curr);
			setAnimated(true);
		}
	}
	if(currentLayout == DiveTripModel::TREE){
		fixMessyQtModelBehaviour();
	}
}

void DiveListView::reloadHeaderActions()
{
	// Populate the context menu of the headers that will show
	// the menu to show / hide columns.
	if (!header()->actions().size()) {
		QSettings s;
		s.beginGroup("DiveListColumnState");
		for(int i = 0; i < model()->columnCount(); i++) {
			QString title = QString("%1").arg(model()->headerData(i, Qt::Horizontal).toString());
			QString settingName = QString("showColumn%1").arg(i);
			QAction *a = new QAction(title, header());
			bool showHeaderFirstRun = !(
				i == DiveTripModel::MAXCNS
				|| i == DiveTripModel::NITROX
				|| i == DiveTripModel::OTU
				|| i == DiveTripModel::TEMPERATURE
				|| i == DiveTripModel::TOTALWEIGHT
				|| i == DiveTripModel::SUIT
				|| i == DiveTripModel::CYLINDER
				|| i == DiveTripModel::SAC );
			bool shown = s.value(settingName, showHeaderFirstRun).toBool();
			a->setCheckable(true);
			a->setChecked(shown);
			a->setProperty("index", i);
			a->setProperty("settingName", settingName);
			connect(a, SIGNAL(triggered(bool)), this, SLOT(toggleColumnVisibilityByIndex()));
			header()->addAction(a);
			setColumnHidden(i, !shown);
		}
		s.endGroup();
	} else {
		for(int i = 0; i < model()->columnCount(); i++) {
			QString title = QString("%1").arg(model()->headerData(i, Qt::Horizontal).toString());
			header()->actions()[i]->setText(title);
		}
	}
}

void DiveListView::toggleColumnVisibilityByIndex()
{
	QAction *action = qobject_cast<QAction*>(sender());
	if (!action)
		return;

	QSettings s;
	s.beginGroup("DiveListColumnState");
	s.setValue(action->property("settingName").toString(), action->isChecked());
	s.endGroup();
	s.sync();
	setColumnHidden(action->property("index").toInt(), !action->isChecked());
	setColumnWidth(lastVisibleColumn(), 10);
}

void DiveListView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
	if (!current.isValid())
		return;
	scrollTo(current);
}

void DiveListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QItemSelection newSelected = selected.size() ? selected : selectionModel()->selection();
	QItemSelection newDeselected = deselected;

	disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),         this, SLOT(currentChanged(QModelIndex,QModelIndex)));

	Q_FOREACH(const QModelIndex& index, newDeselected.indexes()) {
		if (index.column() != 0)
			continue;
		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, DiveTripModel::DIVE_ROLE).value<void*>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				struct dive *child = (struct dive*) model->data(index.child(0,0), DiveTripModel::DIVE_ROLE).value<void*>();
				while (child) {
					deselect_dive(get_divenr(child));
					child = child->next;
				}
			}
		} else {
			deselect_dive(get_divenr(dive));
		}
	}
	Q_FOREACH(const QModelIndex& index, newSelected.indexes()) {
		if (index.column() != 0)
			continue;

		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, DiveTripModel::DIVE_ROLE).value<void*>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				QItemSelection selection;
				struct dive *child = (struct dive*) model->data(index.child(0,0), DiveTripModel::DIVE_ROLE).value<void*>();
				while (child) {
					select_dive(get_divenr(child));
					child = child->next;
				}
				selection.select(index.child(0,0), index.child(model->rowCount(index) -1 , 0));
				selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::NoUpdate);
				if (!isExpanded(index))
					expand(index);
			}
		} else {
			select_dive(get_divenr(dive));
		}
	}
	QTreeView::selectionChanged(selectionModel()->selection(), newDeselected);
	connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),         this, SLOT(currentChanged(QModelIndex,QModelIndex)));
	// now that everything is up to date, update the widgets
	Q_EMIT currentDiveChanged(selected_dive);
}

static bool can_merge(const struct dive *a, const struct dive *b)
{
	if (!a || !b)
		return false;
	if (a->when > b->when)
		return false;
	/* Don't merge dives if there's more than half an hour between them */
	if (a->when + a->duration.seconds + 30*60 < b->when)
		return false;
	return true;
}

void DiveListView::mergeDives()
{
	int i;
	struct dive *dive, *maindive = NULL;

	for_each_dive(i, dive) {
		if (dive->selected) {
			if (!can_merge(maindive, dive)) {
				maindive = dive;
			} else {
				maindive = merge_two_dives(maindive, dive);
				i--; // otherwise we skip a dive in the freshly changed list
			}
		}
	}
	mainWindow()->refreshDisplay();
}

void DiveListView::merge_trip(const QModelIndex &a, int offset)
{
	int i = a.row() + offset;
	QModelIndex b = a.sibling(i,0);

	dive_trip_t *trip_a = (dive_trip_t *) a.data(DiveTripModel::TRIP_ROLE).value<void*>();
	dive_trip_t *trip_b = (dive_trip_t *) b.data(DiveTripModel::TRIP_ROLE).value<void*>();

	if (trip_a == trip_b || !trip_a || !trip_b)
		return;

	if (!trip_a->location && trip_b->location)
		trip_a->location = strdup(trip_b->location);
	if (!trip_a->notes && trip_b->notes)
		trip_a->notes = strdup(trip_b->notes);
	while (trip_b->dives)
		add_dive_to_trip(trip_b->dives, trip_a);
	rememberSelection();
	reload(currentLayout, false);
	fixMessyQtModelBehaviour();
	restoreSelection();
	mark_divelist_changed(true);
}

void DiveListView::mergeTripAbove()
{
	merge_trip(contextMenuIndex, -1);
}

void DiveListView::mergeTripBelow()
{
	merge_trip(contextMenuIndex, +1);
}

void DiveListView::removeFromTrip()
{
	int i;
	struct dive *d;
	for_each_dive(i, d) {
		if (d->selected)
			remove_dive_from_trip(d, false);
	}
	rememberSelection();
	reload(currentLayout, false);
	fixMessyQtModelBehaviour();
	restoreSelection();
	mark_divelist_changed(true);
}

void DiveListView::newTripAbove()
{
	dive_trip_t *trip;
	int idx;
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (!d) // shouldn't happen as we only are setting up this action if this is a dive
		return;
	rememberSelection();
	trip = create_and_hookup_trip_from_dive(d);
	for_each_dive(idx, d) {
		if (d->selected)
			add_dive_to_trip(d, trip);
	}
	trip->expanded = 1;
	reload(currentLayout, false);
	fixMessyQtModelBehaviour();
	mark_divelist_changed(true);
	restoreSelection();
}

void DiveListView::addToTripAbove()
{
	int idx, delta = (currentOrder == Qt::AscendingOrder) ? -1 : +1;
	dive_trip_t *trip = NULL;
	struct dive *pd = NULL;
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (!d) // shouldn't happen as we only are setting up this action if this is a dive
		return;
	rememberSelection();
	if (d->selected) { // we are right-clicking on one of possibly many selected dive(s)
		// find the top selected dive, depending on the list order
		if (delta == 1) {
			for_each_dive(idx, d) {
				if (d->selected)
					pd = d;
			}
			d = pd; // this way we have the chronologically last
		} else {
			for_each_dive(idx, d) {
				if (d->selected)
					break; // now that's the chronologically first
			}
		}
	}
	// now find the trip "above" in the dive list
	if ((pd = get_dive(get_divenr(d) + delta)) != NULL) {
		trip = pd->divetrip;
	}
	if (!pd || !trip)
		// previous dive wasn't in a trip, so something is wrong
		return;
	add_dive_to_trip(d, trip);
	if (d->selected) { // there are possibly other selected dives that we should add
		for_each_dive(idx, d) {
			if (d->selected)
				add_dive_to_trip(d, trip);
		}
	}
	trip->expanded = 1;
	mark_divelist_changed(true);
	reload(currentLayout, false);
	restoreSelection();
	fixMessyQtModelBehaviour();
}

void DiveListView::markDiveInvalid()
{
	int i;
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (!d)
		return;
	for_each_dive (i, d) {
		if (!d->selected)
			continue;
		// now mark the dive invalid... how do we do THAT?
		// d->invalid = true;
	}
	if (amount_selected == 0) {
		mainWindow()->cleanUpEmpty();
	}
	mark_divelist_changed(true);
	mainWindow()->refreshDisplay();
	if(prefs.display_invalid_dives == false) {
		clearSelection();
		// select top dive that isn't marked invalid
		rememberSelection();
	}
	fixMessyQtModelBehaviour();
}

void DiveListView::deleteDive()
{
	int i;
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (!d)
		return;
	// after a dive is deleted the ones following it move forward in the dive_table
	// so instead of using the for_each_dive macro I'm using an explicit for loop
	// to make this easier to understand
	int lastDiveNr = -1;
	for (i = 0; i < dive_table.nr; i++) {
		d = get_dive(i);
		if (!d->selected)
			continue;
		delete_single_dive(i);
		i--; // so the next dive isn't skipped... it's now #i
		lastDiveNr = i;
	}
	if (amount_selected == 0) {
		mainWindow()->cleanUpEmpty();
	}
	mark_divelist_changed(true);
	mainWindow()->refreshDisplay();
	if(lastDiveNr != -1){
		clearSelection();
		selectDive(lastDiveNr);
		rememberSelection();
	}
	fixMessyQtModelBehaviour();
}

void DiveListView::testSlot()
{
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (d) {
		qDebug("testSlot called on dive #%d", d->number);
	} else {
		QModelIndex child = contextMenuIndex.child(0, 0);
		d = (struct dive *) child.data(DiveTripModel::DIVE_ROLE).value<void*>();
		if (d)
			qDebug("testSlot called on trip including dive #%d", d->number);
		else
			qDebug("testSlot called on trip with no dive");
	}
}

void DiveListView::contextMenuEvent(QContextMenuEvent *event)
{
	QAction *collapseAction = NULL;
	// let's remember where we are
	contextMenuIndex = indexAt(event->pos());
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	dive_trip_t *trip = (dive_trip_t *) contextMenuIndex.data(DiveTripModel::TRIP_ROLE).value<void*>();
	QMenu popup(this);
	if (currentLayout == DiveTripModel::TREE) {
		popup.addAction(tr("expand all"), this, SLOT(expandAll()));
		popup.addAction(tr("collapse all"), this, SLOT(collapseAll()));
		collapseAction = popup.addAction(tr("collapse others"), this, SLOT(collapseAll()));
		if (d) {
			popup.addAction(tr("remove dive(s) from trip"), this, SLOT(removeFromTrip()));
			popup.addAction(tr("create new trip above"), this, SLOT(newTripAbove()));
			popup.addAction(tr("add dive(s) to trip immediately above"), this, SLOT(addToTripAbove()));
		}
		if (trip) {
			popup.addAction(tr("merge trip with trip above"), this, SLOT(mergeTripAbove()));
			popup.addAction(tr("merge trip with trip below"), this, SLOT(mergeTripBelow()));
		}
	}
	if (d) {
		popup.addAction(tr("delete dive(s)"), this, SLOT(deleteDive()));
#if 0
		popup.addAction(tr("mark dive(s) invalid", this, SLOT(markDiveInvalid())));
#endif
	}
	if (amount_selected > 1 && consecutive_selected())
		popup.addAction(tr("merge selected dives"), this, SLOT(mergeDives()));
	if (amount_selected >= 1) {
		popup.addAction(tr("save As"), this, SLOT(saveSelectedDivesAs()));
		popup.addAction(tr("export As UDDF"), this, SLOT(exportSelectedDivesAsUDDF()));
		popup.addAction(tr("shift times"), this, SLOT(shiftTimes()));
		popup.addAction(tr("load images"), this, SLOT(loadImages()));
	}
	if (d)
		popup.addAction(tr("upload dive(s) to divelogs.de"), this, SLOT(uploadToDivelogsDE()));
	// "collapse all" really closes all trips,
	// "collapse" keeps the trip with the selected dive open
	QAction * actionTaken = popup.exec(event->globalPos());
	if (actionTaken == collapseAction && collapseAction) {
		this->setAnimated(false);
		selectDive(selected_dive, true);
		scrollTo(selectedIndexes().first());
		this->setAnimated(true);
	}
	event->accept();
}

void DiveListView::saveSelectedDivesAs()
{
	QSettings settings;
	QString lastDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastDir")) {
		if(QDir::setCurrent(settings.value("LastDir").toString())) {
			lastDir = settings.value("LastDir").toString();
		}
	}
	settings.endGroup();

	QString fileName = QFileDialog::getSaveFileName(mainWindow(), tr("Save Dives As..."), QDir::homePath());
	if (fileName.isEmpty())
		return;

	// Keep last open dir
	QFileInfo fileInfo(fileName);
	settings.beginGroup("FileDialog");
	settings.setValue("LastDir",fileInfo.dir().path());
	settings.endGroup();

	QByteArray bt = QFile::encodeName(fileName);
	save_dives_logic(bt.data(), true);
}

void DiveListView::exportSelectedDivesAsUDDF()
{
	QString filename;
	QFileInfo fi(system_default_filename());

	filename = QFileDialog::getSaveFileName(this, tr("Save File as"), fi.absolutePath(),
						tr("UDDF files (*.uddf *.UDDF)"));
	if (!filename.isNull() && !filename.isEmpty())
		export_dives_uddf(filename.toUtf8(), true);
}


void DiveListView::shiftTimes()
{
	ShiftTimesDialog::instance()->show();
}

void DiveListView::loadImages()
{
  Exiv2::Image::AutoPtr exif;
  QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Image Files"), lastUsedImageDir(), tr("Image Files (*.jpg *.jpeg *.pnm *.tif *.tiff)"));

	if (fileNames.isEmpty())
		return;

	updateLastUsedImageDir(QFileInfo(fileNames[0]).dir().path());

	for (int i = 0; i < fileNames.size(); ++i) {
	  printf("Analysing |%s|\n",fileNames.at(i).toUtf8().data());
	  //exif = Exiv2::ImageFactory::open(std::string(fileNames.at(i).toUtf8().data()));

      	  exif = Exiv2::ImageFactory::open ("wreck.jpg");
	  if (exif.get() == 0)
	    continue;
	  exif->readMetadata();
	  Exiv2::ExifData &exifData = exif->exifData();
	  if (exifData.empty()) 
	    continue;
	  Exiv2::ExifData::const_iterator end = exifData.end();
	printf("Let's look at the images!\n");
	  for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
	    const char* key = i->key().c_str();
		const char* value = i->value().toString().c_str();
	    printf("Tag: %s Value: %s\n", key, value);
	  }
	}
}

void DiveListView::uploadToDivelogsDE()
{
	DivelogsDeWebServices::instance()->prepareDivesForUpload();
}

QString DiveListView::lastUsedImageDir()
{
	QSettings settings;
	QString lastImageDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastImageDir"))
		if (QDir::setCurrent(settings.value("LastImageDir").toString()))
			lastImageDir = settings.value("LastIamgeDir").toString();
	return lastImageDir;
}

void DiveListView::updateLastUsedImageDir(const QString& dir)
{
	QSettings s;
	s.beginGroup("FileDialog");
	s.setValue("LastImageDir", dir);
}

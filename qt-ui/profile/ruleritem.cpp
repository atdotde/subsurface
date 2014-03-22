#include "ruleritem.h"
#include "divetextitem.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDebug>

#include <stdint.h>

#include "profile.h"
#include "display.h"

RulerNodeItem2::RulerNodeItem2() : entry(NULL), ruler(NULL)
{
	memset(&pInfo, 0, sizeof(pInfo));
	setRect(-8, -8, 16, 16);
	setBrush(QColor(0xff, 0, 0, 127));
	setPen(QColor(Qt::red));
	setFlag(ItemIsMovable);
	setFlag(ItemSendsGeometryChanges);
	setFlag(ItemIgnoresTransformations);
}

void RulerNodeItem2::setPlotInfo(plot_info& info)
{
	pInfo = info;
	entry = pInfo.entry;
}

void RulerNodeItem2::setRuler(RulerItem2 *r)
{
	ruler = r;
}

void RulerNodeItem2::recalculate()
{
	struct plot_data *data = pInfo.entry + (pInfo.nr - 1);
	uint16_t count = 0;
	if (x() < 0) {
		setPos(0, y());
	} else if (x() > timeAxis->posAtValue(data->sec)) {
		setPos(timeAxis->posAtValue(data->sec), depthAxis->posAtValue(data->depth));
	} else {
		data = pInfo.entry;
		count = 0;
		while (timeAxis->posAtValue(data->sec) < x() && count < pInfo.nr) {
			data = pInfo.entry + count;
			count++;
		}
		setPos(timeAxis->posAtValue(data->sec), depthAxis->posAtValue(data->depth));
		entry = data;
	}
}

QVariant RulerNodeItem2::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionHasChanged) {
		recalculate();
		ruler->recalculate();
	}
	return QGraphicsEllipseItem::itemChange(change, value);
}

RulerItem2::RulerItem2() : source(new RulerNodeItem2()),
	dest(new RulerNodeItem2()),
	timeAxis(NULL),
	depthAxis(NULL),
	textItemBack(new QGraphicsRectItem(this)),
	textItem(new QGraphicsSimpleTextItem(this))
{
	memset(&pInfo, 0, sizeof(pInfo));
	source->setRuler(this);
	dest->setRuler(this);
	textItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	textItemBack->setBrush(QColor(0xff, 0xff, 0xff, 190));
	textItemBack->setPen(QColor(Qt::white));
	setPen(QPen(QColor(Qt::black), 0.0));
}

void RulerItem2::recalculate()
{
	char buffer[500];
	QPointF tmp;
	QFont font;
	QFontMetrics fm(font);

	if (timeAxis == NULL || depthAxis == NULL || pInfo.nr == 0)
		return;

	prepareGeometryChange();
	startPoint = mapFromItem(source, 0, 0);
	endPoint = mapFromItem(dest, 0, 0);

	if (startPoint.x() > endPoint.x()) {
		tmp = endPoint;
		endPoint = startPoint;
		startPoint = tmp;
	}
	QLineF line(startPoint, endPoint);
	setLine(line);
	compare_samples(source->entry, dest->entry, buffer, 500, 1);
	text = QString(buffer);

	// draw text
	QGraphicsView *view = scene()->views().first();
	QPoint begin = view->mapFromScene(mapToScene(startPoint));
	textItem->setText(text);
	qreal tgtX = startPoint.x();
	const qreal diff = begin.x() + textItem->boundingRect().width();
	// clamp so that the text doesn't go out of the screen to the right
	if (diff > view->width()) {
		begin.setX(begin.x() - (diff - view->width()));
		tgtX = mapFromScene(view->mapToScene(begin)).x();
	}
	// always show the text bellow the lowest of the start and end points
	qreal tgtY = (startPoint.y() >= endPoint.y()) ? startPoint.y() : endPoint.y();
	textItem->setPos(tgtX - 1, tgtY + 4);

	// setup the text background
	textItemBack->setVisible(startPoint.x() != endPoint.x());
	QPointF wh = mapFromScene(view->mapToScene(QPoint(textItem->boundingRect().width(),
		textItem->boundingRect().height())));
	textItemBack->setRect(tgtX - 2, tgtY + 3, wh.x() + 2, wh.y() + 3);
}

RulerNodeItem2 *RulerItem2::sourceNode() const
{
	return source;
}

RulerNodeItem2 *RulerItem2::destNode() const
{
	return dest;
}

void RulerItem2::setPlotInfo(plot_info info)
{
	pInfo = info;
	dest->setPlotInfo(info);
	source->setPlotInfo(info);
	dest->recalculate();
	source->recalculate();
	recalculate();
}

void RulerItem2::setAxis(DiveCartesianAxis *time, DiveCartesianAxis *depth)
{
	timeAxis = time;
	depthAxis = depth;
	dest->depthAxis = depth;
	dest->timeAxis = time;
	source->depthAxis = depth;
	source->timeAxis = time;
	recalculate();
}

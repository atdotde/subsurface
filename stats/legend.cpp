// SPDX-License-Identifier: GPL-2.0
#include "legend.h"
#include "statscolors.h"
#include "zvalues.h"

#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPen>

static const double legendBorderSize = 2.0;
static const double legendBoxBorderSize = 1.0;
static const double legendBoxBorderRadius = 4.0;	// radius of rounded corners
static const double legendBoxScale = 0.8;		// 1.0: text-height of the used font
static const double legendInternalBorderSize = 2.0;
static const QColor legendColor(0x00, 0x8e, 0xcc, 192); // Note: fourth argument is opacity
static const QColor legendBorderColor(Qt::black);

Legend::Legend(StatsView &view, const std::vector<QString> &names) :
	ChartRectItem(view, QPen(legendBorderColor, legendBorderSize), QBrush(legendColor), legendBoxBorderRadius),
	displayedItems(0), width(0.0), height(0.0),
	font(QFont())	// Make configurable
{
	entries.reserve(names.size());
	QFontMetrics fm(font);
	fontHeight = fm.height();
	int idx = 0;
	for (const QString &name: names)
		entries.emplace_back(name, idx++, (int)names.size(), fm);
}

Legend::Entry::Entry(const QString &name, int idx, int numBins, const QFontMetrics &fm) :
	name(name),
	rectBrush(QBrush(binColor(idx, numBins)))
{
	width = fm.height() + 2.0 * legendBoxBorderSize + fm.size(Qt::TextSingleLine, name).width();
}

void Legend::hide()
{
	ChartRectItem::resize(QSizeF(1,1));
	img->fill(Qt::transparent);
}

void Legend::resize()
{
	if (entries.empty())
		return hide();

	QSizeF size = sceneSize();

	// Silly heuristics: make the legend at most half as high and half as wide as the chart.
	// Not sure if that makes sense - this might need some optimization.
	int maxRows = static_cast<int>(size.height() / 2.0 - 2.0 * legendInternalBorderSize) / fontHeight;
	if (maxRows <= 0)
		return hide();
	int numColumns = ((int)entries.size() - 1) / maxRows + 1;
	int numRows = ((int)entries.size() - 1) / numColumns + 1;

	double x = legendInternalBorderSize;
	displayedItems = 0;
	for (int col = 0; col < numColumns; ++col) {
		double y = legendInternalBorderSize;
		double nextX = x;

		for (int row = 0; row < numRows; ++row) {
			int idx = col * numRows + row;
			if (idx >= (int)entries.size())
				break;
			entries[idx].pos = QPointF(x, y);
			nextX = std::max(nextX, x + entries[idx].width);
			y += fontHeight;
			++displayedItems;
		}
		x = nextX;
		width = nextX;
		if (width >= size.width() / 2.0) // More than half the chart-width -> give up
			break;
	}
	width += legendInternalBorderSize;
	height = 2 * legendInternalBorderSize + numRows * fontHeight;

	ChartRectItem::resize(QSizeF(width, height));

	// Paint rectangles
	painter->setPen(QPen(legendBorderColor, legendBoxBorderSize));
	for (int i = 0; i < displayedItems; ++i) {
		QPointF itemPos = entries[i].pos;
		painter->setBrush(entries[i].rectBrush);
		QRectF rect(itemPos, QSizeF(fontHeight, fontHeight));
		// Decrease box size by legendBoxScale factor
		double delta = fontHeight * (1.0 - legendBoxScale) / 2.0;
		rect = rect.adjusted(delta, delta, -delta, -delta);
		painter->drawRect(rect);
	}

	// Paint labels
	painter->setPen(darkLabelColor); // QPainter uses pen not brush for text!
	painter->setFont(font);
	for (int i = 0; i < displayedItems; ++i) {
		QPointF itemPos = entries[i].pos;
		itemPos.rx() += fontHeight + 2.0 * legendBoxBorderSize;
		QRectF rect(itemPos, QSizeF(entries[i].width, fontHeight));
		painter->drawText(rect, entries[i].name);
	}

	// For now, place the legend in the top right corner.
	QPointF pos(size.width() - width - 10.0, 10.0);
	setPos(pos);
}

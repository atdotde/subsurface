#ifndef ANIMATIONFUNCTIONS_H
#define ANIMATIONFUNCTIONS_H

#include <QtGlobal>
class QObject;

namespace Animations{
	void hide(QObject *obj);
	void moveTo(QObject *obj, qreal x, qreal y);
};

#endif
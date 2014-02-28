#ifndef STARWIDGET_H
#define STARWIDGET_H

#include <QWidget>

enum StarConfig {
	SPACING = 2,
	IMG_SIZE = 16,
	TOTALSTARS = 5
};

class StarWidget : public QWidget {
	Q_OBJECT
public:
	explicit StarWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	int currentStars() const;

	/*reimp*/ QSize sizeHint() const;

	static QPixmap starActive();
	static QPixmap starInactive();

signals:
	void valueChanged(int stars);

public
slots:
	void setCurrentStars(int value);
	void setReadOnly(bool readOnly);

protected:
	/*reimp*/ void mouseReleaseEvent(QMouseEvent *);
	/*reimp*/ void paintEvent(QPaintEvent *);
	/*reimp*/ void focusInEvent(QFocusEvent *);
	/*reimp*/ void focusOutEvent(QFocusEvent *);
	/*reimp*/ void keyPressEvent(QKeyEvent *);

private:
	int current;
	bool readOnly;

	static QPixmap *activeStar;
	static QPixmap *inactiveStar;
	QPixmap grayImage(QPixmap *coloredImg);
};

#endif // STARWIDGET_H

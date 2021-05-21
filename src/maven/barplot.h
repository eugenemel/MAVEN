#ifndef BARPLOT_H
#define BARPLOT_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"


class MainWindow;
class PeakGroup;
class QGraphicsItem;
class QGraphicsScene;

class BarPlot : public QObject, public QGraphicsItem
{
    Q_OBJECT
   
#if QT_VERSION >= 0x040600
    Q_INTERFACES( QGraphicsItem )
#endif

	static PeakGroup::QType qtype; 

public:
	BarPlot(QGraphicsItem *parent, QGraphicsScene *scene);
    BarPlot(QGraphicsItem *parent, QGraphicsScene *scene, MainWindow *mainWindow);
	~BarPlot();
	void setPeakGroup(PeakGroup *group);
        QRectF boundingRect() const;
	void setMainWindow(MainWindow* mw) { _mw = mw; }
	void switchQValue();
	void clear();
        void showSampleNames(bool flag) { _showSampleNames=flag; }
        void showIntensityText(bool flag)   { _showIntensityText=flag; }
        void showQValueType(bool flag) { _showQValueType=flag; }

    bool isOnEicWidget = true;
    int _latestWidth = 1;
    int _latestHeight = 1;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void mousePressEvent (QGraphicsSceneMouseEvent*) { if (!isOnEicWidget) return; switchQValue(); update(); }
   void	wheelEvent ( QGraphicsSceneWheelEvent * event );
	
private:
    pair<char, int> getNumTypeAndNumPrec(float maxYvalue);
	QVector<QString> _labels;
	QVector<QColor>  _colors; 
	QVector<float>   _yvalues;
	int _width;
	int _height;
	float _barwidth;

        bool _showSampleNames;
        bool _showIntensityText;
        bool _showQValueType;

        MainWindow* _mw;
        float _yValuesMean = 0.0f;
        float _yValuesCoV = 0.0f;
};

#endif

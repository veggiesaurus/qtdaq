#include "Plots/SortedPairPlot.h"

SortedPairPlot::SortedPairPlot( QWidget *parent ):
    QwtPlot( parent ),
    d_curve( NULL )
{
    canvas()->setStyleSheet(
        "border: 2px solid Black;"
        "border-radius: 15px;"
        "background-color: qlineargradient( x1: 0, y1: 0, x2: 0, y2: 1,"
            "stop: 0 LemonChiffon, stop: 1 PaleGoldenrod );"
    );

    // attach curve
    d_curve = new QwtPlotCurve( "Scattered Points" );
    d_curve->setPen( QColor( "Purple" ) );

	d_curve_proportional = new QwtPlotCurve( "Proportional" );
    d_curve_proportional->setPen( QColor( "Red" ) );
	double xValsProportional[]={-1000000, 1000000};
	double yValsProportional[]={-1000000, 1000000};
	d_curve_proportional->setSamples(xValsProportional, yValsProportional, 2);
	d_curve_proportional->setRenderHint(QwtPlotItem::RenderAntialiased, true);
	d_curve_proportional->attach( this );
    // when using QwtPlotCurve::ImageBuffer simple dots can be
    // rendered in parallel on multicore systems.
    d_curve->setRenderThreadCount( 0 ); // 0: use QThread::idealThreadCount()

    d_curve->attach( this );

	QwtSymbol* symbol=new QwtSymbol(QwtSymbol::XCross);
    symbol->setSize(5);
    setSymbol(symbol );

    // panning with the left mouse button
    (void )new QwtPlotPanner( canvas() );

    // zoom in/out with the wheel
    QwtPlotMagnifier *magnifier = new QwtPlotMagnifier( canvas() );
    magnifier->setMouseButton( Qt::NoButton );    
}

void SortedPairPlot::setSymbol( QwtSymbol *symbol )
{
    d_curve->setSymbol( symbol );
    d_curve->setStyle( QwtPlotCurve::Dots );
}

void SortedPairPlot::setSamples( const QVector<double> &samplesX,  const QVector<double> &samplesY)
{
    d_curve->setPaintAttribute( 
        QwtPlotCurve::ImageBuffer, samplesX.size() > 1000 );

	d_curve->setSamples( samplesX, samplesY );
}
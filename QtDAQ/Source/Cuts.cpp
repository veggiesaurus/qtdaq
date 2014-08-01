#include "Cuts.h"

QDataStream& operator >>(QDataStream& in, Condition& c)
{
    in >>*((quint32*)&c);
    return in;
}

QDataStream &operator<<(QDataStream &out, const Cut &obj)
{
	out<<UI_SAVE_VERSION;
	out<<obj.name<<obj.channel;
     return out;
}
 
QDataStream &operator>>(QDataStream &in, Cut &obj)
{
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	in>>obj.name>>obj.channel;
    return in;
}

bool LinearCut::isInCutRegion(SampleStatistics stats)
{
	double val=valueFromStatsAndParameter(stats, parameter);
	return (val>=cutMin && val<=cutMax);
}

QDataStream &operator<<(QDataStream &out, const LinearCut &obj)
{
	out<<static_cast<const Cut&>(obj);
	out<<UI_SAVE_VERSION;
	out<<obj.parameter<<obj.cutMin<<obj.cutMax;
    return out;
}
 
QDataStream &operator>>(QDataStream &in, LinearCut &obj)
{
	in>>static_cast<Cut&>(obj);
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	in>>*((quint32*)&obj.parameter)>>obj.cutMin>>obj.cutMax;
    return in;
}

bool PolygonalCut::isInCutRegion(SampleStatistics stats)
{
	double valX=valueFromStatsAndParameter(stats, parameterX);
	double valY=valueFromStatsAndParameter(stats, parameterY);
	QPointF valPoint(valX, valY);
	return points.containsPoint(valPoint, Qt::FillRule::OddEvenFill);
}

QDataStream &operator<<(QDataStream &out, const PolygonalCut &obj)
{
	out<<static_cast<const Cut&>(obj);
	out<<UI_SAVE_VERSION;
	out<<obj.parameterX<<obj.parameterY<<obj.points;
    return out;
}
 
QDataStream &operator>>(QDataStream &in, PolygonalCut &obj)
{
	in>>static_cast<Cut&>(obj);
	quint32 uiSaveVersion;
	in>>uiSaveVersion;
	in>>*((quint32*)&obj.parameterX)>>*((quint32*)&obj.parameterY)>>obj.points;
    return in;
}
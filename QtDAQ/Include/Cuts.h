#pragma once

#include <QString>
#include <QPolygonF>
#include <QDataStream>

#include "DRS4Acquisition.h"
#include "globals.h"

enum Condition
{
	IGNORE_CUT=0,
	INCLUDE_REGION=1,
	EXCLUDE_REGION=-1
};
Q_DECLARE_METATYPE(Condition)
QDataStream& operator >>(QDataStream& in, Condition& e);

struct Cut
{	
	int channel;
	QString name;
	virtual bool isInCutRegion(SampleStatistics stats){return true;};
};
Q_DECLARE_METATYPE(Cut)

QDataStream &operator<<(QDataStream &out, const Cut &obj);
QDataStream &operator>>(QDataStream &in, Cut &obj);

struct LinearCut: Cut
{
	HistogramParameter parameter;
	double cutMin, cutMax;
	bool isInCutRegion(SampleStatistics stats);
};
Q_DECLARE_METATYPE(LinearCut)

QDataStream &operator<<(QDataStream &out, const LinearCut &obj);
QDataStream &operator>>(QDataStream &in, LinearCut &obj);

struct PolygonalCut: Cut
{
	HistogramParameter parameterX, parameterY;
	QPolygonF points;
	bool isInCutRegion(SampleStatistics stats);
};
Q_DECLARE_METATYPE(PolygonalCut)

QDataStream &operator<<(QDataStream &out, const PolygonalCut &obj);
QDataStream &operator>>(QDataStream &in, PolygonalCut &obj);
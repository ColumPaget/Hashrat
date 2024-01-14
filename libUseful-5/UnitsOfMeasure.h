/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_MEASURE_H
#define LIBUSEFUL_MEASURE_H

#include "defines.h"
#include "includes.h"

/*
Functions relating to convertion between SI Units/ Metric and and IEC units. So, is kilo 1024, or 1000?
*/

#ifdef __cplusplus
extern "C" {
#endif


//a simple power function included to allow libUseful to build without needing libmath/libm
double ToPower(double val, double power);

const char *ToSIUnit(double Value, int Base, int Precision);
#define ToIEC(Value, Precision) (ToSIUnit((Value), 1024, Precision))
#define ToMetric(Value, Precision) (ToSIUnit((Value), 1000, Precision))

//Convert to and from metric
double FromSIUnit(const char *Data, int BAse);
#define FromIEC(Value, Precision) (FromSIUnit((Value), 1024))
#define FromMetric(Value, Precision) (FromSIUnit((Value), 1000))


#ifdef __cplusplus
}
#endif


#endif

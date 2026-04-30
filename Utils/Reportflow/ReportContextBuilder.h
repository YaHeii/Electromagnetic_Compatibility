#pragma once

#include <QJsonObject>

#include "Interface/SimulationResult.h"

class ReportContextBuilder {
public:
    static QJsonObject build(const SimulationTaskResult& result);
};

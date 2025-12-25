#include "../../include/models/DataModel.h"

DataModel* DataModel::instance()
{
    static DataModel inst;
    return &inst;
}

DataModel::DataModel(QObject *parent) : QObject(parent)
{
}

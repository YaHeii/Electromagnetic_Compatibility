//
// Created by lenovo on 25-10-6.
//

#include "../../include/models/DataModel.h"

DataModel* DataModel::instance()
{
    static DataModel inst;
    return &inst;
}

DataModel::DataModel(QObject *parent) : QObject(parent)
{
}

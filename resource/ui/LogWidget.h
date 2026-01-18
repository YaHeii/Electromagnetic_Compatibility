#pragma once

#include <QWidget>
#include <vector>
#include <memory>
#include <QString>
#include "spdlog/spdlog.h"
#include "../../include/utils/QtSpdlogSink.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LogWidget; }
QT_END_NAMESPACE

class LogWidget : public QWidget {
  Q_OBJECT
public:
  explicit LogWidget(QWidget* parent = nullptr);
  ~LogWidget();
  // 创建并返回一个指向UI日志接收器的指针
  std::shared_ptr<spdlog::sinks::sink> createGuiLogSink();

private:
  LogEmitter* _logEmitter; // 日志发射器
  Ui::LogWidget* ui;

public slots:
  void onLogReceived(const QString& message, int level);
}; 

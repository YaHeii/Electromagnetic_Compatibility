#include "Resource/ui/SimulationPreviewCard.h"

#include <QPainter>

SimulationPreviewCard::SimulationPreviewCard(QWidget* parent)
    : ElaInteractiveCard(parent) {
    setFixedSize(360, 128);
    setCardPixmapSize(148, 96);
    setCardPixMode(ElaCardPixType::PixMode::RoundedRect);
    setTitlePixelSize(16);
    setSubTitlePixelSize(12);
    setTitleSpacing(4);
}

bool SimulationPreviewCard::isSelected() const {
    return _selected;
}

void SimulationPreviewCard::setSelected(bool selected) {
    if (_selected == selected) {
        return;
    }
    _selected = selected;
    update();
}

void SimulationPreviewCard::paintEvent(QPaintEvent* event) {
    ElaInteractiveCard::paintEvent(event);

    Q_UNUSED(event);

    if (!_selected) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#1f6feb"), 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 8, 8);
}

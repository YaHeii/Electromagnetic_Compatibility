#pragma once

#include "ElaInteractiveCard.h"

class SimulationPreviewCard : public ElaInteractiveCard {
    Q_OBJECT

public:
    explicit SimulationPreviewCard(QWidget* parent = nullptr);

    bool isSelected() const;
    void setSelected(bool selected);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool _selected{false};
};

#include "SettingsDialogs.h"

#include <actions/GroupAction.h>

#include <QHBoxLayout>

using namespace mv::gui;

ConversionDialog::ConversionDialog(QWidget* parent, const QString& transformName, ToggleAction* sameChannelSettingAction, DecimalAction* singleDecimalSettingAction, SlidersAction* channelWiseDecimalAction) :
    QDialog(parent), _conversionButton(this, "Convert")
{
    setWindowTitle("Settings: " + transformName);

    connect(&_conversionButton, &TriggerAction::triggered, this, &ConversionDialog::closeDialogAction);

    auto* layout = new QHBoxLayout();

    auto groupAction = new GroupAction(this, "PointDataConversionGroupAction");

    groupAction->setText("Settings");
    groupAction->setToolTip("Data conversion settings");
    groupAction->setLabelSizingType(GroupAction::LabelSizingType::Auto);
    groupAction->addAction(sameChannelSettingAction);
    groupAction->addAction(singleDecimalSettingAction);
    groupAction->addAction(channelWiseDecimalAction);
    groupAction->addAction(&_conversionButton);

    layout->addWidget(groupAction->createWidget(this));
    setLayout(layout);
}

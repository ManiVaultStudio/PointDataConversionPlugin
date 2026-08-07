#pragma once

#include "SlidersAction.h"

#include <actions/DecimalAction.h>
#include <actions/ToggleAction.h>
#include <actions/TriggerAction.h>

#include <QDialog>

/**
 * Helper dialog to set conversion options
 *
 * @author Alex Vieth
 */
class ConversionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConversionDialog(QWidget* parent, mv::gui::ToggleAction* sameFactorAction, mv::gui::DecimalAction* arcSinFactorAction, mv::gui::SlidersAction* arcSinFactorsAction);

signals:
    void closeDialog(bool onlyIndices);

private slots:
    void closeDialogAction() {
        emit QDialog::accept();
    }

private:
    mv::gui::TriggerAction _conversionButton;
};

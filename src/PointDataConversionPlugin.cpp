#include "PointDataConversionPlugin.h"

#include <PointData/PointData.h>

#include <actions/PluginTriggerAction.h>

#include <atomic>
#include <cassert>
#include <cmath>

#include <QDebug>

Q_PLUGIN_METADATA(IID "studio.manivault.PointDataConversionPlugin")

using namespace mv;
using namespace mv::gui;

// =============================================================================
// PointDataConversionPlugin
// =============================================================================

const QMap<PointDataConversionPlugin::Conversion, QString> PointDataConversionPlugin::CONVERSIONS = QMap<Conversion, QString>({
    { Conversion::Log2, "Log2" },
    { Conversion::ArcSin, "Arcsin" }
});

PointDataConversionPlugin::PointDataConversionPlugin(const mv::plugin::PluginFactory* factory) :
    TransformationPlugin(factory)
{
}

void PointDataConversionPlugin::transform()
{
    auto points = getInputDataset<Points>();

    if (!points.isValid())
        return;

    QApplication::processEvents();
        
    auto& task = points->getTask();
        
    task.setName("Converting");
    task.setRunning();
    task.setProgressDescription(QString("%1 conversion").arg(getConversionName(_conversion)));
    
    points->visitData([this, &points, &task](auto pointData) {
        std::atomic_uint64_t noPointsProcessed = 0;
    
        const auto numPointsF   = static_cast<float>(points->getNumPoints());
        const auto numPointsI   = static_cast<std::int64_t>(points->getNumPoints());
        const auto numDims      = points->getNumDimensions();

        assert(!_cofactors.empty());
        assert(_cofactors.size() == 1 || _cofactors.size() == numDims);

        if (_cofactors.size() == 1)
            qDebug() << "PointDataConversionPlugin::transform: cofactor of" << _cofactors[0];
        else
            qDebug() << "PointDataConversionPlugin::transform: cofactors of" << _cofactors;

#pragma omp parallel for
        for (std::int64_t pointIndex = 0; pointIndex < numPointsI; pointIndex++) {

            for (std::uint64_t dimensionIndex = 0; dimensionIndex < numDims; dimensionIndex++) {
                switch (_conversion)
                {
                case Conversion::Log2:
                    pointData[pointIndex][dimensionIndex] = std::log2f(pointData[pointIndex][dimensionIndex] + 1.0f);
                    break;

                case Conversion::ArcSin:

                    const float cofactor = (_cofactors.size() == 1)  ? _cofactors[0] : _cofactors[dimensionIndex];

                    pointData[pointIndex][dimensionIndex] = std::asinhf(pointData[pointIndex][dimensionIndex] / cofactor);
                    break;
                }
            }

            if (const auto processed = ++noPointsProcessed;
                processed % 1000 == 0) {
#pragma omp critical
                {
                    task.setProgress(static_cast<float>(noPointsProcessed) / numPointsF);
                    QApplication::processEvents();
                }
            }
        }

    });
        
    task.setProgress(1.0f);
    task.setFinished();
        
    events().notifyDatasetDataChanged(points);
}

PointDataConversionPlugin::Conversion PointDataConversionPlugin::getConversion() const
{
    return _conversion;
}

void PointDataConversionPlugin::setConversion(const Conversion& conversion)
{
    if (conversion == _conversion)
        return;

    _conversion = conversion;
}

QString PointDataConversionPlugin::getConversionName(const Conversion& conversion)
{
    return CONVERSIONS[conversion];
}

// =============================================================================
// PointDataConversionPluginFactory
// =============================================================================

PointDataConversionPluginFactory::PointDataConversionPluginFactory() :
    _sameFactorAction(this, "Same factor", true),
    _arcSinFactorAction(this, "Factor",
        SlidersAction::EntryData::DEFAULT_MIN, SlidersAction::EntryData::DEFAULT_MAX,
        SlidersAction::EntryData::DEFAULT_VALUE, SlidersAction::EntryData::DEFAULT_DECIMALS),
    _arcSinFactorsAction(this, "Factors")
{
    connect(&_sameFactorAction, &ToggleAction::toggled, this, [&](bool toggled)
    {
        _arcSinFactorAction.setEnabled(_sameFactorAction.isChecked());
        _arcSinFactorsAction.setAllSlidersEnabled(!_sameFactorAction.isChecked());
    });
}

PointDataConversionPlugin* PointDataConversionPluginFactory::produce()
{
    return new PointDataConversionPlugin(this);
}

std::vector<float> PointDataConversionPluginFactory::getArcSinCoFactor() const
{
    if (_sameFactorAction.isChecked())
        return { _arcSinFactorAction.getValue() };

    return _arcSinFactorsAction.getValues();
}

PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    if (datasets.count() >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        const auto addPluginTriggerAction = [this, &pluginTriggerActions, datasets](const PointDataConversionPlugin::Conversion& type) -> void {
            const auto typeName = PointDataConversionPlugin::getConversionName(type);

            auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), icon(), [this, datasets, type](PluginTriggerAction& pluginTriggerAction) -> void {
                for (const auto& dataset : datasets) {
                    const_cast<PointDataConversionPluginFactory*>(this)->openConfigDialog(type, dataset);
                }
                });

            pluginTriggerActions << pluginTriggerAction;
            };

        addPluginTriggerAction(PointDataConversionPlugin::Conversion::Log2);
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::ArcSin);
    }

    return pluginTriggerActions;
}

// This is used in e.g. the image viewer
PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const mv::DataTypes& dataTypes) const
{
    PluginTriggerActions pluginTriggerActions;

    if (dataTypes.count(PointType) == dataTypes.count()) {
        const auto addPluginTriggerAction = [this, &pluginTriggerActions](const PointDataConversionPlugin::Conversion& type) -> void {
            const auto typeName = PointDataConversionPlugin::getConversionName(type);

            auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), icon(), [this, type](PluginTriggerAction& pluginTriggerAction) -> void {
                for (const auto& dataset : pluginTriggerAction.getDatasets()) {
                    createPluginAndTransform(type, dataset);
                }
            });

            pluginTriggerAction->setConfigurationAction(const_cast<PointDataConversionPluginFactory*>(this)->getConfigurationAction(type));

            pluginTriggerActions << pluginTriggerAction;
        };

        addPluginTriggerAction(PointDataConversionPlugin::Conversion::Log2);
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::ArcSin);
    }

    return pluginTriggerActions;
}

WidgetAction* PointDataConversionPluginFactory::getConfigurationAction(const PointDataConversionPlugin::Conversion& type)
{
    const auto createGroupAction = [this]() -> GroupAction* {

        _arcSinFactorsAction.initialize({});
        _sameFactorAction.setChecked(true);

        auto groupAction = new GroupAction(this, "PointDataConversionGroupAction");

        groupAction->setText("Settings");
        groupAction->setToolTip("Data conversion settings");
        groupAction->setLabelSizingType(GroupAction::LabelSizingType::Auto);
        groupAction->addAction(&_arcSinFactorAction);

        return groupAction;
    };

    switch (type)
    {
        case PointDataConversionPlugin::Conversion::Log2:
            return nullptr;

        case PointDataConversionPlugin::Conversion::ArcSin:
            return createGroupAction();
    }

    return nullptr;
}


void PointDataConversionPluginFactory::openConfigDialog(const PointDataConversionPlugin::Conversion& type, const mv::Dataset<mv::DatasetImpl>& inputDataset)
{
    _sameFactorAction.setChecked(true);
    const std::vector<QString> dimNamesVec = mv::Dataset<Points>(inputDataset)->getDimensionNames();
    const QStringList dimNamesList(dimNamesVec.begin(), dimNamesVec.end());
    _arcSinFactorsAction.initialize(dimNamesList);

    switch (type)
    {
    case PointDataConversionPlugin::Conversion::Log2:
        createPluginAndTransform(type, inputDataset);
        break;

    case PointDataConversionPlugin::Conversion::ArcSin:
    {
        ConversionDialog inputDialog(nullptr, &_sameFactorAction, &_arcSinFactorAction, &_arcSinFactorsAction);
        inputDialog.setModal(true);
        if (inputDialog.exec() == QDialog::Accepted)
            createPluginAndTransform(type, inputDataset);

        break;
    }
    }

}

void PointDataConversionPluginFactory::createPluginAndTransform(const PointDataConversionPlugin::Conversion& type, const mv::Dataset<mv::DatasetImpl>& inputDataset) const
{
    auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

    pluginInstance->setInputDataset(inputDataset);
    pluginInstance->setConversion(type);
    pluginInstance->setCofactor(getArcSinCoFactor());
    pluginInstance->transform();

}

// =============================================================================
// Helper
// =============================================================================

ConversionDialog::ConversionDialog(QWidget* parent, ToggleAction* sameFactorAction, DecimalAction* arcSinFactorAction, SlidersAction* arcSinFactorsAction) :
    QDialog(parent), _conversionButton(this, "Convert")
{
    setWindowTitle(tr("Data conversion settings"));

    connect(&_conversionButton, &TriggerAction::triggered, this, &ConversionDialog::closeDialogAction);

    auto* layout = new QHBoxLayout();

    auto groupAction = new GroupAction(this, "PointDataConversionGroupAction");

    groupAction->setText("Settings");
    groupAction->setToolTip("Data conversion settings");
    groupAction->setLabelSizingType(GroupAction::LabelSizingType::Auto);
    groupAction->addAction(sameFactorAction);
    groupAction->addAction(arcSinFactorAction);
    groupAction->addAction(arcSinFactorsAction);
    groupAction->addAction(&_conversionButton);

    layout->addWidget(groupAction->createWidget(this));
    setLayout(layout);
}

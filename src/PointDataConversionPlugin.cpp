#include "PointDataConversionPlugin.h"

#include <PointData/PointData.h>

#include <actions/PluginTriggerAction.h>

#include <atomic>
#include <cmath>

#include <QDebug>

Q_PLUGIN_METADATA(IID "studio.manivault.PointDataConversionPlugin")

using namespace mv;
using namespace mv::gui;

const QMap<PointDataConversionPlugin::Conversion, QString> PointDataConversionPlugin::CONVERSIONS = QMap<Conversion, QString>({
    { Conversion::Log2, "Log2" },
    { Conversion::ArcSin, "Arcsin" }
});

PointDataConversionPlugin::PointDataConversionPlugin(const mv::plugin::PluginFactory* factory) :
    TransformationPlugin(factory),
    _conversion(Conversion::ArcSin),
    _cofactors({5.f})
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
    
        float cofactor          = _cofactors[0];
        const auto numPointsF   = static_cast<float>(points->getNumPoints());
        const auto numPointsI   = static_cast<std::int64_t>(points->getNumPoints());
        const auto numDims      = points->getNumDimensions();

#pragma omp parallel for
        for (std::int64_t pointIndex = 0; pointIndex < numPointsI; pointIndex++) {

            for (std::uint64_t dimensionIndex = 0; dimensionIndex < numDims; dimensionIndex++) {
                switch (_conversion)
                {
                case Conversion::Log2:
                    pointData[pointIndex][dimensionIndex] = std::log2f(pointData[pointIndex][dimensionIndex] + 1.0f);
                    break;

                case Conversion::ArcSin:
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

PointDataConversionPluginFactory::PointDataConversionPluginFactory() :
    _arcSinFactorAction(this, "Factor", 1.0f, 100.0f, 5.0f, 5.0f)
{
}

PointDataConversionPlugin* PointDataConversionPluginFactory::produce()
{
    return new PointDataConversionPlugin(this);
}

std::vector<float> PointDataConversionPluginFactory::getArcSinCoFactor() const
{
    return { _arcSinFactorAction.getValue() };
}

// TODO: add gui to optionally set per-channel cofactor
PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    if (datasets.count() >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        const auto addPluginTriggerAction = [this, &pluginTriggerActions, datasets](const PointDataConversionPlugin::Conversion& type) -> void {
            const auto typeName = PointDataConversionPlugin::getConversionName(type);

            auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), icon(), [this, datasets, type](PluginTriggerAction& pluginTriggerAction) -> void {
                for (const auto& dataset : datasets) {
                    auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

                    pluginInstance->setInputDataset(dataset);
                    pluginInstance->setConversion(type);
                    pluginInstance->transform();
                }
                });

            pluginTriggerActions << pluginTriggerAction;
            };

        addPluginTriggerAction(PointDataConversionPlugin::Conversion::Log2);
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::ArcSin);
    }

    return pluginTriggerActions;
}

// This is used in the image viewer
PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const mv::DataTypes& dataTypes) const
{
    PluginTriggerActions pluginTriggerActions;

    if (dataTypes.count(PointType) == dataTypes.count()) {
        const auto addPluginTriggerAction = [this, &pluginTriggerActions](const PointDataConversionPlugin::Conversion& type) -> void {
            const auto typeName = PointDataConversionPlugin::getConversionName(type);

            auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), icon(), [this, type](PluginTriggerAction& pluginTriggerAction) -> void {
                for (const auto& dataset : pluginTriggerAction.getDatasets()) {
                    auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

                    pluginInstance->setInputDataset(dataset);
                    pluginInstance->setConversion(type);
                    pluginInstance->setCofactor(getArcSinCoFactor());

                    pluginInstance->transform();
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
    const auto createGroupAction = [this](WidgetAction& widgetAction) -> GroupAction* {
        auto groupAction = new GroupAction(this, "PointDataConversionGroupAction");

        groupAction->setText("Settings");
        groupAction->setToolTip("Data conversion settings");
        groupAction->setLabelSizingType(GroupAction::LabelSizingType::Auto);
        groupAction->addAction(&widgetAction);

        return groupAction;
    };

    switch (type)
    {
        case PointDataConversionPlugin::Conversion::Log2:
            return nullptr;

        case PointDataConversionPlugin::Conversion::ArcSin:
            return createGroupAction(_arcSinFactorAction);
    }

    return nullptr;
}

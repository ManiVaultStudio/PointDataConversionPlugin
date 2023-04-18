#include "PointDataConversionPlugin.h"

#include <PointData/PointData.h>

#include <actions/PluginTriggerAction.h>

#include <cmath>

#include <QDebug>

Q_PLUGIN_METADATA(IID "nl.BioVault.PointDataConversionPlugin")

using namespace hdps;

const QMap<PointDataConversionPlugin::Type, QString> PointDataConversionPlugin::types = QMap<PointDataConversionPlugin::Type, QString>({
    { PointDataConversionPlugin::Type::Log2, "Log2" },
    { PointDataConversionPlugin::Type::ArcSin, "Arcsin" }
});

PointDataConversionPlugin::PointDataConversionPlugin(const PluginFactory* factory) :
    TransformationPlugin(factory),
    _type(Type::ArcSin)
{
}

void PointDataConversionPlugin::transform()
{
    for (auto inputDataset : getInputDatasets()) {
        auto points = Dataset<Points>(inputDataset);

        if (!points.isValid())
            continue;

        points->setLocked(true);
        {
            QApplication::processEvents();

            points->getDataHierarchyItem().setTaskName("Converting");
            points->getDataHierarchyItem().setTaskRunning();
            points->getDataHierarchyItem().setTaskDescription(QString("%1 conversion").arg(getTypeName(_type)));

            points->visitData([this, &points](auto pointData) {
                std::uint32_t noPointsProcessed = 0;

                for (auto point : pointData) {
                    for (std::int32_t dimensionIndex = 0; dimensionIndex < points->getNumDimensions(); dimensionIndex++) {
                        switch (_type)
                        {
                            case PointDataConversionPlugin::Type::Log2:
                                point[dimensionIndex] = log2(point[dimensionIndex] + 1);
                                break;

                            case PointDataConversionPlugin::Type::ArcSin:
                                point[dimensionIndex] = asinh(point[dimensionIndex] / 5.0f);
                                break;

                            default:
                                break;
                        }
                    }

                    ++noPointsProcessed;

                    if (noPointsProcessed % 1000 == 0) {
                        points->getDataHierarchyItem().setTaskProgress(static_cast<float>(noPointsProcessed) / static_cast<float>(points->getNumPoints()));
                        
                        QApplication::processEvents();
                    }
                }
            });

            points->getDataHierarchyItem().setTaskProgress(1.0f);
            points->getDataHierarchyItem().setTaskFinished();

            events().notifyDatasetChanged(points);
        }
        points->setLocked(false);
    }
}

PointDataConversionPlugin::Type PointDataConversionPlugin::getType() const
{
    return _type;
}

void PointDataConversionPlugin::setType(const Type& type)
{
    if (type == _type)
        return;

    _type = type;
}

QString PointDataConversionPlugin::getTypeName(const Type& type)
{
    return types[type];
}

PointDataConversionPluginFactory::PointDataConversionPluginFactory() :
    _arcSinFactorAction(this, "Factor", 1.0f, 100.0f, 5.0f, 5.0f, 1)
{
}

PointDataConversionPlugin* PointDataConversionPluginFactory::produce()
{
    return new PointDataConversionPlugin(this);
}

PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const hdps::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto numberOfDatasets = datasets.count();

    if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        if (numberOfDatasets >= 1 && datasets.first()->getDataType() == PointType) {
            const auto addPluginTriggerAction = [this, &pluginTriggerActions, datasets](const PointDataConversionPlugin::Type& type) -> void {
                const auto typeName = PointDataConversionPlugin::getTypeName(type);

                auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), getIcon(), [this, datasets, type](PluginTriggerAction& pluginTriggerAction) -> void {
                    for (auto dataset : datasets) {
                        auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

                        pluginInstance->setInputDatasets(datasets);
                        pluginInstance->setType(type);
                        pluginInstance->transform();
                    }
                });

                pluginTriggerActions << pluginTriggerAction;
            };

            addPluginTriggerAction(PointDataConversionPlugin::Type::Log2);
            addPluginTriggerAction(PointDataConversionPlugin::Type::ArcSin);
        }
    }

    return pluginTriggerActions;
}

PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const hdps::DataTypes& dataTypes) const
{
    PluginTriggerActions pluginTriggerActions;

    if (dataTypes.count(PointType) == dataTypes.count()) {
        const auto addPluginTriggerAction = [this, &pluginTriggerActions](const PointDataConversionPlugin::Type& type) -> void {
            const auto typeName = PointDataConversionPlugin::getTypeName(type);

            auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), getIcon(), [this, type](PluginTriggerAction& pluginTriggerAction) -> void {
                for (auto dataset : Datasets()) {
                    auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

                    pluginInstance->setInputDatasets(Datasets());
                    pluginInstance->setType(type);
                    pluginInstance->transform();
                }
            });

            pluginTriggerAction->setConfigurationAction(const_cast<PointDataConversionPluginFactory*>(this)->getConfigurationAction(type));

            pluginTriggerActions << pluginTriggerAction;
        };

        addPluginTriggerAction(PointDataConversionPlugin::Type::Log2);
        addPluginTriggerAction(PointDataConversionPlugin::Type::ArcSin);
    }

    return pluginTriggerActions;
}

WidgetAction* PointDataConversionPluginFactory::getConfigurationAction(const PointDataConversionPlugin::Type& type)
{
    const auto createGroupAction = [this](const WidgetActions& widgetActions) -> GroupAction* {
        auto groupAction = new GroupAction(this);

        groupAction->setText("Settings");
        groupAction->setToolTip("Data conversion settings");
        groupAction->setLabelSizingType(GroupAction::LabelSizingType::Auto);
        groupAction->setActions(widgetActions);

        return groupAction;
    };

    switch (type)
    {
        case PointDataConversionPlugin::Type::Log2:
            return nullptr;

        case PointDataConversionPlugin::Type::ArcSin:
            return createGroupAction({ &_arcSinFactorAction });

        default:
            break;
    }

    return nullptr;
}

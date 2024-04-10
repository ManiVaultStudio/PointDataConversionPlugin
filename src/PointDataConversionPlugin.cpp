#include "PointDataConversionPlugin.h"

#include <PointData/PointData.h>

#include <actions/PluginTriggerAction.h>

#include <cmath>

#include <QDebug>

Q_PLUGIN_METADATA(IID "studio.manivault.PointDataConversionPlugin")

using namespace mv;

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
    auto points = getInputDataset<Points>();

    if (!points.isValid())
        return;

    QApplication::processEvents();
        
    auto& task = points->getTask();
        
    task.setName("Converting");
    task.setRunning();
    task.setProgressDescription(QString("%1 conversion").arg(getTypeName(_type)));
    
    qDebug() << "PointDataConversionPlugin:: Apply " << getTypeName(_type) << " conversion to " << points->getGuiName();

    points->visitData([this, &points, &task](auto pointData) {
        std::uint32_t noPointsProcessed = 0;
        
        for (auto point : pointData) {
            for (std::int32_t dimensionIndex = 0; dimensionIndex < points->getNumDimensions(); dimensionIndex++) {
                switch (_type)
                {
                    case PointDataConversionPlugin::Type::Log2:
                        point[dimensionIndex] = std::log2(point[dimensionIndex] + 1);
                        break;
        
                    case PointDataConversionPlugin::Type::ArcSin:
                        point[dimensionIndex] = std::asinh(point[dimensionIndex] / 5.0f);
                        break;
        
                    default:
                        break;
                }
            }
        
            ++noPointsProcessed;
        
            if (noPointsProcessed % 1000 == 0) {
                task.setProgress(static_cast<float>(noPointsProcessed) / static_cast<float>(points->getNumPoints()));
                    
                QApplication::processEvents();
            }
        }
    });
        
    task.setProgress(1.0f);
    task.setFinished();
        
    events().notifyDatasetDataChanged(points);
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
    _arcSinFactorAction(this, "Factor", 1.0f, 100.0f, 5.0f, 5.0f)
{
}

PointDataConversionPlugin* PointDataConversionPluginFactory::produce()
{
    return new PointDataConversionPlugin(this);
}

PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto numberOfDatasets = datasets.count();

    if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        if (numberOfDatasets >= 1 && datasets.first()->getDataType() == PointType) {
            const auto addPluginTriggerAction = [this, &pluginTriggerActions, datasets](const PointDataConversionPlugin::Type& type) -> void {
                const auto typeName = PointDataConversionPlugin::getTypeName(type);

                auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), getIcon(), [this, datasets, type](PluginTriggerAction& pluginTriggerAction) -> void {
                    for (const auto& dataset : datasets) {
                        auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

                        pluginInstance->setInputDataset(dataset);
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

PluginTriggerActions PointDataConversionPluginFactory::getPluginTriggerActions(const mv::DataTypes& dataTypes) const
{
    PluginTriggerActions pluginTriggerActions;

    if (dataTypes.count(PointType) == dataTypes.count()) {
        const auto addPluginTriggerAction = [this, &pluginTriggerActions](const PointDataConversionPlugin::Type& type) -> void {
            const auto typeName = PointDataConversionPlugin::getTypeName(type);

            auto pluginTriggerAction = new PluginTriggerAction(const_cast<PointDataConversionPluginFactory*>(this), this, QString("Conversion/%1").arg(typeName), QString("Perform %1 data conversion").arg(typeName), getIcon(), [this, type](PluginTriggerAction& pluginTriggerAction) -> void {
                for (const auto& dataset : pluginTriggerAction.getDatasets()) {
                    auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

                    pluginInstance->setInputDataset(dataset);
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
        case PointDataConversionPlugin::Type::Log2:
            return nullptr;

        case PointDataConversionPlugin::Type::ArcSin:
            return createGroupAction(_arcSinFactorAction);

        default:
            break;
    }

    return nullptr;
}

#include "PointDataConversionPlugin.h"

#include "SettingsDialogs.h"

#include <PointData/PointData.h>

#include <actions/PluginTriggerAction.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <limits>

#include <QDebug>

Q_PLUGIN_METADATA(IID "studio.manivault.PointDataConversionPlugin")

using namespace mv;
using namespace mv::gui;

// =============================================================================
// PointDataConversionPlugin
// =============================================================================

const QMap<PointDataConversionPlugin::Conversion, QString> PointDataConversionPlugin::CONVERSIONS = QMap<Conversion, QString>({
    { Conversion::Log2, "Log2" },
    { Conversion::Log1p, "Log1p" },
    { Conversion::ArcSinh, "Arcsinh" },
    { Conversion::ClampMax, "Clamp (max)" }
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

        assert(!_conversionSetting.empty());
        assert(_conversionSetting.size() == 1 || _conversionSetting.size() == numDims);

        std::vector<float> dimMax;

        // Some conversions require a preparation step
        switch (_conversion)
        {
        case Conversion::Log2: break;
        case Conversion::Log1p: break;
        case Conversion::ArcSinh:  
            
            if (_conversionSetting.size() == 1)
                qDebug() << "PointDataConversionPlugin::transform: cofactor of" << _conversionSetting[0];
            else
                qDebug() << "PointDataConversionPlugin::transform: cofactors of" << _conversionSetting;

            break;

        case Conversion::ClampMax:

            dimMax.resize(numDims, std::numeric_limits<float>::lowest());

            for (std::int64_t pointIndex = 0; pointIndex < numPointsI; pointIndex++) {
                auto point = pointData[pointIndex];
                for (std::uint64_t dimensionIndex = 0; dimensionIndex < numDims; dimensionIndex++) {
                    dimMax[dimensionIndex] = std::max(dimMax[dimensionIndex], static_cast<float>(point[dimensionIndex]));
                }
            }

            for (std::uint64_t dimensionIndex = 0; dimensionIndex < numDims; dimensionIndex++) {
                const float percentile = ((_conversionSetting.size() == 1) ? _conversionSetting[0] : _conversionSetting[dimensionIndex]) * 0.01f; // setting is in [1, 100] but we want %
                dimMax[dimensionIndex] = percentile * dimMax[dimensionIndex];
            }

        }


        // Convert each point
#pragma omp parallel for
        for (std::int64_t pointIndex = 0; pointIndex < numPointsI; pointIndex++) {
            auto point = pointData[pointIndex];
            for (std::uint64_t dimensionIndex = 0; dimensionIndex < numDims; dimensionIndex++) {
                switch (_conversion)
                {
                case Conversion::Log2:
                    point[dimensionIndex] = std::log2f(point[dimensionIndex] + 1.0f);
                    break;

                case Conversion::Log1p:
                    // more precise than the expression std::log(1 + num) if num is close to zero,
                    // see https://en.cppreference.com/cpp/numeric/math/log1p 
                    point[dimensionIndex] = std::log1pf(point[dimensionIndex]);
                    break;

                case Conversion::ArcSinh:
                {
                    const float cofactor = (_conversionSetting.size() == 1)  ? _conversionSetting[0] : _conversionSetting[dimensionIndex];
                    point[dimensionIndex] = std::asinhf(point[dimensionIndex] / cofactor);
                    break;
                }

                case Conversion::ClampMax:
                    point[dimensionIndex] = std::min(static_cast<float>(point[dimensionIndex]), dimMax[dimensionIndex]);
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
    _sameChannelSettingAction(this, "Same factor", true),
    _singleDecimalSettingAction(this, "Factor"),
    _channelWiseDecimalAction(this, "Factors")
{
    _singleDecimalSettingAction.setToolTip("Apply the same cofactors to each channel.");
    _channelWiseDecimalAction.setToolTip("Apply different cofactors to each channel.");

    _singleDecimalSettingAction.initialize(SlidersAction::EntryData::DEFAULT_MIN, SlidersAction::EntryData::DEFAULT_MAX,
        SlidersAction::EntryData::DEFAULT_VALUE, SlidersAction::EntryData::DEFAULT_DECIMALS);

    connect(&_sameChannelSettingAction, &ToggleAction::toggled, this, [&](bool toggled)
        {
            _singleDecimalSettingAction.setEnabled(_sameChannelSettingAction.isChecked());
            _channelWiseDecimalAction.setAllSlidersEnabled(!_sameChannelSettingAction.isChecked());
        });

    connect(&_singleDecimalSettingAction, &DecimalAction::valueChanged, this, [&](float value)
        {
            const auto sliderValues = _channelWiseDecimalAction.getValues();
            
            const bool allEqual = !sliderValues.empty() &&
            std::all_of(sliderValues.cbegin() + 1, sliderValues.cend(),
                [&](const float v) { return std::abs(v - sliderValues.front()) < 0.0001f; });

            if (!allEqual)
                return;

            _channelWiseDecimalAction.setValueForAllEntries(value);
        });

}

PointDataConversionPlugin* PointDataConversionPluginFactory::produce()
{
    return new PointDataConversionPlugin(this);
}

std::vector<float> PointDataConversionPluginFactory::getConversionSetting() const
{
    if (_sameChannelSettingAction.isChecked())
        return { _singleDecimalSettingAction.getValue() };

    return _channelWiseDecimalAction.getValues();
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
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::Log1p);
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::ArcSinh);
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::ClampMax);
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
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::Log1p);
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::ArcSinh);
        addPluginTriggerAction(PointDataConversionPlugin::Conversion::ClampMax);
    }

    return pluginTriggerActions;
}

WidgetAction* PointDataConversionPluginFactory::getConfigurationAction(const PointDataConversionPlugin::Conversion& type)
{
    const auto createGroupAction = [this]() -> GroupAction* {

        _channelWiseDecimalAction.initialize({});
        _sameChannelSettingAction.setChecked(true);

        auto groupAction = new GroupAction(this, "PointDataConversionGroupAction");

        groupAction->setText("Settings");
        groupAction->setToolTip("Data conversion settings");
        groupAction->setLabelSizingType(GroupAction::LabelSizingType::Auto);
        groupAction->addAction(&_singleDecimalSettingAction);

        return groupAction;
    };

    WidgetAction* configAction = nullptr;
    setConfigDialogDefaultSettings(type);

    switch (type)
    {
        case PointDataConversionPlugin::Conversion::Log2:
            break;
        case PointDataConversionPlugin::Conversion::Log1p:
            break;

        case PointDataConversionPlugin::Conversion::ArcSinh:
        {
            configAction = createGroupAction();
            break;
        }

        case PointDataConversionPlugin::Conversion::ClampMax:
        {
            configAction = createGroupAction();
            break;
        }
    }

    return configAction;
}

void PointDataConversionPluginFactory::openConfigDialog(const PointDataConversionPlugin::Conversion& type, const mv::Dataset<mv::DatasetImpl>& inputDataset)
{
    _sameChannelSettingAction.setChecked(true);
    const std::vector<QString> dimNamesVec = mv::Dataset<Points>(inputDataset)->getDimensionNames();
    const QStringList dimNamesList(dimNamesVec.begin(), dimNamesVec.end());
    _channelWiseDecimalAction.initialize(dimNamesList);

    setConfigDialogDefaultSettings(type);

    switch (type)
    {
    case PointDataConversionPlugin::Conversion::Log2:
        createPluginAndTransform(type, inputDataset);
        break;
    case PointDataConversionPlugin::Conversion::Log1p:
        createPluginAndTransform(type, inputDataset);
        break;

    case PointDataConversionPlugin::Conversion::ArcSinh:
    {
        ConversionDialog inputDialog(nullptr, PointDataConversionPlugin::CONVERSIONS[PointDataConversionPlugin::Conversion::ArcSinh],
            &_sameChannelSettingAction, &_singleDecimalSettingAction, &_channelWiseDecimalAction);
        inputDialog.setModal(true);
        if (inputDialog.exec() == QDialog::Accepted)
            createPluginAndTransform(type, inputDataset);

        break;
    }
    case PointDataConversionPlugin::Conversion::ClampMax:
    {
        ConversionDialog inputDialog(nullptr, PointDataConversionPlugin::CONVERSIONS[PointDataConversionPlugin::Conversion::ClampMax],
            &_sameChannelSettingAction, &_singleDecimalSettingAction, &_channelWiseDecimalAction);
        inputDialog.setModal(true);
        if (inputDialog.exec() == QDialog::Accepted)
            createPluginAndTransform(type, inputDataset);

        break;
    }
    }

}

void PointDataConversionPluginFactory::setConfigDialogDefaultSettings(const PointDataConversionPlugin::Conversion& type)
{
    switch (type)
    {
    case PointDataConversionPlugin::Conversion::Log2:
        break;
    case PointDataConversionPlugin::Conversion::Log1p:
        break;

    case PointDataConversionPlugin::Conversion::ArcSinh:
    {
        _singleDecimalSettingAction.setText("Cofactor");
        _singleDecimalSettingAction.setValue(5.f);
        _channelWiseDecimalAction.setText("Cofactors");
        _channelWiseDecimalAction.setValueForAllEntries(99.f);
        break;
    }
    case PointDataConversionPlugin::Conversion::ClampMax:
    {
        _singleDecimalSettingAction.setText("Percentile");
        _singleDecimalSettingAction.setValue(99.f);
        _channelWiseDecimalAction.setText("Percentiles");
        _channelWiseDecimalAction.setValueForAllEntries(99.f);
        break;
    }

    }
}

void PointDataConversionPluginFactory::createPluginAndTransform(const PointDataConversionPlugin::Conversion& type, const mv::Dataset<mv::DatasetImpl>& inputDataset) const
{
    auto pluginInstance = dynamic_cast<PointDataConversionPlugin*>(plugins().requestPlugin(getKind()));

    pluginInstance->setInputDataset(inputDataset);
    pluginInstance->setConversion(type);
    pluginInstance->setConversionSetting(getConversionSetting());
    pluginInstance->transform();

}

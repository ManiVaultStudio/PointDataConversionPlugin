#pragma once

#include <actions/DecimalAction.h>
#include <actions/ToggleAction.h>

#include <Dataset.h>
#include <TransformationPlugin.h>

#include "SlidersAction.h"

#include <QString>
#include <QStringList>
#include <vector>

/**
 * Point data conversion plugin class
 *
 * @author Thomas Kroes
 */
class PointDataConversionPlugin : public mv::plugin::TransformationPlugin
{
    Q_OBJECT

public:

    /** Point data conversion type */
    enum class Conversion {
        Log2,       /** log2(value+1) */
        ArcSinh,     /** asinh(value/factor), inverse hyperbolic sine */
        ClampMax,   /** clamp (max) value to a percentile of its respective dimension */
    };

    static const QMap<Conversion, QString> CONVERSIONS;

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    PointDataConversionPlugin(const mv::plugin::PluginFactory* factory);

    /** Destructor */
    ~PointDataConversionPlugin() override = default;
    
    /** Initialization is called when the plugin is first instantiated. */
    void init() override {}

    /** Performs the data transformation */
    void transform() override;

    /** Set conversion setting */
    void setConversionSetting(std::vector<float> cofactors) { _conversionSetting = std::move(cofactors); }

    /**
     * Get point data conversion type
     * @return Point data conversion type
     */
    Conversion getConversion() const;

    /**
     * Set point data conversion type
     * @param conversion Point data conversion type
     */
    void setConversion(const Conversion& conversion);

    /**
     * Get string representation of type enum
     * @param conversion Point data conversion type
     * @return conversion name
     */
    static QString getConversionName(const Conversion& conversion);

private:
    Conversion          _conversion = Conversion::ArcSinh;
    std::vector<float>  _conversionSetting = { 5.f };
};

/**
 * Point data conversion plugin factory class
 *
 * @author Thomas Kroes
 */
class PointDataConversionPluginFactory : public mv::plugin::TransformationPluginFactory
{
    Q_INTERFACES(mv::plugin::TransformationPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.PointDataConversionPlugin"
                      FILE  "PluginInfo.json")

public:

    /** Default constructor */
    PointDataConversionPluginFactory();

    /** Destructor */
    ~PointDataConversionPluginFactory() override {}
    
    /** Creates an instance of the point data conversion plugin */
    PointDataConversionPlugin* produce() override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;

    /**
     * Get plugin trigger actions given \p dataTypes
     * @param dataTypes Vector of input data types
     * @return Vector of plugin trigger actions
     */
    mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::DataTypes& dataTypes) const override;

    /**
     * Get configuration action for \p type
     * @return Pointer to configuration action (may be null)
     */
    WidgetAction* getConfigurationAction(const PointDataConversionPlugin::Conversion& type);

    /**
     * Show option dialog and run transform
     */
    void openConfigDialog(const PointDataConversionPlugin::Conversion& type, const mv::Dataset<mv::DatasetImpl>& inputDataset);

    /**
     * Create a transformation plugin and apply transformation
     */
    void createPluginAndTransform(const PointDataConversionPlugin::Conversion& type, const mv::Dataset<mv::DatasetImpl>& inputDataset) const;

private:
    std::vector<float> getConversionSetting() const;

    void setConfigDialogDefaultSettings(const PointDataConversionPlugin::Conversion& type);

private:
    mv::gui::ToggleAction  _sameChannelSettingAction;

    mv::gui::DecimalAction _singleDecimalSettingAction;
    mv::gui::SlidersAction _channelWiseDecimalAction;
};

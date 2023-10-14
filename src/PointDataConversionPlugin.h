#pragma once

#include <TransformationPlugin.h>

#include <Dataset.h>

using namespace mv::plugin;
using namespace mv::gui;
using namespace mv::util;

class QLabel;

/**
 * Point data conversion plugin class
 *
 * @author Thomas Kroes
 */
class PointDataConversionPlugin : public TransformationPlugin
{
    Q_OBJECT

public:

    /** Point data conversion type */
    enum class Type {
        Log2,       /** log2(value+1) */
        ArcSin      /** asinh(value/factor) */
    };

    static const QMap<Type, QString> types;

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    PointDataConversionPlugin(const PluginFactory* factory);

    /** Destructor */
    ~PointDataConversionPlugin() override = default;
    
    /** Initialization is called when the plugin is first instantiated. */
    void init() override {};

    /** Performs the data transformation */
    void transform() override;

    /**
    /**
     * Get point data conversion type
     * @return Point data conversion type
     */
    Type getType() const;

    /**
     * Set point data conversion type
     * @param type Point data conversion type
     */
    void setType(const Type& type);

    /**
     * Get string representation of type enum
     * @param type Point data conversion type
     * @return Type name
     */
    static QString getTypeName(const Type& type);

private:
    Type    _type;      /** Data conversion type */
};

/**
 * Point data conversion plugin factory class
 *
 * @author Thomas Kroes
 */
class PointDataConversionPluginFactory : public TransformationPluginFactory
{
    Q_INTERFACES(mv::plugin::TransformationPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.BioVault.PointDataConversionPlugin"
                      FILE  "PointDataConversionPlugin.json")

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
    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;

    /**
     * Get plugin trigger actions given \p dataTypes
     * @param datasetTypes Vector of input data types
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const mv::DataTypes& dataTypes) const override;

    /**
     * Get configuration action for \p type
     * @return Pointer to configuration action (may be null)
     */
    WidgetAction* getConfigurationAction(const PointDataConversionPlugin::Type& type);

private:
    DecimalAction   _arcSinFactorAction;    /** Factor for arcsin(value/factor) conversion */
};

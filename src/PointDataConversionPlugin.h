#pragma once

#include <TransformationPlugin.h>

#include <Dataset.h>

using namespace hdps::plugin;
using namespace hdps::gui;
using namespace hdps::util;

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
        Log2,       /** log2(1 + value) */
        ArcSin5     /** asinh(value / 5.0) */
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
    Q_INTERFACES(hdps::plugin::TransformationPluginFactory hdps::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.BioVault.PointDataConversionPlugin"
                      FILE  "PointDataConversionPlugin.json")

public:

    /** Default constructor */
    PointDataConversionPluginFactory() {}

    /** Destructor */
    ~PointDataConversionPluginFactory() override {}
    
    /** Creates an instance of the point data conversion plugin */
    PointDataConversionPlugin* produce() override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const hdps::Datasets& datasets) const override;

    /**
     * Get plugin trigger actions given \p dataTypes
     * @param datasetTypes Vector of input data types
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const hdps::DataTypes& dataTypes) const override;
};

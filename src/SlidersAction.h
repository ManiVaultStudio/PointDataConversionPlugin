#pragma once

#include <actions/WidgetAction.h>

#include <QString>
#include <QStringList>
#include <QWidget>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace mv::gui {

    class SlidersAction : public WidgetAction {
        Q_OBJECT

    public:
        /** Describes the widget flags */
        enum WidgetFlag {
            Default = 0x00001,
        };

    public:
        struct OptionData {
            static constexpr float DEFAULT_MIN = 1.0f;
            static constexpr float DEFAULT_MAX = 100.0f;
            static constexpr float DEFAULT_VALUE = 5.0f;
            static constexpr std::int32_t DEFAULT_DECIMALS = 2;

            float min = DEFAULT_MIN;
            float max = DEFAULT_MAX;
            float value = DEFAULT_VALUE;
        };

    public:
        explicit SlidersAction(QObject* parent, const QString& title);

        void initialize(const QStringList& options = QStringList());
        void setOptions(const QStringList& options);

        void setRangeForOption(const QString& option, float minimum, float maximum);
        void setValueForOption(const QString& option, float value);
        void setDataForOption(const QString& option, float value, float minimum, float maximum);
        [[nodiscard]] float getValueForOption(const QString& option) const;
        [[nodiscard]] std::vector<float> getValues() const;

    signals:
        void optionValueChanged(const QString& option, float value);

    protected:
        QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override;

    private:

        QStringList _options = {};
        std::unordered_map<QString, OptionData> _optionData = {};
    };
}

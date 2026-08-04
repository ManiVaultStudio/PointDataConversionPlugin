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
        explicit SlidersAction(QObject* parent, const QString& title);

        /** Describes the widget flags */
        enum WidgetFlag {
            Default = 0x00001,
        };

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
        struct OptionData {
            float min = 0.0f;
            float max = 1.0f;
            float value = 0.5f;
        };

        QStringList _options = {};
        std::unordered_map<QString, OptionData> _optionData = {};
    };
}

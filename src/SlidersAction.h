#pragma once

#include <actions/WidgetAction.h>

#include <QListWidget>
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
            DisableOnFirstOpen = 0x00002,
        };

    public:
        struct EntryData {
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

        void initialize(const QStringList& entries = QStringList());
        void setEntries(const QStringList& entries);
        void setAllEntriesToDefault();

        void setRangeForEntry(const QString& entry, float minimum, float maximum);
        void setValueForEntry(const QString& entry, float value);
        void setValueForAllEntries(float value); // also updates sliders
        void setDataForEntry(const QString& entry, float value, float minimum, float maximum);
        [[nodiscard]] float getValueForEntry(const QString& entry) const;
        [[nodiscard]] std::vector<float> getValues() const;

        void setAllSlidersEnabled(bool enabled);
        void setAllSlidersValues(float value);

    signals:
        void entryValueChanged(const QString& entry, float value);

    protected:
        QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override;

    private:
        QStringList _entries = {};
        std::unordered_map<QString, EntryData> _entryData = {};
        QListWidget* _sliderList = nullptr;

    };
}

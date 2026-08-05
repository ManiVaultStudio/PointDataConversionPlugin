#include "SlidersAction.h"

#include <actions/DecimalAction.h>

#include <QHBoxLayout>
#include <QLabel>

using namespace mv::gui;

SlidersAction::SlidersAction(QObject* parent, const QString& title) :
    WidgetAction(parent, title)
{
    setText(title);
    setDefaultWidgetFlags(SlidersAction::DisableOnFirstOpen);
}

void SlidersAction::initialize(const QStringList& entries) {
    setEntries(entries);
    setAllEntriesToDefault();
}

void SlidersAction::setEntries(const QStringList& entries) {
    _entries = entries;
    // populate default EntryData if new
    for (const QString& opt : entries) {
        if (!_entryData.contains(opt)) {
            _entryData.insert({ opt, EntryData{} });
        }
    }
    // remove data for removed entries
    for (auto it = _entryData.begin(); it != _entryData.end(); ) {
        if (!entries.contains(it->first))
            it = _entryData.erase(it);
        else ++it;
    }
}

void SlidersAction::setAllEntriesToDefault()
{
    for (const auto& [name, data] : _entryData)
        setDataForEntry(name, 5.f, 0.f, 100.f);

}

void SlidersAction::setRangeForEntry(const QString& entry, float minimum, float maximum) 
{
    if (!_entryData.contains(entry)) return;
    auto& d = _entryData[entry];
    d.min = minimum;
    d.max = maximum;
    d.value = std::clamp(d.value, d.min, d.max);
}

void SlidersAction::setValueForEntry(const QString& entry, float value) 
{
    if (!_entryData.contains(entry)) return;
    auto& d = _entryData[entry];
    value = std::clamp(value, d.min, d.max);
    if (std::abs(d.value - value) < 0.0001f) return;
    d.value = value;
    emit entryValueChanged(entry, value);
}

void SlidersAction::setDataForEntry(const QString& entry, float value, float minimum, float maximum) 
{
    setRangeForEntry(entry, minimum, maximum);
    setValueForEntry(entry, value);
}

float SlidersAction::getValueForEntry(const QString& entry) const 
{
    if (!_entryData.contains(entry))
        return 0.0f;
    return _entryData.at(entry).value;
}

std::vector<float> SlidersAction::getValues() const
{
    std::vector<float> values;
    values.reserve(_entries.size());

    for (const auto& opt : _entries)
        values.push_back(getValueForEntry(opt));

    return values;
}

void SlidersAction::setAllSlidersEnabled(bool enabled)
{
    setEnabled(enabled);

    if (!_sliderList)
        return;

    for (int i = 0; i < _sliderList->count(); ++i) {
        QListWidgetItem* item = _sliderList->item(i);
        if (QWidget* row = _sliderList->itemWidget(item)) {
            row->setEnabled(enabled);
        }
    }

}

QWidget* SlidersAction::getWidget(QWidget* parent, const std::int32_t& widgetFlags) 
{
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);

    const bool disableAll = _sliderList == nullptr && widgetFlags == WidgetFlag::DisableOnFirstOpen;

    _sliderList = new QListWidget(container);
    _sliderList->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(_sliderList);

    for (const QString& opt : _entries) {
        auto* item = new QListWidgetItem(_sliderList);

        QWidget* row = new QWidget(_sliderList);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(2, 2, 2, 2);

        const EntryData& d = _entryData.at(opt);

        DecimalAction* slider = new DecimalAction(this, opt, d.min, d.max, d.value, EntryData::DEFAULT_DECIMALS);
        rowLayout->addWidget(slider->createLabelWidget(container));
        rowLayout->addWidget(slider->createWidget(container));

        _sliderList->addItem(item);
        _sliderList->setItemWidget(item, row);
        item->setSizeHint(row->sizeHint());

        connect(slider, &DecimalAction::valueChanged, this, [this, opt](float value) {
            setValueForEntry(opt, value);
            });
    }

    if (disableAll)
        setAllSlidersEnabled(false);

    return container;
}

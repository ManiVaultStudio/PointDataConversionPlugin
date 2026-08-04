#include "SlidersAction.h"

#include <actions/DecimalAction.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>

using namespace mv::gui;

SlidersAction::SlidersAction(QObject* parent, const QString& title) :
    WidgetAction(parent, title)
{
    setText(title);
    setDefaultWidgetFlags(SlidersAction::Default);
}

void SlidersAction::initialize(const QStringList& options) {
    setOptions(options);
}

void SlidersAction::setOptions(const QStringList& options) {
    _options = options;
    // populate default OptionData if new
    for (const QString& opt : options) {
        if (!_optionData.contains(opt)) {
            _optionData.insert({ opt, OptionData{} });
        }
    }
    // remove data for removed options
    for (auto it = _optionData.begin(); it != _optionData.end(); ) {
        if (!options.contains(it->first))
            it = _optionData.erase(it);
        else ++it;
    }
}

void SlidersAction::setRangeForOption(const QString& option, float minimum, float maximum) 
{
    if (!_optionData.contains(option)) return;
    auto& d = _optionData[option];
    d.min = minimum;
    d.max = maximum;
    d.value = std::clamp(d.value, d.min, d.max);
}

void SlidersAction::setValueForOption(const QString& option, float value) 
{
    if (!_optionData.contains(option)) return;
    auto& d = _optionData[option];
    value = std::clamp(value, d.min, d.max);
    if (std::abs(d.value - value) < 0.0001f) return;
    d.value = value;
    emit optionValueChanged(option, value);
}

void SlidersAction::setDataForOption(const QString& option, float value, float minimum, float maximum) 
{
    setRangeForOption(option, minimum, maximum);
    setValueForOption(option, value);
}

float SlidersAction::getValueForOption(const QString& option) const 
{
    if (!_optionData.contains(option))
        return 0.0f;
    return _optionData.at(option).value;
}

std::vector<float> SlidersAction::getValues() const
{
    std::vector<float> values;
    values.reserve(_options.size());

    for (const auto& opt : _options)
        values.push_back(getValueForOption(opt));

    return values;
}

QWidget* SlidersAction::getWidget(QWidget* parent, const std::int32_t& widgetFlags) 
{
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* list = new QListWidget(container);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(list);

    for (const QString& opt : _options) {
        auto* item = new QListWidgetItem(list);

        QWidget* row = new QWidget(list);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(2, 2, 2, 2);

        const OptionData& d = _optionData.at(opt);

        DecimalAction* slider = new DecimalAction(this, opt, d.min, d.max, d.value, OptionData::DEFAULT_DECIMALS);
        rowLayout->addWidget(slider->createLabelWidget(container));
        rowLayout->addWidget(slider->createWidget(container));

        list->addItem(item);
        list->setItemWidget(item, row);
        item->setSizeHint(row->sizeHint());

        connect(slider, &DecimalAction::valueChanged, this, [this, opt](float value) {
            setValueForOption(opt, value);
            });
    }

    return container;
}

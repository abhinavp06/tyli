#include "media_fields.hpp"

#include <utility>

namespace tyli::fields {

namespace a = hoardor::audio;
namespace v = hoardor::video;

std::optional<a::Field> audio(const QString& name) {
    static const std::pair<const char*, a::Field> names[] = {
        {"title", a::Field::Title},       {"artist", a::Field::Artist},         {"albumArtist", a::Field::AlbumArtist},
        {"album", a::Field::Album},       {"genre", a::Field::Genre},           {"year", a::Field::Year},
        {"disc", a::Field::Disc},         {"track", a::Field::Track},           {"duration", a::Field::Duration},
        {"bitrate", a::Field::Bitrate},   {"sampleRate", a::Field::SampleRate}, {"bitDepth", a::Field::BitDepth},
        {"codec", a::Field::Codec},       {"lossless", a::Field::Lossless},     {"added", a::Field::Added},
        {"category", a::Field::Category}, {"root", a::Field::Root},             {"entry", a::Field::Entry},
        {"search", a::Field::Search},
    };
    for (const auto& [n, f] : names) {
        if (name == QLatin1String(n)) return f;
    }
    return std::nullopt;
}

std::optional<v::Field> video(const QString& name) {
    static const std::pair<const char*, v::Field> names[] = {
        {"type", v::Field::Type},         {"title", v::Field::Title},       {"year", v::Field::Year},
        {"genre", v::Field::Genre},       {"director", v::Field::Director}, {"show", v::Field::Show},
        {"season", v::Field::Season},     {"episode", v::Field::Episode},   {"duration", v::Field::Duration},
        {"height", v::Field::Height},     {"hdr", v::Field::Hdr},           {"videoCodec", v::Field::VideoCodec},
        {"added", v::Field::Added},       {"category", v::Field::Category}, {"root", v::Field::Root},
        {"entry", v::Field::Entry},       {"search", v::Field::Search},
    };
    for (const auto& [n, f] : names) {
        if (name == QLatin1String(n)) return f;
    }
    return std::nullopt;
}

namespace {

template <class Value>
Value toValue(const QVariant& value) {
    switch (value.typeId()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Bool:
        return Value{static_cast<std::int64_t>(value.toLongLong())};
    case QMetaType::Double: {
        const double d = value.toDouble();
        if (d == static_cast<double>(static_cast<std::int64_t>(d))) return Value{static_cast<std::int64_t>(d)};
        break;
    }
    default:
        break;
    }
    return Value{value.toString().toStdString()};
}

template <class Filter, class Lookup>
std::optional<Filter> filterOf(const QVariantMap& map, Lookup lookup) {
    Filter filter;
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        auto field = lookup(it.key());
        if (!field) return std::nullopt;
        using Value = decltype(filter.all.front().value);
        filter.all.push_back({*field, toValue<Value>(it.value())});
    }
    return filter;
}

template <class Order, class Lookup>
std::optional<std::vector<Order>> orderOf(const QStringList& names, Lookup lookup) {
    std::vector<Order> out;
    for (QString name : names) {
        const bool descending = name.startsWith(QLatin1Char('-'));
        if (descending) name.remove(0, 1);
        auto field = lookup(name);
        if (!field) return std::nullopt;
        out.push_back(Order{*field, descending});
    }
    return out;
}

}

std::optional<a::Filter> audioFilter(const QVariantMap& filter) { return filterOf<a::Filter>(filter, audio); }
std::optional<v::Filter> videoFilter(const QVariantMap& filter) { return filterOf<v::Filter>(filter, video); }
std::optional<std::vector<a::Order>> audioOrder(const QStringList& order) { return orderOf<a::Order>(order, audio); }
std::optional<std::vector<v::Order>> videoOrder(const QStringList& order) { return orderOf<v::Order>(order, video); }

}

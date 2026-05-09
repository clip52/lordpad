#include "JsonSchemaValidator.h"

#include <QJsonArray>
#include <QObject>
#include <QRegularExpression>

namespace JsonSchemaValidator {

namespace {

QString jsonTypeName(const QJsonValue& v)
{
    switch (v.type()) {
        case QJsonValue::Bool:   return QStringLiteral("boolean");
        case QJsonValue::Double: return QStringLiteral("number");
        case QJsonValue::String: return QStringLiteral("string");
        case QJsonValue::Array:  return QStringLiteral("array");
        case QJsonValue::Object: return QStringLiteral("object");
        case QJsonValue::Null:   return QStringLiteral("null");
        default:                 return QStringLiteral("undefined");
    }
}

bool typeMatches(const QString& expected, const QJsonValue& v)
{
    if (expected == QStringLiteral("integer")) {
        if (v.type() != QJsonValue::Double) return false;
        const double d = v.toDouble();
        return d == static_cast<qint64>(d);
    }
    return jsonTypeName(v) == expected;
}

void validateInner(const QJsonValue& instance, const QJsonObject& schema,
                   const QString& path, QStringList& errors)
{
    auto err = [&](const QString& msg) { errors << QStringLiteral("%1: %2").arg(path, msg); };

    // type
    if (schema.contains(QStringLiteral("type"))) {
        const QJsonValue tv = schema.value(QStringLiteral("type"));
        bool ok = false;
        if (tv.isArray()) {
            for (const QJsonValue& t : tv.toArray()) {
                if (typeMatches(t.toString(), instance)) { ok = true; break; }
            }
        } else if (tv.isString()) {
            ok = typeMatches(tv.toString(), instance);
        } else {
            ok = true;   // schema with a non-string type — be lenient.
        }
        if (!ok) {
            err(QObject::tr("tipo esperado %1, encontrado %2")
                    .arg(tv.isArray() ? QStringLiteral("um de [...]") : tv.toString(),
                         jsonTypeName(instance)));
        }
    }

    // enum
    if (schema.contains(QStringLiteral("enum"))) {
        const QJsonArray allowed = schema.value(QStringLiteral("enum")).toArray();
        bool ok = false;
        for (const QJsonValue& a : allowed) {
            if (a == instance) { ok = true; break; }
        }
        if (!ok) err(QObject::tr("valor não está em enum"));
    }

    // string-specific
    if (instance.isString()) {
        const QString s = instance.toString();
        if (schema.contains(QStringLiteral("minLength"))
            && s.size() < schema.value(QStringLiteral("minLength")).toInt())
            err(QObject::tr("comprimento mínimo violado"));
        if (schema.contains(QStringLiteral("maxLength"))
            && s.size() > schema.value(QStringLiteral("maxLength")).toInt())
            err(QObject::tr("comprimento máximo violado"));
        if (schema.contains(QStringLiteral("pattern"))) {
            QRegularExpression rx(schema.value(QStringLiteral("pattern")).toString());
            if (rx.isValid() && !rx.match(s).hasMatch())
                err(QObject::tr("não casa com pattern"));
        }
    }

    // numeric-specific
    if (instance.isDouble()) {
        const double n = instance.toDouble();
        if (schema.contains(QStringLiteral("minimum"))
            && n < schema.value(QStringLiteral("minimum")).toDouble())
            err(QObject::tr("abaixo do mínimo"));
        if (schema.contains(QStringLiteral("maximum"))
            && n > schema.value(QStringLiteral("maximum")).toDouble())
            err(QObject::tr("acima do máximo"));
    }

    // object-specific
    if (instance.isObject()) {
        const QJsonObject obj = instance.toObject();
        // required
        const QJsonArray req = schema.value(QStringLiteral("required")).toArray();
        for (const QJsonValue& r : req) {
            const QString name = r.toString();
            if (!obj.contains(name)) err(QObject::tr("propriedade obrigatória ausente: %1").arg(name));
        }
        // properties
        const QJsonObject props = schema.value(QStringLiteral("properties")).toObject();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (!props.contains(it.key())) continue;
            validateInner(it.value(), props.value(it.key()).toObject(),
                          path + QStringLiteral("/") + it.key(), errors);
        }
    }

    // array-specific
    if (instance.isArray()) {
        const QJsonArray arr = instance.toArray();
        if (schema.contains(QStringLiteral("items"))) {
            const QJsonObject itemSchema = schema.value(QStringLiteral("items")).toObject();
            for (int i = 0; i < arr.size(); ++i) {
                validateInner(arr.at(i), itemSchema,
                              path + QStringLiteral("/") + QString::number(i), errors);
            }
        }
    }
}

} // namespace

QStringList validate(const QJsonValue& instance, const QJsonObject& schema)
{
    QStringList errors;
    validateInner(instance, schema, QStringLiteral("#"), errors);
    return errors;
}

} // namespace JsonSchemaValidator

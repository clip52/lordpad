#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonValue>

// JsonSchemaValidator (M16) — small subset of JSON Schema (draft-07-ish).
//
// Supported keywords:
//   type           "string" | "number" | "integer" | "boolean" | "object" | "array" | "null"
//                  (or an array of those)
//   required       array of property names that must be present
//   properties     map of name → schema
//   items          schema applied to every element of an array
//   enum           array of allowed values
//   minimum / maximum     numeric bounds
//   minLength / maxLength string length bounds
//   pattern        regex (QRegularExpression) on strings
//
// Anything else in the schema is ignored — this is "good enough for editor
// validation feedback", not a conformant draft-07 implementation.
namespace JsonSchemaValidator {

// Validate `instance` against `schema`. Returns the list of error messages
// (empty when the instance is valid). Each message includes the JSON pointer
// path where the error was found.
QStringList validate(const QJsonValue& instance, const QJsonObject& schema);

} // namespace JsonSchemaValidator

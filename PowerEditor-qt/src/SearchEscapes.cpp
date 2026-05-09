#include "SearchEscapes.h"

namespace SearchEscapes {

QString expandExtended(const QString& input)
{
    QString out;
    out.reserve(input.size());
    const int n = input.size();
    int i = 0;
    while (i < n) {
        const QChar c = input.at(i);
        if (c != QLatin1Char('\\') || i + 1 >= n) {
            out.append(c);
            ++i;
            continue;
        }
        const QChar nc = input.at(i + 1);
        switch (nc.toLatin1()) {
            case 'n':  out.append('\n'); i += 2; continue;
            case 'r':  out.append('\r'); i += 2; continue;
            case 't':  out.append('\t'); i += 2; continue;
            case '0':  out.append(QChar(0)); i += 2; continue;
            case 'b':  out.append('\b'); i += 2; continue;
            case 'f':  out.append('\f'); i += 2; continue;
            case 'v':  out.append('\v'); i += 2; continue;
            case '\\': out.append('\\'); i += 2; continue;
            case '"':  out.append('"');  i += 2; continue;
            case '\'': out.append('\''); i += 2; continue;
            case 'x':
            case 'X': {
                int j = i + 2;
                int digits = 0;
                int v = 0;
                while (j < n && digits < 2) {
                    const QChar h = input.at(j);
                    int d = -1;
                    if (h.isDigit()) d = h.digitValue();
                    else if (h.toLower() >= 'a' && h.toLower() <= 'f') d = 10 + h.toLower().toLatin1() - 'a';
                    if (d < 0) break;
                    v = v * 16 + d;
                    ++j; ++digits;
                }
                if (digits == 0) { out.append(c); ++i; continue; }
                out.append(QChar(static_cast<ushort>(v & 0xFF)));
                i = j;
                continue;
            }
            case 'd':
            case 'D': {
                int j = i + 2;
                int digits = 0;
                int v = 0;
                while (j < n && digits < 3 && input.at(j).isDigit()) {
                    v = v * 10 + input.at(j).digitValue();
                    ++j; ++digits;
                }
                if (digits == 0 || v > 255) { out.append(c); ++i; continue; }
                out.append(QChar(static_cast<ushort>(v)));
                i = j;
                continue;
            }
            case 'o':
            case 'O': {
                int j = i + 2;
                int digits = 0;
                int v = 0;
                while (j < n && digits < 3) {
                    const QChar oc = input.at(j);
                    if (oc < '0' || oc > '7') break;
                    v = v * 8 + (oc.toLatin1() - '0');
                    ++j; ++digits;
                }
                if (digits == 0 || v > 255) { out.append(c); ++i; continue; }
                out.append(QChar(static_cast<ushort>(v)));
                i = j;
                continue;
            }
            default:
                // Unknown escape: keep both backslash and the next char.
                out.append(c);
                ++i;
                continue;
        }
    }
    return out;
}

} // namespace SearchEscapes

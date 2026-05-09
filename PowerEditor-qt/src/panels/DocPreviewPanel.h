#pragma once

#include <QDockWidget>
#include <QString>

class QComboBox;
class QPushButton;
class QLabel;
class QScrollArea;
class QPlainTextEdit;

// DocPreviewPanel (M24) — render multi-format via subprocess pra PNG.
//
// Formatos:
//   PDF       — usa `pdftoppm -r 100 in.pdf out` (poppler-utils)
//   PlantUML  — `plantuml -tpng -o<dir> in.puml`
//   AsciiDoc  — `asciidoctor -b html5 -o out.html in.adoc` (mostra HTML em browser embarcado simples?
//                 sem WebEngine; vamos exibir HTML cru no QPlainTextEdit)
//   LaTeX     — `pdflatex -interaction=nonstopmode in.tex` → PNG via pdftoppm
class DocPreviewPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit DocPreviewPanel(QWidget* parent = nullptr);

    bool openFile(const QString& path);

private slots:
    void onOpen();
    void onRender();

private:
    bool renderPdf(const QString& src, QString* err);
    bool renderPlantUml(const QString& src, QString* err);
    bool renderAsciidoc(const QString& src, QString* err);
    bool renderLatex(const QString& src, QString* err);

    QString detectFormat(const QString& path) const;

    QString         m_path;
    QString         m_format;
    QPushButton*    m_openBtn = nullptr;
    QPushButton*    m_renderBtn = nullptr;
    QComboBox*      m_formatCombo = nullptr;
    QLabel*         m_image = nullptr;
    QScrollArea*    m_scroll = nullptr;
    QPlainTextEdit* m_html = nullptr;
    QLabel*         m_status = nullptr;
};

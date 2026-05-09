#include "MainWindow.h"
#include "Settings.h"
#include "Theme.h"
#include "I18n.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QStringList>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("clip52");
    QCoreApplication::setOrganizationDomain("github.com/clip52");
    QCoreApplication::setApplicationName("LordPad");
    QCoreApplication::setApplicationVersion("0.9");

    // Window icon: prefer the icon theme (when installed via .desktop + hicolor),
    // fall back to the Qt resource so the running binary always has an icon
    // (taskbar / Alt-Tab / window decoration / about dialog).
    {
        QIcon themed = QIcon::fromTheme(QStringLiteral("lordpad"));
        if (themed.isNull()) themed = QIcon(QStringLiteral(":/icons/lordpad.svg"));
        app.setWindowIcon(themed);
    }

    // Link the running app to its .desktop entry so the window manager groups
    // the taskbar icon correctly under the launcher and inherits its metadata.
    QGuiApplication::setDesktopFileName(QStringLiteral("lordpad"));

    // Default UI language: pt-BR (loadDefaultTranslations falls back to pt-BR per project policy).
    I18n::loadDefaultTranslations(&app);

    QCommandLineParser parser;
    parser.setApplicationDescription("LordPad — editor de código nativo Linux Qt6");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Files to open", "[files...]");
    parser.process(app);

    ThemeManager::apply(&app, Settings::instance().darkTheme() ? AppTheme::Dark : AppTheme::Light);

    MainWindow w;
    w.show();

    const QStringList files = parser.positionalArguments();
    for (const QString& f : files) w.openFile(f);

    return app.exec();
}

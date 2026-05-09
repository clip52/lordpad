#include "MainWindow.h"
#include "EditorTab.h"
#include "FileIO.h"
#include "Settings.h"
#include "Theme.h"
#include "LexerMap.h"
#include "MultiView.h"
#include "LanguagesMenu.h"
#include "AutoSavePolicy.h"
#include "SessionManager.h"
#include "AutoCompleter.h"
#include "BookmarkManager.h"
#include "EditOperations.h"
#include "EolMenu.h"
#include "JsonXmlFormatter.h"
#include "MacroRecorder.h"
#include "ColorPickerHelper.h"
#include "dialogs/BookmarkDialog.h"
#include "dialogs/MacroDialog.h"
#include "dialogs/WordCountDialog.h"
#include "BraceMatcher.h"
#include "EditEnhancements.h"
#include "ExternalFileWatcher.h"
#include "PrintHelper.h"
#include "RecentProjects.h"
#include "Snippets.h"
#include "SpellChecker.h"
#include "WhitespaceView.h"
#include "dialogs/HashDialog.h"
#include "dialogs/SnippetsDialog.h"
#include "panels/FunctionListPanel.h"
#include "panels/DocumentMapPanel.h"
#include "panels/FileBrowserPanel.h"
#include "panels/ExecOutputPanel.h"
#include "panels/TerminalPanel.h"
#include "panels/FtpPanel.h"
#include "panels/SftpPanel.h"
#include "panels/GitLogPanel.h"
#include "panels/CalendarPanel.h"
#include "panels/CalculatorPanel.h"
#include "panels/TodoPanel.h"
#include "panels/QrPanel.h"
#include "panels/MermaidPanel.h"
#include "panels/MergeResolverPanel.h"
#include "panels/TestRunnerPanel.h"
#include "panels/NotebookPanel.h"
#include "panels/NotesPanel.h"
#include "panels/RestClientPanel.h"
#include "panels/SqlitePanel.h"
#include "panels/AiPanel.h"
#include "panels/PoEditorPanel.h"
#include "CoverageGutter.h"
#include "JsonSchemaValidator.h"
#include "EditorConfig.h"
#include "AutoPair.h"
#include "SmartIndent.h"
#include "CodeMetrics.h"
#include "panels/ImageViewerPanel.h"
#include "panels/RegexTesterPanel.h"
#include "panels/UnitConverterPanel.h"
#include "panels/JsonPathPanel.h"
#include "panels/ColorPalettePanel.h"
#include "panels/SysInfoPanel.h"
#include "panels/DevToolsPanel.h"
#include "panels/DbShellPanel.h"
#include "panels/JsonLinesPanel.h"
#include "panels/DocPreviewPanel.h"
#include "panels/ClipboardHistoryPanel.h"
#include "panels/ArchiveBrowserPanel.h"
#include "panels/TimerPanel.h"
#include "panels/GraphQlPanel.h"
#include "panels/CodeReviewPanel.h"
#include "panels/ProfileRunnerPanel.h"
#include "panels/ImageAnnotPanel.h"
#include "panels/CsvChartPanel.h"
#include "panels/MdTablePanel.h"
#include "panels/CronEditorPanel.h"
#include "panels/FileWatcherPanel.h"
#include "panels/TextStatsPanel.h"
#include "panels/MindMapPanel.h"
#include "panels/PastebinPanel.h"
#include "panels/TimeTrackerPanel.h"
#include "panels/CodeClonesPanel.h"
#include "panels/CallGraphPanel.h"
#include "panels/SecretScannerPanel.h"
#include "panels/ScreenshotPanel.h"
#include "panels/RssReaderPanel.h"
#include "panels/CliShellPanel.h"
#include "panels/HexViewerPanel.h"
#include "panels/SqlSchemaPanel.h"
#include "panels/BuildWatchPanel.h"
#include "panels/TodoAggregatorPanel.h"
#include "panels/GrepPanel.h"
#include "panels/YamlValidatorPanel.h"
#include "panels/DockerPanel.h"
#include "panels/JqRunnerPanel.h"
#include "panels/HtmlPreviewPanel.h"
#include "panels/GistPanel.h"
#include "panels/EnvManagerPanel.h"
#include "panels/SshExecPanel.h"
#include "panels/CheatsheetPanel.h"
#include "panels/VaultPanel.h"
#include "panels/KubectlPanel.h"
#include "panels/SystemMonitorPanel.h"
#include "panels/GpgPanel.h"
#include "panels/SslCertViewerPanel.h"
#include "panels/AsciiDocPreviewPanel.h"
#include "panels/PortScanPanel.h"
#include "panels/SystemdServicesPanel.h"
#include "panels/TunnelManagerPanel.h"
#include "panels/ApiBrowserPanel.h"
#include "panels/LogTailPanel.h"
#include "panels/SshfsPanel.h"
#include "AutoCorrect.h"
#include "SmartSelection.h"
#include "AiGhostCompletion.h"
#include "LayoutPresets.h"
#include "dialogs/GitignoreDialog.h"
#include "dialogs/PreCommitDialog.h"
#include "GitOps.h"
#include "TasksRunner.h"
#include "FormatOnSave.h"
#include "TypewriterMode.h"
#include "VimMode.h"
#include "Refactor.h"
#include "dialogs/ThemeEditorDialog.h"
#include "dialogs/SshConnectDialog.h"
#include "dialogs/GitCommitDialog.h"
#include "dialogs/GitBranchDialog.h"
#include "dialogs/GitStashDialog.h"
#include "dialogs/FindReplaceDialog.h"
#include "dialogs/GoToLineDialog.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/ComparePanel.h"
#include "dialogs/CssPreviewPane.h"
#include "dialogs/CsvTableView.h"
#include "dialogs/MarkdownPreviewPane.h"
#include "dialogs/HexViewer.h"
#include "dialogs/FindInFilesDialog.h"
#include "dialogs/CommandPalette.h"
#include "TabExtras.h"
#include "UrlHyperlink.h"
#include "CodeFormatter.h"
#include "GitStatusService.h"
#include "ThemePack.h"
#include "CrashRecovery.h"
#include "Workspace.h"
#include "OutlineRegex.h"
#include "MultiCursor.h"
#include "SmartHighlight.h"
#include "ColorMarkers.h"
#include "FoldingMenu.h"
#include "LspClient.h"
#include "LspFeatures.h"
#include "GitDiffGutter.h"
#include "ViewModes.h"
#include "EncodingConvert.h"
#include "plugins/PythonPluginHost.h"
#include "KeybindingsManager.h"
#include "StickyScroll.h"
#include "BracketColors.h"
#include "dialogs/PluginManagerDialog.h"
#include "dialogs/KeybindingsDialog.h"
#include "dialogs/QuickPickDialog.h"
#include "panels/LspDiagnosticsPanel.h"

#include <ScintillaEdit.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QKeyEvent>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>
#include <QSize>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QCursor>
#include <QShortcut>
#include <QToolBar>
#include <QToolButton>
#include <QToolTip>

namespace {
AppTheme themeFromSettings(const Settings& s) {
    if (s.darkTheme()) return AppTheme::Dark;
    return AppTheme::Light;
}
}

MainWindow::MainWindow(QWidget* parent, Mode mode)
    : QMainWindow(parent),
      m_multiView(nullptr),
      m_languagesMenu(nullptr),
      m_findDialog(nullptr),
      m_comparePanel(nullptr),
      m_cssPreviewPane(nullptr),
      m_csvTableView(nullptr),
      m_markdownPreviewPane(nullptr),
      m_hexViewer(nullptr),
      m_findInFilesDialog(nullptr),
      m_commandPalette(nullptr),
      m_functionListPanel(nullptr),
      m_documentMapPanel(nullptr),
      m_fileBrowserPanel(nullptr),
      m_execOutputPanel(nullptr),
      m_terminalPanel(nullptr),
      m_ftpPanel(nullptr),
      m_sftpPanel(nullptr),
      m_gitLogPanel(nullptr),
      m_autoSave(nullptr),
      m_session(nullptr),
      m_autoCompleter(nullptr),
      m_bookmarks(nullptr),
      m_macros(nullptr),
      m_eolMenu(nullptr),
      m_wordCountDialog(nullptr),
      m_macroDialog(nullptr),
      m_bookmarkDialog(nullptr),
      m_spellChecker(nullptr),
      m_externalWatcher(nullptr),
      m_braceMatcher(nullptr),
      m_whitespaceView(nullptr),
      m_snippets(nullptr),
      m_recentProjects(nullptr),
      m_editEnhance(nullptr),
      m_hashDialog(nullptr),
      m_snippetsDialog(nullptr),
      m_tabExtras(nullptr),
      m_codeFormatter(nullptr),
      m_gitStatus(nullptr),
      m_crashRecovery(nullptr),
      m_workspace(nullptr),
      m_statusGit(nullptr),
      m_menuRecentWorkspaces(nullptr),
      m_colorMarkers(nullptr),
      m_foldingMenu(nullptr),
      m_gitDiffGutter(nullptr),
      m_lsp(nullptr),
      m_lspFeatures(nullptr),
      m_lspDiagnosticsPanel(nullptr),
      m_encodingConvert(nullptr),
      m_viewModes(nullptr),
      m_pluginHost(nullptr),
      m_menuPlugins(nullptr),
      m_keybindings(nullptr),
      m_stickyScroll(nullptr),
      m_actSmartHighlight(nullptr),
      m_actGitDiffGutter(nullptr),
      m_actToggleLspDiagnostics(nullptr),
      m_statusPosition(nullptr),
      m_statusEncoding(nullptr),
      m_statusEol(nullptr),
      m_statusLanguage(nullptr) {
    m_mode = mode;
    setWindowTitle(mode == Mode::Primary ? tr("LordPad") : tr("LordPad — janela secundária"));
    setAttribute(Qt::WA_DeleteOnClose);
    {
        // Same fallback chain as main(): prefer themed icon, fall back to resource.
        QIcon icon = QIcon::fromTheme(QStringLiteral("lordpad"));
        if (icon.isNull()) icon = QIcon(QStringLiteral(":/icons/lordpad.svg"));
        setWindowIcon(icon);
    }

    createCentralWidget();
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();

    m_findDialog = new FindReplaceDialog(nullptr, this);
    // m_languagesMenu was created lazily inside createMenus() — DO NOT instantiate
    // a second one here; doing so overwrites the pointer and orphans the QMenu in
    // the menu bar, leaving it permanently disabled.

    // Dock panels (created here, populated/wired below).
    m_functionListPanel = new FunctionListPanel(this);
    m_documentMapPanel  = new DocumentMapPanel(this);
    m_fileBrowserPanel  = new FileBrowserPanel(this);
    m_execOutputPanel   = new ExecOutputPanel(this);
    addDockWidget(Qt::LeftDockWidgetArea,  m_fileBrowserPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_functionListPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_documentMapPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_execOutputPanel);
    // M9: terminal panel as a sibling of the exec output, hidden by default.
    m_terminalPanel = new TerminalPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalPanel);
    m_terminalPanel->hide();
    // M9: FTP panel.
    m_ftpPanel = new FtpPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_ftpPanel);
    m_ftpPanel->hide();
    connect(m_ftpPanel, &FtpPanel::openLocalFile, this, &MainWindow::openFile);
    connect(m_ftpPanel, &FtpPanel::statusMessage,
            this, [this](const QString& msg) { statusBar()->showMessage(msg, 5000); });
    // M10: SFTP panel.
    m_sftpPanel = new SftpPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_sftpPanel);
    m_sftpPanel->hide();
    // M10: Git log panel.
    m_gitLogPanel = new GitLogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_gitLogPanel);
    m_gitLogPanel->hide();

    // M12 utility panels — todos hidden by default; menus toggle visibility.
    m_calendarPanel   = new CalendarPanel(this);
    m_calculatorPanel = new CalculatorPanel(this);
    m_todoPanel       = new TodoPanel(this);
    m_qrPanel         = new QrPanel(this);
    m_mermaidPanel    = new MermaidPanel(this);
    addDockWidget(Qt::RightDockWidgetArea,  m_calendarPanel);
    addDockWidget(Qt::RightDockWidgetArea,  m_calculatorPanel);
    addDockWidget(Qt::RightDockWidgetArea,  m_todoPanel);
    addDockWidget(Qt::RightDockWidgetArea,  m_qrPanel);
    addDockWidget(Qt::RightDockWidgetArea,  m_mermaidPanel);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_calendarPanel),
                            static_cast<QDockWidget*>(m_calculatorPanel),
                            static_cast<QDockWidget*>(m_todoPanel),
                            static_cast<QDockWidget*>(m_qrPanel),
                            static_cast<QDockWidget*>(m_mermaidPanel) })
        d->hide();
    m_tasksRunner = new TasksRunner(this);

    // M13: merge resolver + test runner + format-on-save.
    m_mergePanel = new MergeResolverPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_mergePanel);
    m_mergePanel->hide();
    m_testRunner = new TestRunnerPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_testRunner);
    m_testRunner->hide();
    connect(m_testRunner, &TestRunnerPanel::openFileRequested,
            this, [this](const QString& p, int line) {
                openFile(p);
                if (auto* t = currentTab(); t && t->editor()) {
                    t->editor()->gotoLine(qMax(0, line - 1));
                    t->editor()->scrollCaret();
                }
            });
    m_formatOnSave = new FormatOnSave(this);

    // M14: notebook + notes panels (hidden by default).
    m_notebookPanel = new NotebookPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_notebookPanel);
    m_notebookPanel->hide();
    m_notesPanel    = new NotesPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_notesPanel);
    m_notesPanel->hide();

    // M15/M16: REST / SQLite / AI / PO panels (hidden by default).
    m_restPanel   = new RestClientPanel(this);
    m_sqlitePanel = new SqlitePanel(this);
    m_aiPanel     = new AiPanel(this);
    m_poPanel     = new PoEditorPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_restPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_sqlitePanel);
    addDockWidget(Qt::RightDockWidgetArea, m_aiPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_poPanel);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_restPanel),
                            static_cast<QDockWidget*>(m_sqlitePanel),
                            static_cast<QDockWidget*>(m_aiPanel),
                            static_cast<QDockWidget*>(m_poPanel) })
        d->hide();
    // M16: coverage gutter (per-editor attach happens in connectTabSignals).
    m_coverage = new CoverageGutter(this);

    // M17–M20: editor helpers + utility panels.
    m_autoPair    = new AutoPair(this);
    m_smartIndent = new SmartIndent(this);
    m_imagePanel  = new ImageViewerPanel(this);
    m_regexPanel  = new RegexTesterPanel(this);
    m_unitPanel   = new UnitConverterPanel(this);
    m_jsonPathPanel = new JsonPathPanel(this);
    m_colorPalette  = new ColorPalettePanel(this);
    m_sysInfoPanel  = new SysInfoPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_imagePanel);
    addDockWidget(Qt::RightDockWidgetArea, m_regexPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_unitPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_jsonPathPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_colorPalette);
    addDockWidget(Qt::RightDockWidgetArea, m_sysInfoPanel);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_imagePanel),
                            static_cast<QDockWidget*>(m_regexPanel),
                            static_cast<QDockWidget*>(m_unitPanel),
                            static_cast<QDockWidget*>(m_jsonPathPanel),
                            static_cast<QDockWidget*>(m_colorPalette),
                            static_cast<QDockWidget*>(m_sysInfoPanel) })
        d->hide();
    // M18: color palette → insert hex at caret in active editor.
    connect(m_colorPalette, &ColorPalettePanel::insertHexRequested, this, [this](const QString& hex) {
        if (auto* t = currentTab(); t && t->editor()) t->editor()->replaceSel(hex.toUtf8().constData());
    });

    // M21–M26
    m_aiGhost   = new AiGhostCompletion(this);
    m_layouts   = new LayoutPresets(this, this);
    m_devTools  = new DevToolsPanel(this);
    m_dbShell   = new DbShellPanel(this);
    m_jsonl     = new JsonLinesPanel(this);
    m_docPreview= new DocPreviewPanel(this);
    m_clipboard = new ClipboardHistoryPanel(this);
    m_archive   = new ArchiveBrowserPanel(this);
    m_timer     = new TimerPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_devTools);
    addDockWidget(Qt::RightDockWidgetArea, m_dbShell);
    addDockWidget(Qt::RightDockWidgetArea, m_jsonl);
    addDockWidget(Qt::RightDockWidgetArea, m_docPreview);
    addDockWidget(Qt::RightDockWidgetArea, m_clipboard);
    addDockWidget(Qt::RightDockWidgetArea, m_archive);
    addDockWidget(Qt::RightDockWidgetArea, m_timer);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_devTools),
                            static_cast<QDockWidget*>(m_dbShell),
                            static_cast<QDockWidget*>(m_jsonl),
                            static_cast<QDockWidget*>(m_docPreview),
                            static_cast<QDockWidget*>(m_clipboard),
                            static_cast<QDockWidget*>(m_archive),
                            static_cast<QDockWidget*>(m_timer) })
        d->hide();
    // M25: clipboard insert → editor.
    connect(m_clipboard, &ClipboardHistoryPanel::insertTextRequested, this,
            [this](const QString& text) {
                if (auto* t = currentTab(); t && t->editor()) t->editor()->replaceSel(text.toUtf8().constData());
            });
    // M26: pomodoro notification → status bar.
    connect(m_timer, &TimerPanel::notify, this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 5000);
    });

    // M27–M35
    m_autoCorrect = new AutoCorrect(this);
    m_graphql     = new GraphQlPanel(this);
    m_review      = new CodeReviewPanel(this);
    m_profile     = new ProfileRunnerPanel(this);
    m_imgAnnot    = new ImageAnnotPanel(this);
    m_csvChart    = new CsvChartPanel(this);
    m_mdTable     = new MdTablePanel(this);
    m_cron        = new CronEditorPanel(this);
    m_watcher     = new FileWatcherPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_graphql);
    addDockWidget(Qt::RightDockWidgetArea, m_review);
    addDockWidget(Qt::BottomDockWidgetArea, m_profile);
    addDockWidget(Qt::RightDockWidgetArea, m_imgAnnot);
    addDockWidget(Qt::RightDockWidgetArea, m_csvChart);
    addDockWidget(Qt::RightDockWidgetArea, m_mdTable);
    addDockWidget(Qt::RightDockWidgetArea, m_cron);
    addDockWidget(Qt::BottomDockWidgetArea, m_watcher);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_graphql),
                            static_cast<QDockWidget*>(m_review),
                            static_cast<QDockWidget*>(m_profile),
                            static_cast<QDockWidget*>(m_imgAnnot),
                            static_cast<QDockWidget*>(m_csvChart),
                            static_cast<QDockWidget*>(m_mdTable),
                            static_cast<QDockWidget*>(m_cron),
                            static_cast<QDockWidget*>(m_watcher) })
        d->hide();
    connect(m_review, &CodeReviewPanel::gotoLineRequested, this, [this](int line) {
        if (auto* t = currentTab(); t && t->editor()) {
            t->editor()->gotoLine(qMax(0, line - 1));
            t->editor()->scrollCaret();
        }
    });

    // M36–M50
    m_textStats   = new TextStatsPanel(this);
    m_mindMap     = new MindMapPanel(this);
    m_pastebin    = new PastebinPanel(this);
    m_timeTracker = new TimeTrackerPanel(this);
    m_codeClones  = new CodeClonesPanel(this);
    m_callGraph   = new CallGraphPanel(this);
    m_secrets     = new SecretScannerPanel(this);
    m_screenshot  = new ScreenshotPanel(this);
    m_rss         = new RssReaderPanel(this);
    m_cliShell    = new CliShellPanel(this);
    m_hexPanel    = new HexViewerPanel(this);
    m_sqlSchema   = new SqlSchemaPanel(this);
    m_buildWatch  = new BuildWatchPanel(this);
    addDockWidget(Qt::RightDockWidgetArea,  m_textStats);
    addDockWidget(Qt::RightDockWidgetArea,  m_mindMap);
    addDockWidget(Qt::RightDockWidgetArea,  m_pastebin);
    addDockWidget(Qt::RightDockWidgetArea,  m_timeTracker);
    addDockWidget(Qt::RightDockWidgetArea,  m_codeClones);
    addDockWidget(Qt::RightDockWidgetArea,  m_callGraph);
    addDockWidget(Qt::RightDockWidgetArea,  m_secrets);
    addDockWidget(Qt::RightDockWidgetArea,  m_screenshot);
    addDockWidget(Qt::RightDockWidgetArea,  m_rss);
    addDockWidget(Qt::BottomDockWidgetArea, m_cliShell);
    addDockWidget(Qt::RightDockWidgetArea,  m_hexPanel);
    addDockWidget(Qt::RightDockWidgetArea,  m_sqlSchema);
    addDockWidget(Qt::BottomDockWidgetArea, m_buildWatch);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_textStats),
                            static_cast<QDockWidget*>(m_mindMap),
                            static_cast<QDockWidget*>(m_pastebin),
                            static_cast<QDockWidget*>(m_timeTracker),
                            static_cast<QDockWidget*>(m_codeClones),
                            static_cast<QDockWidget*>(m_callGraph),
                            static_cast<QDockWidget*>(m_secrets),
                            static_cast<QDockWidget*>(m_screenshot),
                            static_cast<QDockWidget*>(m_rss),
                            static_cast<QDockWidget*>(m_cliShell),
                            static_cast<QDockWidget*>(m_hexPanel),
                            static_cast<QDockWidget*>(m_sqlSchema),
                            static_cast<QDockWidget*>(m_buildWatch) })
        d->hide();
    auto gotoLineCb = [this](int line) {
        if (auto* t = currentTab(); t && t->editor()) {
            t->editor()->gotoLine(qMax(0, line - 1));
            t->editor()->scrollCaret();
        }
    };
    connect(m_codeClones, &CodeClonesPanel::gotoLineRequested, this, gotoLineCb);
    connect(m_secrets,    &SecretScannerPanel::gotoLineRequested, this, gotoLineCb);

    // M51–M60
    m_todoAgg  = new TodoAggregatorPanel(this);
    m_grep     = new GrepPanel(this);
    m_yamlVal  = new YamlValidatorPanel(this);
    m_docker   = new DockerPanel(this);
    m_jq       = new JqRunnerPanel(this);
    m_htmlPrev = new HtmlPreviewPanel(this);
    m_gist     = new GistPanel(this);
    m_envMgr   = new EnvManagerPanel(this);
    m_sshExec  = new SshExecPanel(this);
    m_cheats   = new CheatsheetPanel(this);
    m_vault    = new VaultPanel(this);
    m_kubectl    = new KubectlPanel(this);
    m_sysmon     = new SystemMonitorPanel(this);
    m_gpg        = new GpgPanel(this);
    m_ssl        = new SslCertViewerPanel(this);
    m_adoc       = new AsciiDocPreviewPanel(this);
    m_portScan   = new PortScanPanel(this);
    m_systemd    = new SystemdServicesPanel(this);
    m_tunnels    = new TunnelManagerPanel(this);
    m_apiBrowser = new ApiBrowserPanel(this);
    m_logTail    = new LogTailPanel(this);
    m_sshfs      = new SshfsPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_kubectl);
    addDockWidget(Qt::RightDockWidgetArea,  m_sysmon);
    addDockWidget(Qt::RightDockWidgetArea,  m_gpg);
    addDockWidget(Qt::RightDockWidgetArea,  m_ssl);
    addDockWidget(Qt::RightDockWidgetArea,  m_adoc);
    addDockWidget(Qt::BottomDockWidgetArea, m_portScan);
    addDockWidget(Qt::BottomDockWidgetArea, m_systemd);
    addDockWidget(Qt::RightDockWidgetArea,  m_tunnels);
    addDockWidget(Qt::RightDockWidgetArea,  m_apiBrowser);
    addDockWidget(Qt::BottomDockWidgetArea, m_logTail);
    addDockWidget(Qt::RightDockWidgetArea,  m_sshfs);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_kubectl),
                            static_cast<QDockWidget*>(m_sysmon),
                            static_cast<QDockWidget*>(m_gpg),
                            static_cast<QDockWidget*>(m_ssl),
                            static_cast<QDockWidget*>(m_adoc),
                            static_cast<QDockWidget*>(m_portScan),
                            static_cast<QDockWidget*>(m_systemd),
                            static_cast<QDockWidget*>(m_tunnels),
                            static_cast<QDockWidget*>(m_apiBrowser),
                            static_cast<QDockWidget*>(m_logTail),
                            static_cast<QDockWidget*>(m_sshfs) })
        d->hide();
    connect(m_sshfs, &SshfsPanel::openFileRequested, this, &MainWindow::openFile);
    addDockWidget(Qt::BottomDockWidgetArea, m_todoAgg);
    addDockWidget(Qt::BottomDockWidgetArea, m_grep);
    addDockWidget(Qt::RightDockWidgetArea,  m_yamlVal);
    addDockWidget(Qt::BottomDockWidgetArea, m_docker);
    addDockWidget(Qt::RightDockWidgetArea,  m_jq);
    addDockWidget(Qt::RightDockWidgetArea,  m_htmlPrev);
    addDockWidget(Qt::RightDockWidgetArea,  m_gist);
    addDockWidget(Qt::RightDockWidgetArea,  m_envMgr);
    addDockWidget(Qt::BottomDockWidgetArea, m_sshExec);
    addDockWidget(Qt::RightDockWidgetArea,  m_cheats);
    addDockWidget(Qt::RightDockWidgetArea,  m_vault);
    for (QDockWidget* d : { static_cast<QDockWidget*>(m_todoAgg),
                            static_cast<QDockWidget*>(m_grep),
                            static_cast<QDockWidget*>(m_yamlVal),
                            static_cast<QDockWidget*>(m_docker),
                            static_cast<QDockWidget*>(m_jq),
                            static_cast<QDockWidget*>(m_htmlPrev),
                            static_cast<QDockWidget*>(m_gist),
                            static_cast<QDockWidget*>(m_envMgr),
                            static_cast<QDockWidget*>(m_sshExec),
                            static_cast<QDockWidget*>(m_cheats),
                            static_cast<QDockWidget*>(m_vault) })
        d->hide();
    auto openAtLineCb = [this](const QString& path, int line) {
        openFile(path);
        if (auto* t = currentTab(); t && t->editor()) {
            t->editor()->gotoLine(qMax(0, line - 1));
            t->editor()->scrollCaret();
        }
    };
    connect(m_todoAgg, &TodoAggregatorPanel::openFileAtLine, this, openAtLineCb);
    connect(m_grep,    &GrepPanel::openFileAtLine,           this, openAtLineCb);

    // M14: typewriter + vim helpers (per-editor wiring done in connectTabSignals).
    m_typewriter = new TypewriterMode(this);
    m_vim        = new VimMode(this);
    connect(m_vim, &VimMode::requestSaveCurrentTab, this, [this]() {
        if (auto* t = currentTab()) saveTab(t);
    });
    connect(m_vim, &VimMode::requestCloseCurrentTab, this, [this]() {
        if (auto* t = currentTab()) onMultiViewTabCloseRequested(t);
    });
    connect(m_vim, &VimMode::requestOpenFile, this, &MainWindow::openFile);
    connect(m_vim, &VimMode::modeChanged, this, [this](VimMode::Mode) {
        if (m_statusVim) m_statusVim->setText(m_vim ? m_vim->modeLabel() : QString());
    });
    connect(m_sftpPanel, &SftpPanel::openLocalFile, this, &MainWindow::openFile);
    connect(m_sftpPanel, &SftpPanel::statusMessage,
            this, [this](const QString& msg) { statusBar()->showMessage(msg, 5000); });
    // Hide all docks by default; user toggles them via View menu.
    m_functionListPanel->hide();
    m_documentMapPanel->hide();
    m_fileBrowserPanel->hide();
    m_execOutputPanel->hide();
    // M9: clickable error-line jump.
    connect(m_execOutputPanel, &ExecOutputPanel::locationActivated,
            this, &MainWindow::onExecOutputLocationActivated);

    connect(m_functionListPanel, &FunctionListPanel::gotoLineRequested,
            this, &MainWindow::onFunctionListGoto);
    connect(m_fileBrowserPanel,  &FileBrowserPanel::openFileRequested,
            this, &MainWindow::onFileBrowserOpenFile);

    // Auto-save: emit-only; we save dirty named tabs on each tick.
    m_autoSave = new AutoSavePolicy(this);
    connect(m_autoSave, &AutoSavePolicy::autoSaveTick, this, &MainWindow::onAutoSaveTick);

    // Session manager: load + save list of open files.
    m_session = new SessionManager(this);

    // M4 helpers (singletons attached to MainWindow's lifetime).
    m_autoCompleter = new AutoCompleter(this);
    m_bookmarks     = new BookmarkManager(this);
    m_macros        = new MacroRecorder(this);
    m_eolMenu       = new EolMenu(this);

    // M5 helpers
    m_spellChecker    = new SpellChecker(this);
    m_externalWatcher = new ExternalFileWatcher(this);
    m_braceMatcher    = new BraceMatcher(this);
    m_whitespaceView  = new WhitespaceView(this);
    m_snippets        = new Snippets(this);
    m_recentProjects  = new RecentProjects(this);
    m_editEnhance     = new EditEnhancements(this);
    connect(m_externalWatcher, &ExternalFileWatcher::fileChangedExternally,
            this, &MainWindow::onExternalFileChanged);
    connect(m_externalWatcher, &ExternalFileWatcher::fileRemovedExternally,
            this, &MainWindow::onExternalFileRemoved);

    // M6 helpers
    m_tabExtras     = new TabExtras(this);
    m_codeFormatter = new CodeFormatter(this);
    m_gitStatus     = new GitStatusService(this);
    m_crashRecovery = new CrashRecovery(this);
    m_workspace     = new Workspace(this);
    connect(m_gitStatus, &GitStatusService::statusReady,
            this, &MainWindow::onGitStatusReady);
    if (m_multiView) {
        if (auto* g = m_multiView->primaryGroup())   m_tabExtras->attachTabWidget(g);
        if (auto* g = m_multiView->secondaryGroup()) m_tabExtras->attachTabWidget(g);
    }
    m_crashRecovery->start();

    // M7 helpers — instantiated after multiView/menus exist so we can register
    // shortcuts and dock the LSP diagnostics panel against the main window.
    m_colorMarkers   = new ColorMarkers(this);
    m_foldingMenu    = new FoldingMenu(this);
    m_gitDiffGutter  = new GitDiffGutter(this);
    m_lsp            = new LspClient(this);
    m_encodingConvert = new EncodingConvert(this);
    m_viewModes      = new ViewModes(this, m_multiView, this);
    m_viewModes->registerShortcuts();
    connect(m_lsp, &LspClient::diagnosticsUpdated,
            this, &MainWindow::onLspDiagnosticsUpdated);
    connect(m_lsp, &LspClient::serverError,
            this, &MainWindow::onLspServerError);
    m_lspDiagnosticsPanel = new LspDiagnosticsPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_lspDiagnosticsPanel);
    m_lspDiagnosticsPanel->hide();
    connect(m_lspDiagnosticsPanel, &LspDiagnosticsPanel::diagnosticActivated,
            this, &MainWindow::onLspDiagnosticActivated);

    // M8: keep status bar in sync with encoding/EOL changes.
    connect(m_encodingConvert, &EncodingConvert::encodingChanged,
            this, [this](const QString&, EncodingConvert::Encoding) { updateStatusBar(); });
    if (m_eolMenu) {
        connect(m_eolMenu, &EolMenu::eolModeChanged,
                this, [this](int) { updateStatusBar(); });
    }

    // M8: LSP feature wiring (hover / goto-def / completion).
    m_lspFeatures = new LspFeatures(m_lsp, m_multiView, this);
    connect(m_lspFeatures, &LspFeatures::hoverReady,
            this, &MainWindow::onLspHoverReady);
    connect(m_lspFeatures, &LspFeatures::definitionResolved,
            this, &MainWindow::onLspDefinitionResolved);
    // M9 result signals
    connect(m_lspFeatures, &LspFeatures::signatureHelpReady,
            this, &MainWindow::onLspSignatureHelpReady);
    connect(m_lspFeatures, &LspFeatures::referencesReady,
            this, &MainWindow::onLspReferencesReady);
    connect(m_lspFeatures, &LspFeatures::documentSymbolsReady,
            this, &MainWindow::onLspDocumentSymbolsReady);
    connect(m_lspFeatures, &LspFeatures::workspaceSymbolsReady,
            this, &MainWindow::onLspWorkspaceSymbolsReady);
    connect(m_lspFeatures, &LspFeatures::renameEditsReady,
            this, &MainWindow::onLspRenameEditsReady);
    // M11
    connect(m_lspFeatures, &LspFeatures::codeActionsReady,
            this, &MainWindow::onLspCodeActionsReady);
    connect(m_lspFeatures, &LspFeatures::inlayHintsReady,
            this, &MainWindow::onLspInlayHintsReady);
    // M13
    connect(m_lspFeatures, &LspFeatures::semanticTokensReady,
            this, &MainWindow::onLspSemanticTokensReady);
    connect(m_lspFeatures, &LspFeatures::outlineSymbolsReady,
            this, &MainWindow::onLspOutlineSymbolsReady);
    // F12 (goto definition) and Ctrl+Space (completion) are wired through
    // QAction shortcuts on their menu entries — see createMenus().

    // M9: Python plugin host. Boot the interpreter, install the accessor /
    // handler callbacks, then auto-load every .py in the plugin dir.
    // M12: only the Primary window owns the plugin host. Python can only be
    // initialized once per process, and plugins register against a single
    // active editor — secondary windows skip the entire subsystem.
    m_pluginHost = (m_mode == Mode::Primary) ? new PythonPluginHost(this) : nullptr;
    if (m_pluginHost) {
    m_pluginHost->setSciAccessor(  [this]() -> ScintillaEdit* {
        auto* t = currentTab(); return t ? t->editor() : nullptr;
    });
    m_pluginHost->setPathAccessor( [this]() -> QString {
        auto* t = currentTab(); return t ? t->filePath() : QString();
    });
    m_pluginHost->setSaveAccessor( [this]() -> bool {
        auto* t = currentTab(); return t ? saveTab(t) : false;
    });
    m_pluginHost->setLexerAccessor([this]() -> QString {
        auto* t = currentTab(); return t ? LexerMap::lexerNameForPath(t->filePath()) : QString();
    });
    m_pluginHost->setMessageHandler([this](const QString& title, const QString& text) {
        QMessageBox::information(this, title, text);
    });
    m_pluginHost->setAskHandler([this](const QString& prompt, const QString& def, bool* ok) -> QString {
        bool localOk = false;
        const QString r = QInputDialog::getText(this, tr("Plugin"), prompt,
                                                QLineEdit::Normal, def, &localOk);
        if (ok) *ok = localOk;
        return r;
    });
    m_pluginHost->setActionHandler([this](const QString& label, std::function<void()> cb) {
        if (!m_menuPlugins) return;
        auto* a = m_menuPlugins->addAction(label);
        connect(a, &QAction::triggered, this, [cb] { cb(); });
    });
    if (m_pluginHost->isAvailable() && m_pluginHost->initialize()) {
        const QStringList files = m_pluginHost->discoverPlugins();
        for (const QString& f : files) m_pluginHost->loadPlugin(f);
    }
    }   // if (m_pluginHost)

    // M10: keybindings — collect after all menus + plugins have registered
    // their actions, then apply persisted overrides on top of the defaults.
    m_keybindings = new KeybindingsManager(this);
    m_keybindings->collectFromHost(this);
    m_keybindings->applyPersistedOverrides();

    // M11: sticky scroll header. Single instance shared across editors;
    // setActiveEditor moves it to the current tab.
    m_stickyScroll = new StickyScroll(this);

    // Oferece restaurar buffers órfãos de uma execução anterior que travou.
    {
        const auto orphans = m_crashRecovery->findOrphanRecoveries();
        for (const auto& r : orphans) {
            const QString label = r.originalPath.isEmpty() ? tr("(sem título)") : r.originalPath;
            const auto btn = QMessageBox::question(this, tr("Recuperação"),
                tr("Recuperar conteúdo de \"%1\" da sessão anterior?").arg(label),
                QMessageBox::Yes | QMessageBox::No);
            if (btn == QMessageBox::Yes) {
                QFile f(r.recoveryFile);
                if (f.open(QIODevice::ReadOnly)) {
                    QByteArray all = f.readAll();
                    int hdrEnd = all.indexOf("---END-META---\n");
                    QByteArray body = (hdrEnd >= 0) ? all.mid(hdrEnd + 15) : all;
                    auto* tab = new EditorTab(this);
                    applyEditorPreferences(tab);
                    applyThemeAndLexer(tab);
                    connectTabSignals(tab);
                    m_multiView->addTab(tab, tab->tabTitle());
                    tab->editor()->setText(body.constData());
                    tab->setModified(true);
                    setActiveTab(tab);
                }
            }
            m_crashRecovery->consume(r);
        }
    }

    const Settings& s = Settings::instance();
    if (!s.windowGeometry().isEmpty()) restoreGeometry(s.windowGeometry());
    else                               resize(1100, 750);
    if (!s.windowState().isEmpty())    restoreState(s.windowState());

    rebuildRecentFilesMenu();

    // Restore previous session if enabled. M12: only the Primary window
    // restores; Secondary windows always start with a single empty buffer.
    bool restoredAny = false;
    if (m_mode == Mode::Primary && m_session && m_session->restoreOnStartup()) {
        int activeIndex = 0;
        const QStringList paths = m_session->loadSession(&activeIndex);
        for (const QString& p : paths) openFile(p);
        if (!paths.isEmpty()) {
            restoredAny = true;
            if (activeIndex >= 0 && activeIndex < m_multiView->tabCount())
                setActiveTab(tabAt(activeIndex));
        }
    }
    if (!restoredAny) onFileNew();
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// Construction helpers
// ---------------------------------------------------------------------------
void MainWindow::createCentralWidget() {
    m_multiView = new MultiView(this);
    m_multiView->setTabsClosable(true);
    m_multiView->setTabsMovable(true);
    setCentralWidget(m_multiView);

    connect(m_multiView, &MultiView::currentTabChanged, this, &MainWindow::onMultiViewCurrentChanged);
    connect(m_multiView, &MultiView::tabCloseRequested, this, &MainWindow::onMultiViewTabCloseRequested);
    connect(m_multiView, &MultiView::newTabRequested,   this, &MainWindow::onFileNew);
}

void MainWindow::createActions() {
    auto mk = [this](const QString& text, const QKeySequence& shortcut, auto slot) {
        auto* a = new QAction(text, this);
        if (!shortcut.isEmpty()) a->setShortcut(shortcut);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };

    m_actNew      = mk(tr("&New"),         QKeySequence::New,    &MainWindow::onFileNew);
    m_actOpen     = mk(tr("&Open..."),     QKeySequence::Open,   &MainWindow::onFileOpen);
    m_actSave     = mk(tr("&Save"),        QKeySequence::Save,   &MainWindow::onFileSave);
    m_actSaveAs   = mk(tr("Save &As..."),  QKeySequence::SaveAs, &MainWindow::onFileSaveAs);
    m_actClose    = mk(tr("&Close"),       QKeySequence::Close,  &MainWindow::onFileClose);
    m_actExit     = mk(tr("E&xit"),        QKeySequence::Quit,   &MainWindow::onFileExit);

    m_actUndo      = mk(tr("&Undo"),       QKeySequence::Undo,      &MainWindow::onEditUndo);
    m_actRedo      = mk(tr("&Redo"),       QKeySequence::Redo,      &MainWindow::onEditRedo);
    m_actCut       = mk(tr("Cu&t"),        QKeySequence::Cut,       &MainWindow::onEditCut);
    m_actCopy      = mk(tr("&Copy"),       QKeySequence::Copy,      &MainWindow::onEditCopy);
    m_actPaste     = mk(tr("&Paste"),      QKeySequence::Paste,     &MainWindow::onEditPaste);
    m_actSelectAll = mk(tr("Select &All"), QKeySequence::SelectAll, &MainWindow::onEditSelectAll);

    m_actFind      = mk(tr("&Find..."),         QKeySequence::Find,    &MainWindow::onSearchFind);
    m_actReplace   = mk(tr("&Replace..."),      QKeySequence::Replace, &MainWindow::onSearchReplace);
    m_actGoToLine  = mk(tr("&Go To Line..."),   QKeySequence(Qt::CTRL | Qt::Key_G), &MainWindow::onSearchGoToLine);

    m_themeGroup = new QActionGroup(this);
    m_themeGroup->setExclusive(true);
    m_actThemeLight   = mk(tr("Theme: &Light"),   QKeySequence(), &MainWindow::onViewSetThemeLight);
    m_actThemeDark    = mk(tr("Theme: &Dark"),    QKeySequence(), &MainWindow::onViewSetThemeDark);
    m_actThemeDracula = mk(tr("Theme: D&racula"), QKeySequence(), &MainWindow::onViewSetThemeDracula);
    for (auto* a : { m_actThemeLight, m_actThemeDark, m_actThemeDracula }) {
        a->setCheckable(true);
        m_themeGroup->addAction(a);
    }
    const auto savedTheme = Settings::instance().darkTheme() ? AppTheme::Dark : AppTheme::Light;
    if (savedTheme == AppTheme::Dark) m_actThemeDark->setChecked(true);
    else                               m_actThemeLight->setChecked(true);

    m_actToggleLineNumbers = mk(tr("Show &Line Numbers"),  QKeySequence(), &MainWindow::onViewToggleLineNumbers);
    m_actToggleLineNumbers->setCheckable(true);
    m_actToggleLineNumbers->setChecked(Settings::instance().showLineNumbers());

    m_actToggleWordWrap    = mk(tr("&Word Wrap"),          QKeySequence(), &MainWindow::onViewToggleWordWrap);
    m_actToggleWordWrap->setCheckable(true);
    m_actToggleWordWrap->setChecked(Settings::instance().wordWrap());

    m_actSplitHorizontal      = mk(tr("Split &Horizontal"),     QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), &MainWindow::onViewSplitHorizontal);
    m_actSplitVertical        = mk(tr("Split &Vertical"),       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V), &MainWindow::onViewSplitVertical);
    m_actUnsplit              = mk(tr("&Unsplit"),              QKeySequence(),                                  &MainWindow::onViewUnsplit);
    m_actMoveTabToOtherGroup  = mk(tr("&Move Tab to Other Group"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M), &MainWindow::onViewMoveTabToOtherGroup);

    m_actPreferences = mk(tr("&Preferences..."), QKeySequence::Preferences, &MainWindow::onToolsPreferences);
    m_actCompare     = mk(tr("&Compare..."),     QKeySequence(),            &MainWindow::onToolsCompare);
    m_actCssPreview  = mk(tr("CSS &Preview..."), QKeySequence(),            &MainWindow::onToolsCssPreview);
    m_actCsvView     = mk(tr("&CSV Table View..."), QKeySequence(),         &MainWindow::onToolsCsvView);
    m_actMarkdownPreview = mk(tr("&Markdown Preview..."), QKeySequence(),   &MainWindow::onToolsMarkdownPreview);
    m_actHexViewer   = mk(tr("&Hex Viewer..."),  QKeySequence(),            &MainWindow::onToolsHexViewer);
    m_actFindInFiles = mk(tr("Find in &Files..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), &MainWindow::onToolsFindInFiles);
    m_actCommandPalette = mk(tr("Command &Palette..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), &MainWindow::onToolsCommandPalette);
    m_actRunCommand  = mk(tr("&Run Command..."), QKeySequence(Qt::CTRL | Qt::Key_R), &MainWindow::onToolsRunCommand);

    m_actToggleFunctionList = mk(tr("Function &List Panel"), QKeySequence(), &MainWindow::onViewToggleFunctionList);
    m_actToggleDocumentMap  = mk(tr("Document &Map Panel"),  QKeySequence(), &MainWindow::onViewToggleDocumentMap);
    m_actToggleFileBrowser  = mk(tr("File &Browser Panel"),  QKeySequence(), &MainWindow::onViewToggleFileBrowser);
    m_actToggleExecOutput   = mk(tr("&Exec Output Panel"),   QKeySequence(), &MainWindow::onViewToggleExecOutput);
    for (auto* a : { m_actToggleFunctionList, m_actToggleDocumentMap,
                     m_actToggleFileBrowser, m_actToggleExecOutput }) {
        a->setCheckable(true);
        a->setChecked(false);
    }

    m_actAbout = mk(tr("&About"), QKeySequence(), &MainWindow::onHelpAbout);

    // ---- M4 actions: edit ops, bookmarks, tools ----
    // (Defined here as anonymous to keep the constructor compact; menu wiring done in createMenus.)
}

// Helper: create + connect M4 actions. Called from createMenus() before the menus are built.
namespace { struct M4Tag {}; }

void MainWindow::createMenus() {
    auto* mb = menuBar();

    auto* mFile = mb->addMenu(tr("&Arquivo"));
    mFile->addAction(m_actNew);
    mFile->addAction(m_actOpen);
    auto mkF = [this](QMenu* m, const QString& t, const QKeySequence& sc, auto slot) {
        auto* a = m->addAction(t);
        if (!sc.isEmpty()) a->setShortcut(sc);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    // M12: open another full editor window (sem plugin host — limitação
    // documentada em MainWindow::Mode::Secondary).
    mkF(mFile, tr("Nova &Janela"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), &MainWindow::onFileNewWindow);
    mkF(mFile, tr("Open &Folder..."), QKeySequence(Qt::CTRL | Qt::Key_K), &MainWindow::onFileOpenFolder);
    m_menuRecent = mFile->addMenu(tr("Abrir &Recente"));
    mFile->addSeparator();
    mFile->addAction(m_actSave);
    mFile->addAction(m_actSaveAs);
    mkF(mFile, tr("&Reload from Disk"),  QKeySequence(),                       &MainWindow::onFileReloadFromDisk);
    mFile->addSeparator();
    mkF(mFile, tr("&Print..."),          QKeySequence::Print,                  &MainWindow::onFilePrint);
    mkF(mFile, tr("Print Pre&view..."),  QKeySequence(),                       &MainWindow::onFilePrintPreview);
    mFile->addSeparator();
    auto* mWorkspace = mFile->addMenu(tr("&Workspace"));
    mkF(mWorkspace, tr("Abrir Workspace..."), QKeySequence(),                                  &MainWindow::onWorkspaceOpen);
    mkF(mWorkspace, tr("Salvar Workspace"),    QKeySequence(),                                  &MainWindow::onWorkspaceSave);
    mkF(mWorkspace, tr("Salvar Workspace Como..."), QKeySequence(),                            &MainWindow::onWorkspaceSaveAs);
    m_menuRecentWorkspaces = mWorkspace->addMenu(tr("Workspaces Recentes"));
    rebuildRecentWorkspacesMenu();
    mFile->addSeparator();
    mFile->addAction(m_actClose);
    mFile->addSeparator();
    mFile->addAction(m_actExit);

    auto* mEdit = mb->addMenu(tr("&Editar"));
    mEdit->addAction(m_actUndo);
    mEdit->addAction(m_actRedo);
    mEdit->addSeparator();
    mEdit->addAction(m_actCut);
    mEdit->addAction(m_actCopy);
    mEdit->addAction(m_actPaste);
    mEdit->addSeparator();
    mEdit->addAction(m_actSelectAll);
    mEdit->addSeparator();

    auto mkE = [this](QMenu* m, const QString& text, const QKeySequence& sc, auto slot) {
        auto* a = m->addAction(text);
        if (!sc.isEmpty()) a->setShortcut(sc);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    auto* mCase = mEdit->addMenu(tr("Converter &caixa"));
    mkE(mCase, tr("UPPER CASE"),    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_U), &MainWindow::onEditUpperCase);
    mkE(mCase, tr("lower case"),    QKeySequence(Qt::CTRL | Qt::Key_U),             &MainWindow::onEditLowerCase);
    mkE(mCase, tr("Title Case"),    QKeySequence(),                                  &MainWindow::onEditTitleCase);

    auto* mSort = mEdit->addMenu(tr("&Ordenar linhas"));
    mkE(mSort, tr("Ascending"),     QKeySequence(),                                  &MainWindow::onEditSortAsc);
    mkE(mSort, tr("Descending"),    QKeySequence(),                                  &MainWindow::onEditSortDesc);
    mkE(mSort, tr("Unique"),        QKeySequence(),                                  &MainWindow::onEditSortUnique);

    mEdit->addSeparator();
    mkE(mEdit, tr("&Trim Trailing Whitespace"), QKeySequence(),                       &MainWindow::onEditTrimWhitespace);
    mkE(mEdit, tr("&Duplicate Line"),           QKeySequence(Qt::CTRL | Qt::Key_D),   &MainWindow::onEditDuplicateLine);
    mkE(mEdit, tr("Move Line &Up"),             QKeySequence(Qt::ALT | Qt::Key_Up),   &MainWindow::onEditMoveLineUp);
    mkE(mEdit, tr("Move Line &Down"),           QKeySequence(Qt::ALT | Qt::Key_Down), &MainWindow::onEditMoveLineDown);
    mEdit->addSeparator();
    mkE(mEdit, tr("Tabs to Spaces"),            QKeySequence(),                       &MainWindow::onEditTabsToSpaces);
    mkE(mEdit, tr("Spaces to Tabs"),            QKeySequence(),                       &MainWindow::onEditSpacesToTabs);
    mEdit->addSeparator();
    mkE(mEdit, tr("&Formatar Código"),          QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F), &MainWindow::onToolsCodeFormat);
    mEdit->addSeparator();

    // M7: multi-cursor actions.
    auto* mMulti = mEdit->addMenu(tr("Multi&cursor"));
    mkE(mMulti, tr("Adicionar cursor abaixo"),
        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Down),     &MainWindow::onMultiCursorAddBelow);
    mkE(mMulti, tr("Adicionar cursor acima"),
        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Up),       &MainWindow::onMultiCursorAddAbove);
    mMulti->addSeparator();
    mkE(mMulti, tr("Selecionar todas as ocorrências"),
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L),      &MainWindow::onMultiCursorSelectAllOccurrences);
    mkE(mMulti, tr("Adicionar próxima ocorrência"),
        QKeySequence(Qt::ALT | Qt::Key_J),                   &MainWindow::onMultiCursorAddNextOccurrence);

    mEdit->addSeparator();
    if (m_eolMenu) mEdit->addMenu(m_eolMenu->createMenu(this));
    if (m_encodingConvert) mEdit->addMenu(m_encodingConvert->createMenu(this));

    auto* mSearch = mb->addMenu(tr("&Pesquisar"));
    mSearch->addAction(m_actFind);
    mSearch->addAction(m_actReplace);
    mSearch->addSeparator();
    mSearch->addAction(m_actGoToLine);
    mSearch->addSeparator();
    auto mkS = [this](QMenu* m, const QString& text, const QKeySequence& sc, auto slot) {
        auto* a = m->addAction(text);
        if (!sc.isEmpty()) a->setShortcut(sc);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    auto* mBmk = mSearch->addMenu(tr("&Marcadores"));
    mkS(mBmk, tr("Toggle Bookmark"),    QKeySequence(Qt::Key_F2),                       &MainWindow::onBookmarkToggle);
    mkS(mBmk, tr("Next Bookmark"),      QKeySequence(Qt::CTRL | Qt::Key_F2),            &MainWindow::onBookmarkNext);
    mkS(mBmk, tr("Previous Bookmark"),  QKeySequence(Qt::SHIFT | Qt::Key_F2),           &MainWindow::onBookmarkPrev);
    mBmk->addSeparator();
    mkS(mBmk, tr("Bookmark List..."),   QKeySequence(),                                  &MainWindow::onBookmarkList);
    mkS(mBmk, tr("Clear All Bookmarks"),QKeySequence(),                                  &MainWindow::onBookmarkClearAll);

    // M7: color markers. Each "toggle" / "next" action carries the color enum
    // value in QAction::data() so a single slot can dispatch by sender->data().
    auto* mColors = mSearch->addMenu(tr("&Marcadores de Linha"));
    auto addColorBlock = [this, mColors](MarkColor c) {
        auto* sub = mColors->addMenu(ColorMarkers::colorName(c));
        auto* aTog = sub->addAction(tr("Alternar na linha"));
        aTog->setData(static_cast<int>(c));
        connect(aTog, &QAction::triggered, this, &MainWindow::onColorMarkerToggle);
        auto* aNext = sub->addAction(tr("Pular para próximo"));
        aNext->setData(static_cast<int>(c));
        connect(aNext, &QAction::triggered, this, &MainWindow::onColorMarkerNext);
        auto* aClr = sub->addAction(tr("Limpar todos"));
        aClr->setData(static_cast<int>(c));
        connect(aClr, &QAction::triggered, this, &MainWindow::onColorMarkerClearAll);
    };
    addColorBlock(MarkColor::Blue);
    addColorBlock(MarkColor::Red);
    addColorBlock(MarkColor::Green);
    addColorBlock(MarkColor::Yellow);
    addColorBlock(MarkColor::Magenta);

    mSearch->addSeparator();
    mkS(mSearch, tr("Goto &Matching Brace"), QKeySequence(Qt::CTRL | Qt::Key_B),         &MainWindow::onSearchGotoMatchingBrace);
    // M8: LSP navigation. Shortcut is registered separately (createShortcuts in
    // ctor) so the menu entry just exposes it for discoverability.
    mkS(mSearch, tr("Ir para &Definição (LSP)"), QKeySequence(Qt::Key_F12),               &MainWindow::onLspGotoDefinition);
    // M9: LSP find-references / rename live in Search alongside goto-def.
    mkS(mSearch, tr("Encontrar &Referências (LSP)"), QKeySequence(Qt::SHIFT | Qt::Key_F12), &MainWindow::onLspFindReferences);
    mkS(mSearch, tr("&Renomear símbolo (LSP)"),      QKeySequence(Qt::Key_F2),               &MainWindow::onLspRename);
    mkS(mSearch, tr("&Code actions (LSP)"),           QKeySequence(Qt::CTRL | Qt::Key_Period),&MainWindow::onLspCodeActions);

    auto* mView = mb->addMenu(tr("E&xibir"));
    auto* mTheme = mView->addMenu(tr("&Tema"));
    mTheme->addAction(m_actThemeLight);
    mTheme->addAction(m_actThemeDark);
    mTheme->addAction(m_actThemeDracula);
    mTheme->addSeparator();
    auto* mPack = mTheme->addMenu(tr("Pacote de temas"));
    auto* packGroup = new QActionGroup(this);
    packGroup->setExclusive(true);
    auto addPack = [&](ThemePackId id){
        QAction* a = mPack->addAction(ThemePack::displayName(id));
        a->setCheckable(true);
        a->setChecked(ThemePack::loaded() == id);
        a->setData(static_cast<int>(id));
        packGroup->addAction(a);
        connect(a, &QAction::triggered, this, &MainWindow::onThemePackSelected);
    };
    addPack(ThemePackId::None);
    addPack(ThemePackId::SolarizedLight);
    addPack(ThemePackId::SolarizedDark);
    addPack(ThemePackId::Monokai);
    addPack(ThemePackId::Nord);
    mView->addSeparator();
    mView->addAction(m_actToggleLineNumbers);
    mView->addAction(m_actToggleWordWrap);
    mView->addSeparator();
    mView->addAction(m_actSplitHorizontal);
    mView->addAction(m_actSplitVertical);
    mView->addAction(m_actUnsplit);
    mView->addAction(m_actMoveTabToOtherGroup);
    mView->addSeparator();
    auto mkV = [this](QMenu* m, const QString& t, const QKeySequence& sc, auto slot) {
        auto* a = m->addAction(t);
        if (!sc.isEmpty()) a->setShortcut(sc);
        a->setCheckable(true);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    auto* aWs = mkV(mView, tr("Show Whitespace"),     QKeySequence(),  &MainWindow::onViewToggleWhitespace);
    auto* aEol = mkV(mView, tr("Show End of Line"),   QKeySequence(),  &MainWindow::onViewToggleEol);
    auto* aIg = mkV(mView, tr("Show Indent Guides"),  QKeySequence(),  &MainWindow::onViewToggleIndentGuides);
    if (m_whitespaceView) {
        aWs->setChecked(m_whitespaceView->isWhitespaceVisible());
        aEol->setChecked(m_whitespaceView->isEolVisible());
        aIg->setChecked(m_whitespaceView->areIndentGuidesVisible());
    }
    mView->addSeparator();
    if (m_tabExtras) {
        auto* mTab = mView->addMenu(tr("A&bas"));
        mTab->addAction(m_tabExtras->makePinAction(this));
        mTab->addAction(m_tabExtras->makeLockAction(this));
        mTab->addAction(m_tabExtras->makeColorAction(this));
        mTab->addSeparator();
        mTab->addAction(m_tabExtras->makeCloseOthersAction(this));
        mTab->addAction(m_tabExtras->makeCloseToRightAction(this));
        mTab->addAction(m_tabExtras->makeCloseToLeftAction(this));
    }
    mView->addSeparator();
    // M7: Smart Highlight toggle (persisted by SmartHighlight itself).
    m_actSmartHighlight = mView->addAction(tr("Realçar &Ocorrências"));
    m_actSmartHighlight->setCheckable(true);
    m_actSmartHighlight->setChecked(SmartHighlight::shared().isEnabled());
    connect(m_actSmartHighlight, &QAction::triggered, this, &MainWindow::onSmartHighlightToggle);

    // M7: Git diff gutter toggle.
    if (m_gitDiffGutter) {
        m_actGitDiffGutter = mView->addAction(tr("Margem &Git Diff"));
        m_actGitDiffGutter->setCheckable(true);
        m_actGitDiffGutter->setChecked(m_gitDiffGutter->isEnabled());
        connect(m_actGitDiffGutter, &QAction::triggered, this, &MainWindow::onGitDiffGutterToggle);
    }

    // M7: folding submenu.
    if (m_foldingMenu) mView->addMenu(m_foldingMenu->createMenu(this));

    // M7: Zen mode (F11).
    if (m_viewModes) mView->addAction(m_viewModes->zenAction(this));

    mView->addSeparator();
    auto* mPanels = mView->addMenu(tr("&Painéis"));
    mPanels->addAction(m_actToggleFunctionList);
    mPanels->addAction(m_actToggleDocumentMap);
    mPanels->addAction(m_actToggleFileBrowser);
    mPanels->addAction(m_actToggleExecOutput);
    // M9: terminal toggle.
    m_actToggleTerminal = mPanels->addAction(tr("&Terminal"));
    m_actToggleTerminal->setCheckable(true);
    m_actToggleTerminal->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_AsciiTilde));
    connect(m_actToggleTerminal, &QAction::triggered, this, &MainWindow::onViewToggleTerminal);
    if (m_terminalPanel) {
        connect(m_terminalPanel, &QDockWidget::visibilityChanged, this, [this](bool v){
            if (m_actToggleTerminal) m_actToggleTerminal->setChecked(v);
        });
    }
    // M9: FTP panel toggle.
    m_actToggleFtp = mPanels->addAction(tr("Cliente &FTP"));
    m_actToggleFtp->setCheckable(true);
    connect(m_actToggleFtp, &QAction::triggered, this, &MainWindow::onViewToggleFtp);
    if (m_ftpPanel) {
        connect(m_ftpPanel, &QDockWidget::visibilityChanged, this, [this](bool v){
            if (m_actToggleFtp) m_actToggleFtp->setChecked(v);
        });
    }
    // M10: SFTP panel toggle.
    m_actToggleSftp = mPanels->addAction(tr("Cliente &SFTP"));
    m_actToggleSftp->setCheckable(true);
    connect(m_actToggleSftp, &QAction::triggered, this, &MainWindow::onViewToggleSftp);
    if (m_sftpPanel) {
        connect(m_sftpPanel, &QDockWidget::visibilityChanged, this, [this](bool v){
            if (m_actToggleSftp) m_actToggleSftp->setChecked(v);
        });
    }
    // M10: Git log panel toggle.
    m_actToggleGitLog = mPanels->addAction(tr("Git &Log"));
    m_actToggleGitLog->setCheckable(true);
    connect(m_actToggleGitLog, &QAction::triggered, this, &MainWindow::onViewToggleGitLog);
    if (m_gitLogPanel) {
        connect(m_gitLogPanel, &QDockWidget::visibilityChanged, this, [this](bool v){
            if (m_actToggleGitLog) m_actToggleGitLog->setChecked(v);
        });
    }
    // M7: LSP diagnostics dock toggle.
    if (m_lspDiagnosticsPanel) {
        m_actToggleLspDiagnostics = mPanels->addAction(tr("Diagnósticos &LSP"));
        m_actToggleLspDiagnostics->setCheckable(true);
        m_actToggleLspDiagnostics->setChecked(false);
        connect(m_actToggleLspDiagnostics, &QAction::toggled,
                m_lspDiagnosticsPanel, &QWidget::setVisible);
        connect(m_lspDiagnosticsPanel, &QDockWidget::visibilityChanged, this, [this](bool v){
            if (m_actToggleLspDiagnostics) m_actToggleLspDiagnostics->setChecked(v);
        });
    }

    // Languages menu — created lazily after constructor (LanguagesMenu created in ctor body before this).
    if (!m_languagesMenu) m_languagesMenu = new LanguagesMenu(this);
    auto* mLangs = m_languagesMenu->createMenu(this);
    mb->addMenu(mLangs);
    connect(m_languagesMenu, &LanguagesMenu::languageSelected, this, &MainWindow::onLanguageSelected);

    // ─── Tools (enxuto: ≤25 itens, sem duplicar o que está em Git/Mode/AI/Refator) ───
    auto* mTools = mb->addMenu(tr("&Ferramentas"));
    auto mkT = [this](QMenu* m, const QString& text, const QKeySequence& sc, auto slot) {
        auto* a = m->addAction(text);
        if (!sc.isEmpty()) a->setShortcut(sc);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    // Visualizadores rápidos (dialogs).
    mTools->addAction(m_actCompare);
    mTools->addAction(m_actCssPreview);
    mTools->addAction(m_actCsvView);
    mTools->addAction(m_actMarkdownPreview);
    mTools->addAction(m_actHexViewer);
    mTools->addSeparator();
    // Texto/format.
    mkT(mTools, tr("&Word Count..."),    QKeySequence(),                       &MainWindow::onToolsWordCount);
    auto* mFmt = mTools->addMenu(tr("&Formatar"));
    mkT(mFmt,   tr("JSON: Pretty"),      QKeySequence(),                       &MainWindow::onToolsJsonPretty);
    mkT(mFmt,   tr("JSON: Minify"),      QKeySequence(),                       &MainWindow::onToolsJsonMinify);
    mFmt->addSeparator();
    mkT(mFmt,   tr("XML: Pretty"),       QKeySequence(),                       &MainWindow::onToolsXmlPretty);
    mkT(mFmt,   tr("XML: Minify"),       QKeySequence(),                       &MainWindow::onToolsXmlMinify);
    mTools->addSeparator();
    // Macros / snippets / hash.
    mkT(mTools, tr("&Macros..."),        QKeySequence(),                       &MainWindow::onToolsMacroDialog);
    mkT(mTools, tr("&Snippets..."),      QKeySequence(),                       &MainWindow::onToolsSnippets);
    mkT(mTools, tr("Check&sums..."),     QKeySequence(),                       &MainWindow::onToolsHash);
    {
        auto* aSpell = mTools->addAction(tr("Spell &Check"));
        aSpell->setCheckable(true);
        if (m_spellChecker) aSpell->setChecked(m_spellChecker->isEnabled());
        connect(aSpell, &QAction::triggered, this, &MainWindow::onToolsToggleSpellCheck);
    }
    mTools->addSeparator();
    // Busca / exec.
    mTools->addAction(m_actFindInFiles);
    mTools->addAction(m_actRunCommand);
    mkT(mTools, tr("Conectar via SSH…"), QKeySequence(), &MainWindow::onToolsSshConnect);
    mTools->addSeparator();
    // LSP submenu (consolidado).
    auto* mLsp = mTools->addMenu(tr("&LSP"));
    mkT(mLsp, tr("Completar"),         QKeySequence(Qt::CTRL | Qt::Key_Space),                 &MainWindow::onLspTriggerCompletion);
    mkT(mLsp, tr("Hover"),             QKeySequence(Qt::CTRL | Qt::Key_K),                     &MainWindow::onLspShowHover);
    mkT(mLsp, tr("Ajuda de assinatura"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Space),   &MainWindow::onLspSignatureHelp);
    mkT(mLsp, tr("Símbolos do documento"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O),     &MainWindow::onLspDocumentSymbols);
    mkT(mLsp, tr("Símbolos do workspace"), QKeySequence(Qt::CTRL | Qt::Key_T),                 &MainWindow::onLspWorkspaceSymbols);
    mkT(mLsp, tr("Inlay hints — atualizar"), QKeySequence(),                                   &MainWindow::onLspInlayHintsRefresh);
    // Tarefas / build.
    mkT(mTools, tr("Rodar tarefa…"),     QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), &MainWindow::onToolsTasksRun);
    mkT(mTools, tr("Editor de tema…"),   QKeySequence(),                                  &MainWindow::onToolsThemeEditor);
    // Validações.
    mkT(mTools, tr("Carregar coverage (lcov/gcov)…"), QKeySequence(), &MainWindow::onToolsLoadCoverage);
    mkT(mTools, tr("Validar contra JSON Schema…"),    QKeySequence(), &MainWindow::onToolsValidateJsonSchema);
    mkT(mTools, tr("Métricas de código…"),            QKeySequence(), &MainWindow::onToolsCodeMetrics);
    mTools->addSeparator();
    // Layout presets (M26).
    auto* mLayout = mTools->addMenu(tr("&Layouts"));
    mkT(mLayout, tr("Salvar layout…"),  QKeySequence(), &MainWindow::onToolsSaveLayout);
    mkT(mLayout, tr("Aplicar layout…"), QKeySequence(), &MainWindow::onToolsApplyLayout);
    // Atalhos / paleta / preferências.
    mkT(mTools, tr("Atalhos de &teclado…"), QKeySequence(), &MainWindow::onToolsKeybindings);
    mTools->addAction(m_actCommandPalette);
    mTools->addAction(m_actPreferences);

    // M9: Plugins menu. Empty until plugin code calls notepadpp.ui.add_action().
    m_menuPlugins = mb->addMenu(tr("&Plugins"));
    {
        auto* aMgr = m_menuPlugins->addAction(tr("Gerenciar plugins…"));
        connect(aMgr, &QAction::triggered, this, &MainWindow::onToolsPluginManager);
        auto* aOpen = m_menuPlugins->addAction(tr("Abrir pasta de plugins…"));
        connect(aOpen, &QAction::triggered, this, [this]() {
            const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                              + QStringLiteral("/plugins");
            QDir().mkpath(dir);
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        });
        auto* aExamples = m_menuPlugins->addAction(tr("Ver exemplos…"));
        connect(aExamples, &QAction::triggered, this, [this]() {
            // tenta install dir, depois dev dir.
            const QStringList candidates = {
                QStringLiteral("/usr/share/lordpad/plugins-examples"),
                QStringLiteral("/usr/local/share/lordpad/plugins-examples"),
                QCoreApplication::applicationDirPath() + QStringLiteral("/../PowerEditor-qt/plugins/examples"),
                QCoreApplication::applicationDirPath() + QStringLiteral("/plugins/examples"),
            };
            for (const QString& c : candidates) {
                if (QDir(c).exists()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(c));
                    return;
                }
            }
            QMessageBox::information(this, tr("Plugins"),
                tr("Pasta de exemplos não encontrada. Verifique a instalação."));
        });
        m_menuPlugins->addSeparator();
    }

    // ─── M27–M35 + UI/UX: top-level Git / Mode / Panels / AI / Painéis-novos ───
    // Mantemos as entradas em Tools (back-compat) e duplicamos nas barras de
    // topo só quando faz sentido — QAction permite estar em mais de um menu.
    auto mkTop = [this](QMenu* m, const QString& text, const QKeySequence& sc, auto slot) {
        auto* a = m->addAction(text);
        if (!sc.isEmpty()) a->setShortcut(sc);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };

    // Git top-level.
    auto* mGitTop = mb->addMenu(tr("&Git"));
    mkTop(mGitTop, tr("Status (panel)"),     QKeySequence(), &MainWindow::onViewToggleGitLog);
    mkTop(mGitTop, tr("Blame da linha"),     QKeySequence(), &MainWindow::onToolsGitBlame);
    mkTop(mGitTop, tr("Commit…"),            QKeySequence(), &MainWindow::onToolsGitCommit);
    mkTop(mGitTop, tr("Branches…"),          QKeySequence(), &MainWindow::onToolsGitBranches);
    mkTop(mGitTop, tr("Stash…"),             QKeySequence(), &MainWindow::onToolsGitStash);
    mGitTop->addSeparator();
    mkTop(mGitTop, tr("Fetch"),              QKeySequence(), &MainWindow::onToolsGitFetch);
    mkTop(mGitTop, tr("Pull"),               QKeySequence(), &MainWindow::onToolsGitPull);
    mkTop(mGitTop, tr("Push…"),              QKeySequence(), &MainWindow::onToolsGitPush);
    mGitTop->addSeparator();
    mkTop(mGitTop, tr(".gitignore…"),        QKeySequence(), &MainWindow::onToolsGitignoreEdit);
    mkTop(mGitTop, tr("Pre-commit hook…"),   QKeySequence(), &MainWindow::onToolsPreCommitInstall);
    mkTop(mGitTop, tr("Aplicar patch…"),     QKeySequence(), &MainWindow::onToolsApplyPatch);

    // Mode (toggles).
    auto* mModeTop = mb->addMenu(tr("&Modo"));
    auto addCheckMode = [this, mModeTop](const QString& text, bool initial, auto slot) {
        auto* a = mModeTop->addAction(text);
        a->setCheckable(true);
        a->setChecked(initial);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };
    addCheckMode(tr("Vim"),                m_vim ? m_vim->isEnabled() : false,
                 &MainWindow::onToolsToggleVimMode);
    addCheckMode(tr("Typewriter"),         m_typewriter ? m_typewriter->isEnabled() : false,
                 &MainWindow::onViewToggleTypewriter);
    addCheckMode(tr("Auto-pair"),          m_autoPair ? m_autoPair->isEnabled() : true,
                 &MainWindow::onToolsToggleAutoPair);
    addCheckMode(tr("Smart indent"),       m_smartIndent ? m_smartIndent->isEnabled() : true,
                 &MainWindow::onToolsToggleSmartIndent);
    addCheckMode(tr("AI ghost completion"), m_aiGhost ? m_aiGhost->isEnabled() : false,
                 &MainWindow::onToolsToggleAiGhost);
    addCheckMode(tr("Format on save"),     m_formatOnSave ? m_formatOnSave->isEnabled() : false,
                 &MainWindow::onToolsToggleFormatOnSave);
    addCheckMode(tr("AutoCorrect"),        m_autoCorrect ? m_autoCorrect->isEnabled() : true,
                 &MainWindow::onToolsToggleAutoCorrect);

    // ─── Painéis divididos: Painéis / Dados / Rede / DevOps / Util (≤25 cada) ───
    auto addToggle = [this](QMenu* m, const QString& text, QDockWidget* dock, auto slot) {
        auto* a = m->addAction(text);
        a->setCheckable(true);
        if (dock) a->setChecked(dock->isVisible());
        connect(a, &QAction::triggered, this, slot);
        if (dock) connect(dock, &QDockWidget::visibilityChanged, a, &QAction::setChecked);
        return a;
    };

    // Painéis (núcleo + visual + análise de código).
    auto* mPainel = mb->addMenu(tr("&Painéis"));
    addToggle(mPainel, tr("Function list"),       m_functionListPanel,  &MainWindow::onViewToggleFunctionList);
    addToggle(mPainel, tr("Document map"),        m_documentMapPanel,   &MainWindow::onViewToggleDocumentMap);
    addToggle(mPainel, tr("File browser"),        m_fileBrowserPanel,   &MainWindow::onViewToggleFileBrowser);
    addToggle(mPainel, tr("Exec output"),         m_execOutputPanel,    &MainWindow::onViewToggleExecOutput);
    addToggle(mPainel, tr("Terminal"),            m_terminalPanel,      &MainWindow::onViewToggleTerminal);
    mPainel->addSeparator();
    addToggle(mPainel, tr("Cores / paleta"),      m_colorPalette,       &MainWindow::onViewToggleColorPalette);
    addToggle(mPainel, tr("Imagem"),              m_imagePanel,         &MainWindow::onViewToggleImageViewer);
    addToggle(mPainel, tr("Anotar imagem"),       m_imgAnnot,           &MainWindow::onViewToggleImgAnnot);
    addToggle(mPainel, tr("Screenshot"),          m_screenshot,         &MainWindow::onViewToggleScreenshot);
    addToggle(mPainel, tr("Mermaid"),             m_mermaidPanel,       &MainWindow::onViewToggleMermaid);
    addToggle(mPainel, tr("Mind map"),            m_mindMap,            &MainWindow::onViewToggleMindMap);
    addToggle(mPainel, tr("HTML preview"),        m_htmlPrev,           &MainWindow::onViewToggleHtmlPrev);
    addToggle(mPainel, tr("Doc preview"),         m_docPreview,         &MainWindow::onViewToggleDocPreview);
    mPainel->addSeparator();
    addToggle(mPainel, tr("Code review"),         m_review,             &MainWindow::onViewToggleReview);
    addToggle(mPainel, tr("Code clones"),         m_codeClones,         &MainWindow::onViewToggleCodeClones);
    addToggle(mPainel, tr("Call graph"),          m_callGraph,          &MainWindow::onViewToggleCallGraph);
    addToggle(mPainel, tr("Secret scanner"),      m_secrets,            &MainWindow::onViewToggleSecrets);
    addToggle(mPainel, tr("TODOs"),               m_todoAgg,            &MainWindow::onViewToggleTodoAgg);
    addToggle(mPainel, tr("Grep"),                m_grep,               &MainWindow::onViewToggleGrep);
    addToggle(mPainel, tr("Regex"),               m_regexPanel,         &MainWindow::onViewToggleRegexTester);
    addToggle(mPainel, tr("Merge resolver"),      m_mergePanel,         &MainWindow::onViewToggleMergeResolver);
    addToggle(mPainel, tr("Sistema (info)"),      m_sysInfoPanel,       &MainWindow::onViewToggleSysInfo);
    addToggle(mPainel, tr("Sistema (top)"),       m_sysmon,             &MainWindow::onViewToggleSysmon);
    addToggle(mPainel, tr("DevTools"),            m_devTools,           &MainWindow::onViewToggleDevTools);
    addToggle(mPainel, tr("AsciiDoc preview"),    m_adoc,               &MainWindow::onViewToggleAdoc);
    addToggle(mPainel, tr("OpenAPI browser"),     m_apiBrowser,         &MainWindow::onViewToggleApiBrowser);

    // Dados.
    auto* mDados = mb->addMenu(tr("&Dados"));
    addToggle(mDados, tr("SQLite"),              m_sqlitePanel,         &MainWindow::onViewToggleSqlite);
    addToggle(mDados, tr("DB shell"),            m_dbShell,             &MainWindow::onViewToggleDbShell);
    addToggle(mDados, tr("CLI DB"),              m_cliShell,            &MainWindow::onViewToggleCliShell);
    addToggle(mDados, tr("JSONL"),               m_jsonl,               &MainWindow::onViewToggleJsonLines);
    addToggle(mDados, tr("JSON path"),           m_jsonPathPanel,       &MainWindow::onViewToggleJsonPath);
    addToggle(mDados, tr("jq"),                  m_jq,                  &MainWindow::onViewToggleJq);
    addToggle(mDados, tr("MD Table"),            m_mdTable,             &MainWindow::onViewToggleMdTable);
    addToggle(mDados, tr("CSV chart"),           m_csvChart,            &MainWindow::onViewToggleCsvChart);
    addToggle(mDados, tr("SQL Schema"),          m_sqlSchema,           &MainWindow::onViewToggleSqlSchema);
    addToggle(mDados, tr("YAML"),                m_yamlVal,             &MainWindow::onViewToggleYamlVal);
    addToggle(mDados, tr("Hex"),                 m_hexPanel,            &MainWindow::onViewToggleHex);
    addToggle(mDados, tr(".env"),                m_envMgr,              &MainWindow::onViewToggleEnvMgr);
    addToggle(mDados, tr("Archive"),             m_archive,             &MainWindow::onViewToggleArchive);
    addToggle(mDados, tr("Clipboard"),           m_clipboard,           &MainWindow::onViewToggleClipboard);
    addToggle(mDados, tr("GPG"),                 m_gpg,                 &MainWindow::onViewToggleGpg);
    addToggle(mDados, tr("SSL Cert"),            m_ssl,                 &MainWindow::onViewToggleSsl);

    // Rede.
    auto* mRede = mb->addMenu(tr("&Rede"));
    addToggle(mRede, tr("FTP"),                  m_ftpPanel,            &MainWindow::onViewToggleFtp);
    addToggle(mRede, tr("SFTP"),                 m_sftpPanel,           &MainWindow::onViewToggleSftp);
    addToggle(mRede, tr("SSH Exec"),             m_sshExec,             &MainWindow::onViewToggleSshExec);
    addToggle(mRede, tr("REST"),                 m_restPanel,           &MainWindow::onViewToggleRest);
    addToggle(mRede, tr("GraphQL"),              m_graphql,             &MainWindow::onViewToggleGraphQl);
    addToggle(mRede, tr("RSS"),                  m_rss,                 &MainWindow::onViewToggleRss);
    addToggle(mRede, tr("Pastebin"),             m_pastebin,            &MainWindow::onViewTogglePastebin);
    addToggle(mRede, tr("Gist"),                 m_gist,                &MainWindow::onViewToggleGist);
    addToggle(mRede, tr("Git Log"),              m_gitLogPanel,         &MainWindow::onViewToggleGitLog);
    addToggle(mRede, tr("Tradução .po"),         m_poPanel,             &MainWindow::onViewTogglePoEditor);
    addToggle(mRede, tr("SSHFS (montar remoto)"),m_sshfs,               &MainWindow::onViewToggleSshfs);
    addToggle(mRede, tr("Túneis SSH"),           m_tunnels,             &MainWindow::onViewToggleTunnels);
    addToggle(mRede, tr("Portas (ss/scan)"),     m_portScan,            &MainWindow::onViewTogglePortScan);

    // DevOps.
    auto* mDevOps = mb->addMenu(tr("Dev&Ops"));
    addToggle(mDevOps, tr("Build watch"),        m_buildWatch,          &MainWindow::onViewToggleBuildWatch);
    addToggle(mDevOps, tr("Cron"),               m_cron,                &MainWindow::onViewToggleCron);
    addToggle(mDevOps, tr("File watcher"),       m_watcher,             &MainWindow::onViewToggleWatcher);
    addToggle(mDevOps, tr("Profiler"),           m_profile,             &MainWindow::onViewToggleProfile);
    addToggle(mDevOps, tr("Test runner"),        m_testRunner,          &MainWindow::onViewToggleTestRunner);
    addToggle(mDevOps, tr("Docker"),             m_docker,              &MainWindow::onViewToggleDocker);
    addToggle(mDevOps, tr("kubectl"),            m_kubectl,             &MainWindow::onViewToggleKubectl);
    addToggle(mDevOps, tr("systemd"),            m_systemd,             &MainWindow::onViewToggleSystemd);
    addToggle(mDevOps, tr("Log tail"),           m_logTail,             &MainWindow::onViewToggleLogTail);

    // Util.
    auto* mUtil = mb->addMenu(tr("&Util"));
    addToggle(mUtil, tr("Calendário"),           m_calendarPanel,       &MainWindow::onViewToggleCalendar);
    addToggle(mUtil, tr("Calculadora"),          m_calculatorPanel,     &MainWindow::onViewToggleCalculator);
    addToggle(mUtil, tr("QR"),                   m_qrPanel,             &MainWindow::onViewToggleQr);
    addToggle(mUtil, tr("TODO"),                 m_todoPanel,           &MainWindow::onViewToggleTodo);
    addToggle(mUtil, tr("Notas (CherryTree)"),   m_notesPanel,          &MainWindow::onViewToggleNotes);
    addToggle(mUtil, tr("Notebook"),             m_notebookPanel,       &MainWindow::onViewToggleNotebook);
    addToggle(mUtil, tr("Cronômetro / Pomodoro"), m_timer,              &MainWindow::onViewToggleTimer);
    addToggle(mUtil, tr("Time tracker"),         m_timeTracker,         &MainWindow::onViewToggleTimeTracker);
    addToggle(mUtil, tr("Conversor"),            m_unitPanel,           &MainWindow::onViewToggleUnitConverter);
    addToggle(mUtil, tr("Cheatsheets"),          m_cheats,              &MainWindow::onViewToggleCheats);
    addToggle(mUtil, tr("Cofre de senhas"),      m_vault,               &MainWindow::onViewToggleVault);
    addToggle(mUtil, tr("Estatísticas"),         m_textStats,           &MainWindow::onViewToggleTextStats);

    // AI top-level.
    auto* mAiTop = mb->addMenu(tr("&AI"));
    mkTop(mAiTop, tr("Painel AI"),               QKeySequence(),                          &MainWindow::onViewToggleAi);
    mAiTop->addSeparator();
    mkTop(mAiTop, tr("Explicar seleção"),        QKeySequence(),                          &MainWindow::onToolsAiExplain);
    mkTop(mAiTop, tr("Traduzir seleção…"),       QKeySequence(),                          &MainWindow::onToolsAiTranslate);
    mkTop(mAiTop, tr("Mensagem de commit…"),     QKeySequence(),                          &MainWindow::onToolsAiCommitMsg);

    // M29: smart selection no menu Edit já existente.
    {
        auto* mEdit = mb->findChild<QMenu*>(QString());
        // Não: criar um submenu "Refator" aqui no top-level do menu bar.
        auto* mRefactor = mb->addMenu(tr("Re&fator"));
        // Shortcuts: shift+up/down menos disputado; rename-in-scope via Ctrl+Alt+R.
        mkTop(mRefactor, tr("Expandir seleção"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up),    &MainWindow::onEditExpandSelection);
        mkTop(mRefactor, tr("Encolher seleção"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down),  &MainWindow::onEditShrinkSelection);
        mkTop(mRefactor, tr("Renomear no escopo…"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_R),    &MainWindow::onEditRenameInScope);
        // Extract-function: já tem Ctrl+Alt+E no Tools menu — não repetir aqui.
        mkTop(mRefactor, tr("Extrair função…"), QKeySequence(),                                       &MainWindow::onToolsExtractFunction);
        Q_UNUSED(mEdit);
    }

    auto* mHelp = mb->addMenu(tr("A&juda"));
    mHelp->addAction(m_actAbout);
}

void MainWindow::createToolBar() {
    // Helper: prefer freedesktop themed icon (Adwaita/Breeze/Papirus all expose
    // these names); fall back to QStyle's bundled standardIcon so the toolbar
    // is never blank even on minimal icon themes.
    auto themed = [this](const char* name, QStyle::StandardPixmap fallback) -> QIcon {
        QIcon i = QIcon::fromTheme(QString::fromLatin1(name));
        if (i.isNull()) i = style()->standardIcon(fallback);
        return i;
    };

    auto* tb = addToolBar(tr("Main Toolbar"));
    tb->setObjectName(QStringLiteral("MainToolBar"));   // needed by saveState/restoreState
    tb->setMovable(true);
    tb->setIconSize(QSize(20, 20));
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // Set icons on the actions we already created in createActions(). Qt picks them
    // up automatically wherever the action appears (menu + toolbar share state).
    m_actNew      ->setIcon(themed("document-new",          QStyle::SP_FileIcon));
    m_actOpen     ->setIcon(themed("document-open",         QStyle::SP_DirOpenIcon));
    m_actSave     ->setIcon(themed("document-save",         QStyle::SP_DialogSaveButton));
    m_actSaveAs   ->setIcon(themed("document-save-as",      QStyle::SP_DialogSaveButton));
    m_actCut      ->setIcon(themed("edit-cut",              QStyle::SP_FileLinkIcon));
    m_actCopy     ->setIcon(themed("edit-copy",             QStyle::SP_FileIcon));
    m_actPaste    ->setIcon(themed("edit-paste",            QStyle::SP_FileIcon));
    m_actUndo     ->setIcon(themed("edit-undo",             QStyle::SP_ArrowBack));
    m_actRedo     ->setIcon(themed("edit-redo",             QStyle::SP_ArrowForward));
    m_actFind     ->setIcon(themed("edit-find",             QStyle::SP_FileDialogContentsView));
    m_actReplace  ->setIcon(themed("edit-find-replace",     QStyle::SP_FileDialogContentsView));
    m_actFindInFiles->setIcon(themed("system-search",       QStyle::SP_FileDialogContentsView));
    m_actRunCommand ->setIcon(themed("system-run",          QStyle::SP_MediaPlay));
    m_actCommandPalette->setIcon(themed("edit-find",        QStyle::SP_TitleBarMenuButton));
    m_actClose    ->setIcon(themed("window-close",          QStyle::SP_DialogCloseButton));

    tb->addAction(m_actNew);
    tb->addAction(m_actOpen);
    tb->addAction(m_actSave);
    tb->addAction(m_actSaveAs);
    tb->addSeparator();
    tb->addAction(m_actCut);
    tb->addAction(m_actCopy);
    tb->addAction(m_actPaste);
    tb->addSeparator();
    tb->addAction(m_actUndo);
    tb->addAction(m_actRedo);
    tb->addSeparator();
    tb->addAction(m_actFind);
    tb->addAction(m_actReplace);
    tb->addAction(m_actFindInFiles);
    tb->addSeparator();
    tb->addAction(m_actGoToLine);

    // M14: extra toolbar buttons created on the fly. They live only on the
    // toolbar (not in any menu) — the menu items already trigger the same
    // slots, so we don't add duplicate menu entries.
    auto addTbAction = [this, tb, themed](const QString& name,
                                          const QString& tip,
                                          QStyle::StandardPixmap fb,
                                          auto slot) {
        auto* a = new QAction(themed(name.toUtf8().constData(), fb), tip, this);
        a->setToolTip(tip);
        connect(a, &QAction::triggered, this, slot);
        tb->addAction(a);
        return a;
    };
    addTbAction(QStringLiteral("format-text-bold"), tr("Formatar código"),
                QStyle::SP_FileDialogDetailedView, &MainWindow::onToolsCodeFormat);
    addTbAction(QStringLiteral("bookmark-new"), tr("Toggle bookmark"),
                QStyle::SP_DialogYesButton, &MainWindow::onBookmarkToggle);
    addTbAction(QStringLiteral("media-record"), tr("Macros…"),
                QStyle::SP_MediaPlay, &MainWindow::onToolsMacroDialog);

    tb->addSeparator();
    addTbAction(QStringLiteral("media-playback-start"), tr("Rodar tarefa…"),
                QStyle::SP_MediaPlay, &MainWindow::onToolsTasksRun);
    addTbAction(QStringLiteral("emblem-success"), tr("Testes"),
                QStyle::SP_DialogApplyButton, &MainWindow::onViewToggleTestRunner);
    addTbAction(QStringLiteral("vcs-commit"), tr("Git commit…"),
                QStyle::SP_DialogSaveButton, &MainWindow::onToolsGitCommit);

    // M27–M35: ícones extra (mais funções na toolbar).
    tb->addSeparator();
    addTbAction(QStringLiteral("vcs-pull"), tr("Git pull"),
                QStyle::SP_ArrowDown,   &MainWindow::onToolsGitPull);
    addTbAction(QStringLiteral("vcs-push"), tr("Git push"),
                QStyle::SP_ArrowUp,     &MainWindow::onToolsGitPush);
    addTbAction(QStringLiteral("vcs-update-required"), tr("Git fetch"),
                QStyle::SP_BrowserReload, &MainWindow::onToolsGitFetch);

    tb->addSeparator();
    addTbAction(QStringLiteral("preferences-desktop-keyboard-shortcuts"), tr("Quick Switch (Ctrl+P)"),
                QStyle::SP_FileDialogContentsView, &MainWindow::onToolsQuickSwitch);
    addTbAction(QStringLiteral("edit-select-all"), tr("Expandir seleção"),
                QStyle::SP_ArrowRight,  &MainWindow::onEditExpandSelection);
    addTbAction(QStringLiteral("edit-rename"), tr("Renomear no escopo…"),
                QStyle::SP_FileLinkIcon, &MainWindow::onEditRenameInScope);
    addTbAction(QStringLiteral("zoom-original"), tr("Métricas de código…"),
                QStyle::SP_FileDialogInfoView, &MainWindow::onToolsCodeMetrics);

    tb->addSeparator();
    addTbAction(QStringLiteral("network-connect"), tr("REST"),
                QStyle::SP_ComputerIcon, &MainWindow::onViewToggleRest);
    addTbAction(QStringLiteral("preferences-system"), tr("DevTools"),
                QStyle::SP_DialogResetButton, &MainWindow::onViewToggleDevTools);
    addTbAction(QStringLiteral("dialog-information"), tr("AI"),
                QStyle::SP_MessageBoxInformation, &MainWindow::onViewToggleAi);
    addTbAction(QStringLiteral("appointment-new"), tr("Cronômetro"),
                QStyle::SP_MediaSeekForward, &MainWindow::onViewToggleTimer);

    tb->addSeparator();
    tb->addAction(m_actRunCommand);
    tb->addAction(m_actCommandPalette);
}

void MainWindow::createStatusBar() {
    m_statusPosition = new QLabel(tr("Ln 1, Col 1"), this);
    m_statusEncoding = new QLabel(tr("UTF-8"), this);
    m_statusEol      = new QLabel(tr("LF"), this);
    m_statusLanguage = new QLabel(tr("Plain Text"), this);
    m_statusGit      = new QLabel(QString(), this);
    m_statusGit->setToolTip(tr("Status do Git (branch / arquivo)"));
    m_statusEncoding->setToolTip(tr("Codificação atual — alterar em Edit › Codificação"));
    m_statusEol     ->setToolTip(tr("Tipo de quebra de linha — alterar em Edit › EOL"));
    m_statusLanguage->setToolTip(tr("Linguagem/lexer — alterar no menu Languages"));

    // M14: VIM mode label — empty string means vim mode is off.
    m_statusVim = new QLabel(this);
    m_statusVim->setStyleSheet(QStringLiteral("QLabel { color: #ff9966; font-weight: bold; }"));

    statusBar()->addPermanentWidget(m_statusGit);
    statusBar()->addPermanentWidget(m_statusVim);
    statusBar()->addPermanentWidget(m_statusLanguage);
    statusBar()->addPermanentWidget(m_statusPosition);
    statusBar()->addPermanentWidget(m_statusEncoding);
    statusBar()->addPermanentWidget(m_statusEol);
}

// ---------------------------------------------------------------------------
// Tab helpers
// ---------------------------------------------------------------------------
EditorTab* MainWindow::currentTab() const   { return m_multiView->currentTab(); }
EditorTab* MainWindow::tabAt(int index) const { return m_multiView->tabAt(index); }

int MainWindow::findTabByPath(const QString& path) const {
    if (path.isEmpty()) return -1;
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) {
        if (auto* t = tabAt(i); t && t->filePath() == path) return i;
    }
    return -1;
}

void MainWindow::setTabTitle(EditorTab* tab, const QString& title) {
    if (!tab) return;
    auto loc = m_multiView->locateTab(tab);
    if (loc.first && loc.second >= 0) loc.first->setTabText(loc.second, title);
}

void MainWindow::setTabTooltip(EditorTab* tab, const QString& tooltip) {
    if (!tab) return;
    auto loc = m_multiView->locateTab(tab);
    if (loc.first && loc.second >= 0) loc.first->setTabToolTip(loc.second, tooltip);
}

void MainWindow::setActiveTab(EditorTab* tab) {
    if (!tab) return;
    auto loc = m_multiView->locateTab(tab);
    if (loc.first && loc.second >= 0) loc.first->setCurrentIndex(loc.second);
}

void MainWindow::connectTabSignals(EditorTab* tab) {
    connect(tab, &EditorTab::modificationChanged, this, &MainWindow::onCurrentTabModified);
    connect(tab, &EditorTab::filePathChanged, this, &MainWindow::onCurrentTabFilePathChanged);
    connect(tab, &EditorTab::cursorPositionChanged, this, &MainWindow::onCursorPositionChanged);
    if (auto* sci = tab->editor()) {
        UrlHyperlink::installFor(sci);
        // M7: per-editor wiring.
        MultiCursor::installFor(sci);
        SmartHighlight::installFor(sci);
        FoldingMenu::enableFolding(sci);
        // M11: bracket pair colorization.
        BracketColors::installFor(sci);
        // M14: typewriter + vim mode (no-op when toggles are off).
        if (m_typewriter) m_typewriter->attach(sci);
        if (m_vim)        m_vim->attach(sci);
        // M16: coverage gutter — actual hits/misses arrive when the user
        // loads an lcov.info / .gcov file; until then the margin stays empty.
        if (m_coverage)   m_coverage->attach(sci, tab->filePath());
        // M17: auto-pair + smart indent (toggleable from Tools menu).
        if (m_autoPair)    m_autoPair->attach(sci);
        if (m_smartIndent) m_smartIndent->attach(sci);
        // M21: AI ghost completion.
        if (m_aiGhost)     m_aiGhost->attach(sci);
        // M27: AutoCorrect.
        if (m_autoCorrect) m_autoCorrect->attach(sci);
        // M10: hand Tab / Shift+Tab / Esc to MainWindow::eventFilter so snippet
        // sessions can intercept them before Scintilla's default handling.
        sci->installEventFilter(this);
        // didChange-on-modify (LspClient debounces internally).
        connect(sci, &ScintillaEdit::modified, this, [this, tab](
                    Scintilla::ModificationFlags type,
                    Scintilla::Position /*position*/,
                    Scintilla::Position /*length*/,
                    Scintilla::Position /*linesAdded*/,
                    const QByteArray& /*text*/,
                    Scintilla::Position /*line*/,
                    Scintilla::FoldLevel /*foldNow*/,
                    Scintilla::FoldLevel /*foldPrev*/) {
            // Fire only on actual text change (insert / delete), ignoring style/marker noise.
            const auto mask = Scintilla::ModificationFlags::InsertText
                            | Scintilla::ModificationFlags::DeleteText;
            if (static_cast<int>(type & mask) == 0) return;
            if (!tab) return;
            auto* s = tab->editor();
            if (!s) return;
            // LSP didChange (when applicable).
            if (m_lsp && !tab->filePath().isEmpty()) {
                QByteArray content = s->getText(s->textLength() + 1);
                m_lsp->didChange(tab->filePath(), QString::fromUtf8(content));
            }
            // M9: notify Python plugins (path may be empty for unnamed buffers).
            if (m_pluginHost) m_pluginHost->emitOnTextChanged(tab->filePath());
        });
    }
    registerTabForRecovery(tab);
}

void MainWindow::registerTabForRecovery(EditorTab* tab) {
    if (!m_crashRecovery || !tab || !tab->editor()) return;
    const int bufferId = static_cast<int>(reinterpret_cast<qintptr>(tab) & 0x7FFFFFFF);
    QPointer<EditorTab> safe(tab);
    m_crashRecovery->registerBuffer(bufferId, tab->filePath(), QStringLiteral("UTF-8"),
        [safe]() -> QByteArray {
            if (!safe || !safe->editor()) return {};
            auto* sci = safe->editor();
            return sci->getText(sci->textLength() + 1);
        });
}

void MainWindow::applyEditorPreferences(EditorTab* tab) {
    // Pref-only — DOES NOT touch styles. styleClearAll() wipes lexer-applied styles,
    // so theme + lexer are paired separately in applyThemeAndLexer().
    if (!tab || !tab->editor()) return;
    auto* sci = tab->editor();
    const Settings& s = Settings::instance();

    // M14: per-language overrides. QSettings group "Lang/<lexerName>" can hold
    // {indentWidth, useTabs, eol, encoding} that win over the global defaults.
    int  tabWidth = s.tabWidth();
    bool useTabs  = !s.useSpaces();
    {
        const QString lex = LexerMap::lexerNameForPath(tab->filePath());
        if (!lex.isEmpty()) {
            QSettings ls;
            ls.beginGroup(QStringLiteral("Lang/") + lex);
            if (ls.contains(QStringLiteral("indentWidth")))
                tabWidth = ls.value(QStringLiteral("indentWidth")).toInt();
            if (ls.contains(QStringLiteral("useTabs")))
                useTabs  = ls.value(QStringLiteral("useTabs")).toBool();
            ls.endGroup();
        }
        // M17: .editorconfig overrides everything else.
        if (!tab->filePath().isEmpty()) {
            const auto ec = EditorConfig::settingsFor(tab->filePath());
            if (ec.indent_size > 0) tabWidth = ec.indent_size;
            if (!ec.indent_style.isEmpty()) useTabs = (ec.indent_style == QStringLiteral("tab"));
        }
    }

    sci->setTabWidth(tabWidth);
    sci->setUseTabs(useTabs);
    sci->setMarginWidthN(0, s.showLineNumbers() ? sci->textWidth(33 /*STYLE_LINENUMBER*/, "_9999") + 8 : 0);
    sci->setWrapMode(s.wordWrap() ? 1 /*SC_WRAP_WORD*/ : 0 /*SC_WRAP_NONE*/);

    // M15: column ruler. QSettings/Editor/rulerColumn (default 80, 0 = off).
    {
        QSettings ls;
        const int col = ls.value(QStringLiteral("Editor/rulerColumn"), 80).toInt();
        if (col > 0) {
            sci->setEdgeMode(1 /*EDGE_LINE*/);
            sci->setEdgeColumn(col);
            sci->setEdgeColour(0x004D4D4D);
        } else {
            sci->setEdgeMode(0 /*EDGE_NONE*/);
        }
    }
}

void MainWindow::applyThemeAndLexer(EditorTab* tab) {
    // Apply theme defaults (calls styleClearAll), THEN lexer (overrides per-style colors).
    // Order matters: theme first wipes everything, lexer last wins.
    if (!tab || !tab->editor()) return;
    auto* sci = tab->editor();
    const Settings& s = Settings::instance();
    ThemeManager::applyToScintilla(sci, ThemeManager::current(), s.fontFamily(), s.fontSize());
    LexerMap::applyLexerForPath(sci, tab->filePath());
}

void MainWindow::applyThemeToAllTabs() {
    const auto theme = ThemeManager::current();
    ThemeManager::apply(qApp, theme);
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) {
        if (auto* t = tabAt(i)) applyThemeAndLexer(t);
    }
}

void MainWindow::updateWindowTitle() {
    auto* t = currentTab();
    if (!t) { setWindowTitle(tr("LordPad")); return; }
    QString title = t->displayPath();
    if (t->isModified()) title += " *";
    setWindowTitle(QString("%1 — LordPad").arg(title));
}

void MainWindow::updateStatusBar() {
    auto* t = currentTab();
    if (!t || !t->editor()) {
        m_statusPosition->setText(tr("Ln 1, Col 1"));
        if (m_statusEncoding) m_statusEncoding->setText(tr("UTF-8"));
        if (m_statusEol)      m_statusEol->setText(tr("LF"));
        if (m_statusLanguage) m_statusLanguage->setText(tr("Plain Text"));
        return;
    }
    auto* sci = t->editor();

    const auto pos = sci->currentPos();
    const int line = static_cast<int>(sci->lineFromPosition(pos)) + 1;
    const int col  = static_cast<int>(sci->column(pos)) + 1;
    m_statusPosition->setText(tr("Ln %1, Col %2").arg(line).arg(col));

    if (m_statusEncoding) {
        const auto enc = EncodingConvert::loadFor(t->filePath());
        m_statusEncoding->setText(EncodingConvert::displayName(enc));
    }
    if (m_statusEol) {
        // Scintilla EOL modes: 0 CRLF, 1 CR, 2 LF.
        const int mode = static_cast<int>(sci->eOLMode());
        const QString label = (mode == 0) ? QStringLiteral("CRLF")
                            : (mode == 1) ? QStringLiteral("CR")
                            :               QStringLiteral("LF");
        m_statusEol->setText(label);
    }
    if (m_statusLanguage) {
        const QString lex = LexerMap::lexerNameForPath(t->filePath());
        m_statusLanguage->setText(lex.isEmpty() ? tr("Plain Text") : lex);
    }
}

void MainWindow::rebuildRecentFilesMenu() {
    m_menuRecent->clear();
    const QStringList files = Settings::instance().recentFiles();
    if (files.isEmpty()) {
        auto* a = m_menuRecent->addAction(tr("(empty)"));
        a->setEnabled(false);
        return;
    }
    for (const QString& path : files) {
        auto* a = m_menuRecent->addAction(QFileInfo(path).fileName() + "  " + path);
        a->setData(path);
        connect(a, &QAction::triggered, this, &MainWindow::onRecentFileTriggered);
    }
    m_menuRecent->addSeparator();
    auto* clear = m_menuRecent->addAction(tr("Clear list"));
    connect(clear, &QAction::triggered, this, [this]{
        Settings::instance().clearRecentFiles();
        Settings::instance().save();
        rebuildRecentFilesMenu();
    });
}

// ---------------------------------------------------------------------------
// File ops
// ---------------------------------------------------------------------------
void MainWindow::onFileNew() {
    auto* tab = new EditorTab(this);
    applyEditorPreferences(tab);
    applyThemeAndLexer(tab);
    connectTabSignals(tab);
    m_multiView->addTab(tab, tab->tabTitle());
    setActiveTab(tab);
    if (m_findDialog) m_findDialog->setActiveEditor(tab->editor());
    if (m_languagesMenu) {
        m_languagesMenu->setActiveEditor(tab->editor());
        m_languagesMenu->syncCheckedLanguage(QString());
    }
}

void MainWindow::onFileOpen() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                      tr("All Files (*)"));
    if (path.isEmpty()) return;
    openFile(path);
}

void MainWindow::openFile(const QString& path) {
    if (path.isEmpty()) return;
    int existing = findTabByPath(path);
    if (existing >= 0) { setActiveTab(tabAt(existing)); return; }

    auto* tab = currentTab();
    const bool reuseEmpty = tab && tab->filePath().isEmpty() && !tab->isModified()
                            && tab->editor()->length() == 0;
    if (!reuseEmpty) {
        tab = new EditorTab(this);
        applyEditorPreferences(tab);
        applyThemeAndLexer(tab);
        connectTabSignals(tab);
        m_multiView->addTab(tab, tab->tabTitle());
        setActiveTab(tab);
    }

    loadFileIntoTab(tab, path);
}

void MainWindow::loadFileIntoTab(EditorTab* tab, const QString& path) {
    auto result = FileIO::readFile(path);
    if (!result.ok) { QMessageBox::warning(this, tr("Open failed"), result.error); return; }
    auto* sci = tab->editor();
    sci->setText(result.utf8.constData());
    sci->emptyUndoBuffer();
    tab->setFilePath(path);
    tab->setModified(false);

    applyEditorPreferences(tab);
    applyThemeAndLexer(tab);   // theme first then lexer — order matters (styleClearAll)

    Settings::instance().addRecentFile(path);
    Settings::instance().save();
    rebuildRecentFilesMenu();
    if (m_externalWatcher) m_externalWatcher->watch(path);

    setTabTitle(tab, tab->tabTitle());
    setTabTooltip(tab, path);

    // M8: seed the per-path encoding choice from what FileIO detected on load,
    // unless the user has already pinned a preference for this path. Without
    // this seeding the encoding menu would always show UTF-8 for files that
    // were actually loaded as Windows-1252 / Latin-1 / UTF-16.
    {
        QSettings s("clip52", "LordPad");
        s.beginGroup("FileEncoding");
        const bool hasPinned = s.contains(path);
        s.endGroup();
        if (!hasPinned) {
            QString detected = result.detectedEncoding.isEmpty()
                                   ? QStringLiteral("UTF-8")
                                   : result.detectedEncoding;
            // detectedEncoding is uchardet's name (e.g. "WINDOWS-1252") — map
            // to the canonical id we persist.
            const QString upper = detected.toUpper();
            EncodingConvert::Encoding e = EncodingConvert::Encoding::Utf8;
            if (upper == "UTF-8" && result.hadBOM)            e = EncodingConvert::Encoding::Utf8Bom;
            else if (upper == "UTF-8")                        e = EncodingConvert::Encoding::Utf8;
            else if (upper == "UTF-16LE")                     e = EncodingConvert::Encoding::Utf16LE;
            else if (upper == "UTF-16BE")                     e = EncodingConvert::Encoding::Utf16BE;
            else if (upper == "ISO-8859-1" || upper == "LATIN1") e = EncodingConvert::Encoding::Latin1;
            else if (upper == "WINDOWS-1252")                 e = EncodingConvert::Encoding::Windows1252;
            else if (upper == "WINDOWS-1250")                 e = EncodingConvert::Encoding::Windows1250;
            EncodingConvert::saveFor(path, e);
        }
    }
    if (m_languagesMenu) m_languagesMenu->syncCheckedLanguage(LexerMap::lexerNameForPath(path));

    // M7: attach per-file modules now that the path is known.
    if (m_colorMarkers)  m_colorMarkers->attach(sci, path);
    if (m_gitDiffGutter) m_gitDiffGutter->attach(sci, path);
    if (m_lsp) {
        const QString lexer = LexerMap::lexerNameForPath(path);
        if (m_lsp->isLanguageSupported(lexer)) {
            m_lsp->didOpen(path, lexer, QString::fromUtf8(result.utf8));
        }
    }

    updateWindowTitle();
    updateStatusBar();   // M8: refresh encoding/EOL/lang on load

    // M9: notify plugins.
    if (m_pluginHost) m_pluginHost->emitOnLoad(path);
}

bool MainWindow::saveTab(EditorTab* tab) {
    if (!tab) return false;
    if (tab->filePath().isEmpty()) return saveTabAs(tab);

    // M13: optionally run a language-specific formatter on the buffer before
    // we serialize. Failures are non-fatal — we still persist the unformatted
    // text so the user never loses data because their formatter is absent.
    if (m_formatOnSave) {
        QString fmtErr;
        const QString lex = LexerMap::lexerNameForPath(tab->filePath());
        m_formatOnSave->applyIfEnabled(tab->editor(), lex, tab->filePath(), &fmtErr);
        if (!fmtErr.isEmpty()) statusBar()->showMessage(tr("Formatter: %1").arg(fmtErr), 5000);
    }

    QByteArray bytes = tab->editor()->getText(tab->editor()->textLength() + 1);
    if (m_externalWatcher) m_externalWatcher->notifyOurWrite(tab->filePath());
    QString error;
    // M8: honor the per-path encoding preference (pinned by EncodingConvert or
    // seeded from detectEncoding on load). The buffer is always UTF-8 inside
    // Scintilla; writeFileEncoded transcodes + adds BOM when needed.
    const auto enc       = EncodingConvert::loadFor(tab->filePath());
    const QString codec  = EncodingConvert::codecName(enc);
    const bool   addBOM  = EncodingConvert::wantsBOM(enc);
    if (!FileIO::writeFileEncoded(tab->filePath(), bytes, codec, addBOM, &error)) {
        QMessageBox::warning(this, tr("Save failed"), error);
        return false;
    }
    tab->setModified(false);
    setTabTitle(tab, tab->tabTitle());
    // M7: refresh diff gutter against new HEAD-relative state, persist color markers.
    if (m_gitDiffGutter) m_gitDiffGutter->refresh(tab->editor(), tab->filePath());
    if (m_colorMarkers)  m_colorMarkers->persist(tab->editor());
    // M9: notify plugins.
    if (m_pluginHost) m_pluginHost->emitOnSave(tab->filePath());
    // M9/M10: if the saved buffer is a cached remote file, re-upload to the
    // matching server (FTP or SFTP).
    if (m_ftpPanel  && m_ftpPanel->isRemoteCachedFile(tab->filePath()))
        m_ftpPanel->commitRemoteSave(tab->filePath());
    if (m_sftpPanel && m_sftpPanel->isRemoteCachedFile(tab->filePath()))
        m_sftpPanel->commitRemoteSave(tab->filePath());
    updateWindowTitle();
    return true;
}

bool MainWindow::saveTabAs(EditorTab* tab) {
    if (!tab) return false;
    const QString path = QFileDialog::getSaveFileName(this, tr("Save As"),
        tab->filePath().isEmpty() ? QString() : tab->filePath(), tr("All Files (*)"));
    if (path.isEmpty()) return false;
    tab->setFilePath(path);
    LexerMap::applyLexerForPath(tab->editor(), path);
    if (m_languagesMenu) m_languagesMenu->syncCheckedLanguage(LexerMap::lexerNameForPath(path));
    Settings::instance().addRecentFile(path);
    Settings::instance().save();
    rebuildRecentFilesMenu();
    return saveTab(tab);
}

bool MainWindow::maybeSaveTab(EditorTab* tab) {
    if (!tab || !tab->isModified()) return true;
    const auto ret = QMessageBox::warning(this, tr("Unsaved changes"),
        tr("'%1' has unsaved changes. Save before closing?").arg(tab->displayPath()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return false;
    if (ret == QMessageBox::Save)  return saveTab(tab);
    return true;
}

void MainWindow::onFileSave()    { saveTab(currentTab()); }
void MainWindow::onFileSaveAs()  { saveTabAs(currentTab()); }
void MainWindow::onFileClose()   { onMultiViewTabCloseRequested(currentTab()); }
void MainWindow::onFileExit()    { close(); }

// ---------------------------------------------------------------------------
// Edit ops
// ---------------------------------------------------------------------------
#define WITH_SCI(call) do { if (auto* t = currentTab(); t && t->editor()) t->editor()->call; } while (0)

void MainWindow::onEditUndo()      { WITH_SCI(undo()); }
void MainWindow::onEditRedo()      { WITH_SCI(redo()); }
void MainWindow::onEditCut()       { WITH_SCI(cut()); }
void MainWindow::onEditCopy()      { WITH_SCI(copy()); }
void MainWindow::onEditPaste()     { WITH_SCI(paste()); }
void MainWindow::onEditSelectAll() { WITH_SCI(selectAll()); }

#undef WITH_SCI

// ---------------------------------------------------------------------------
// Search ops
// ---------------------------------------------------------------------------
void MainWindow::onSearchFind() {
    if (auto* t = currentTab()) {
        m_findDialog->setActiveEditor(t->editor());
        m_findDialog->showFind();
    }
}

void MainWindow::onSearchReplace() {
    if (auto* t = currentTab()) {
        m_findDialog->setActiveEditor(t->editor());
        m_findDialog->showReplace();
    }
}

void MainWindow::onSearchGoToLine() {
    auto* t = currentTab();
    if (!t) return;
    GoToLineDialog dlg(t->editor(), this);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// View ops
// ---------------------------------------------------------------------------
void MainWindow::onViewSetThemeLight()   { ThemeManager::apply(qApp, AppTheme::Light);   Settings::instance().setDarkTheme(false); Settings::instance().save(); applyThemeToAllTabs(); }
void MainWindow::onViewSetThemeDark()    { ThemeManager::apply(qApp, AppTheme::Dark);    Settings::instance().setDarkTheme(true);  Settings::instance().save(); applyThemeToAllTabs(); }
void MainWindow::onViewSetThemeDracula() { ThemeManager::apply(qApp, AppTheme::Dracula); Settings::instance().setDarkTheme(true);  Settings::instance().save(); applyThemeToAllTabs(); }

void MainWindow::onViewToggleLineNumbers() {
    Settings::instance().setShowLineNumbers(m_actToggleLineNumbers->isChecked());
    Settings::instance().save();
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) applyEditorPreferences(tabAt(i));   // pref only, lexer styles preserved
}

void MainWindow::onViewToggleWordWrap() {
    Settings::instance().setWordWrap(m_actToggleWordWrap->isChecked());
    Settings::instance().save();
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) applyEditorPreferences(tabAt(i));   // pref only, lexer styles preserved
}

void MainWindow::onViewSplitHorizontal()      { m_multiView->splitView(MultiView::Orientation::Horizontal); }
void MainWindow::onViewSplitVertical()        { m_multiView->splitView(MultiView::Orientation::Vertical); }
void MainWindow::onViewUnsplit()              { m_multiView->unsplit(); }
void MainWindow::onViewMoveTabToOtherGroup()  { m_multiView->moveCurrentTabToOtherGroup(); }

// ---------------------------------------------------------------------------
// Languages
// ---------------------------------------------------------------------------
void MainWindow::onLanguageSelected(const QString& lexerName) {
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    // Re-apply theme defaults first (clears stale lexer styling), then the new lexer.
    const Settings& s = Settings::instance();
    ThemeManager::applyToScintilla(t->editor(), ThemeManager::current(), s.fontFamily(), s.fontSize());
    LexerMap::applyLexerByName(t->editor(), lexerName);
}

// ---------------------------------------------------------------------------
// Tools / Help
// ---------------------------------------------------------------------------
void MainWindow::onToolsPreferences() {
    PreferencesDialog dlg(this);
    connect(&dlg, &PreferencesDialog::preferencesChanged, this, [this]{
        m_actToggleLineNumbers->setChecked(Settings::instance().showLineNumbers());
        m_actToggleWordWrap->setChecked(Settings::instance().wordWrap());
        if (Settings::instance().darkTheme()) m_actThemeDark->setChecked(true);
        else                                   m_actThemeLight->setChecked(true);
        const int n = m_multiView->tabCount();
        for (int i = 0; i < n; ++i) applyEditorPreferences(tabAt(i));
        applyThemeToAllTabs();   // already iterates tabs to re-apply theme+lexer
    });
    dlg.exec();
}

void MainWindow::onToolsCompare() {
    if (!m_comparePanel) m_comparePanel = new ComparePanel(this);
    auto* t = currentTab();
    if (t) {
        QByteArray a = t->editor()->getText(t->editor()->textLength() + 1);
        m_comparePanel->setLeft(t->displayPath(), QString::fromUtf8(a));
    }
    // For right-side: ask user to pick a file.
    const QString path = QFileDialog::getOpenFileName(this, tr("Compare with file..."));
    if (!path.isEmpty()) {
        auto r = FileIO::readFile(path);
        if (r.ok) m_comparePanel->setRight(path, QString::fromUtf8(r.utf8));
    }
    m_comparePanel->show();
    m_comparePanel->raise();
    m_comparePanel->activateWindow();
}

void MainWindow::onToolsCssPreview() {
    if (!m_cssPreviewPane) m_cssPreviewPane = new CssPreviewPane(this);
    if (auto* t = currentTab()) m_cssPreviewPane->bindToEditor(t->editor());
    m_cssPreviewPane->show();
    m_cssPreviewPane->raise();
    m_cssPreviewPane->activateWindow();
}

void MainWindow::onToolsCsvView() {
    if (!m_csvTableView) m_csvTableView = new CsvTableView(this);
    if (auto* t = currentTab()) {
        QByteArray a = t->editor()->getText(t->editor()->textLength() + 1);
        m_csvTableView->loadCsv(QString::fromUtf8(a), t->displayPath());
    }
    m_csvTableView->show();
    m_csvTableView->raise();
    m_csvTableView->activateWindow();
}

// ---------------------------------------------------------------------------
// New M3 slots
// ---------------------------------------------------------------------------
void MainWindow::onToolsMarkdownPreview() {
    if (!m_markdownPreviewPane) m_markdownPreviewPane = new MarkdownPreviewPane(this);
    if (auto* t = currentTab()) m_markdownPreviewPane->bindToEditor(t->editor());
    m_markdownPreviewPane->show();
    m_markdownPreviewPane->raise();
    m_markdownPreviewPane->activateWindow();
}

void MainWindow::onToolsHexViewer() {
    if (!m_hexViewer) m_hexViewer = new HexViewer(this);
    if (auto* t = currentTab()) {
        const QByteArray bytes = t->editor()->getText(t->editor()->textLength() + 1);
        m_hexViewer->load(bytes, t->displayPath());
    } else {
        m_hexViewer->load(QByteArray());
    }
    m_hexViewer->show();
    m_hexViewer->raise();
    m_hexViewer->activateWindow();
}

void MainWindow::onToolsFindInFiles() {
    if (!m_findInFilesDialog) {
        m_findInFilesDialog = new FindInFilesDialog(this);
        connect(m_findInFilesDialog, &FindInFilesDialog::openFileRequested,
                this, &MainWindow::onFindInFilesOpen);
    }
    m_findInFilesDialog->show();
    m_findInFilesDialog->raise();
    m_findInFilesDialog->activateWindow();
}

void MainWindow::onToolsCommandPalette() {
    if (!m_commandPalette) m_commandPalette = new CommandPalette(this);
    // Collect leaf actions (skip separators and submenu titles).
    QList<QAction*> actions;
    const auto menus = menuBar()->findChildren<QMenu*>();
    for (auto* m : menus) {
        for (auto* a : m->actions()) {
            if (a->isSeparator()) continue;
            if (a->menu()) continue;
            actions << a;
        }
    }
    m_commandPalette->setActions(actions);
    m_commandPalette->presentCentered();
}

void MainWindow::onToolsRunCommand() {
    if (m_execOutputPanel->isHidden()) {
        m_execOutputPanel->show();
        m_actToggleExecOutput->setChecked(true);
    }
    m_execOutputPanel->raise();
    // Focus the panel's command input — handled internally; we just bring it visible.
    m_execOutputPanel->setFocus();
}

void MainWindow::onViewToggleFunctionList() {
    const bool show = m_actToggleFunctionList->isChecked();
    m_functionListPanel->setVisible(show);
    if (show) {
        if (auto* t = currentTab()) {
            m_functionListPanel->setActiveEditor(t->editor(),
                LexerMap::lexerNameForPath(t->filePath()));
            m_functionListPanel->refresh();
        }
    }
}

void MainWindow::onViewToggleDocumentMap() {
    const bool show = m_actToggleDocumentMap->isChecked();
    m_documentMapPanel->setVisible(show);
    if (show) {
        if (auto* t = currentTab()) m_documentMapPanel->setActiveEditor(t->editor());
    }
}

void MainWindow::onViewToggleFileBrowser() {
    m_fileBrowserPanel->setVisible(m_actToggleFileBrowser->isChecked());
}

void MainWindow::onViewToggleExecOutput() {
    m_execOutputPanel->setVisible(m_actToggleExecOutput->isChecked());
}

void MainWindow::onFileBrowserOpenFile(const QString& path) { openFile(path); }

void MainWindow::onFindInFilesOpen(const QString& path, int line) {
    openFile(path);
    if (auto* t = currentTab()) {
        t->editor()->gotoLine(line - 1);
        t->editor()->scrollCaret();
    }
}

void MainWindow::onFunctionListGoto(int line) {
    if (auto* t = currentTab()) {
        t->editor()->gotoLine(line - 1);
        t->editor()->scrollCaret();
    }
}

void MainWindow::onAutoSaveTick() {
    const bool onlyNamed = m_autoSave && m_autoSave->saveOnlyNamedFiles();
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) {
        auto* t = tabAt(i);
        if (!t || !t->isModified()) continue;
        if (onlyNamed && t->filePath().isEmpty()) continue;
        if (!t->filePath().isEmpty()) saveTab(t);
    }
}

void MainWindow::onHelpAbout() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Sobre LordPad"));
    auto* lay = new QVBoxLayout(&dlg);
    auto* text = new QTextBrowser(&dlg);
    text->setOpenExternalLinks(true);
    text->setMinimumWidth(820);
    text->setMinimumHeight(440);

    const QString html = tr(
        "<h2 style=\"margin-bottom:0\">LordPad <span style=\"color:#9ED12A\">0.9</span></h2>"
        "<p><b>Editor de código nativo Linux Qt6.</b></p>"

        "<table cellpadding=\"6\" cellspacing=\"0\" width=\"100%\" "
                "style=\"border-collapse:collapse\"><tr style=\"vertical-align:top\">"

        "<td width=\"50%\" style=\"padding-right:14px;border-right:1px solid #555\">"
        "<p><b>Editor:</b> Scintilla (Qt) + Lexilla &middot; <b>Toolkit:</b> Qt 6.5+ &middot; "
        "<b>Plugin host:</b> Python embed (opcional).</p>"
        "<h3>Recursos</h3>"
        "<ul style=\"margin-left:0;padding-left:18px\">"
        "<li><b>Editor:</b> multi-cursor, smart highlight, brace matching, sticky scroll, "
        "auto-pair, smart indent, format-on-save, folding, mini-map, multi-view splits.</li>"
        "<li><b>LSP:</b> completar, hover, signature help, símbolos doc/workspace, "
        "inlay hints, diagnostics panel.</li>"
        "<li><b>Git:</b> log, branches, stash, blame, commit, fetch/pull/push, diff gutter, "
        ".gitignore, pre-commit hooks, apply patch.</li>"
        "<li><b>AI:</b> explicar/traduzir seleção, gerar mensagem de commit, ghost completion.</li>"
        "<li><b>Refactor:</b> expandir/encolher seleção, renomear-no-escopo, extrair função, "
        "auto-correct.</li>"
        "<li><b>Modos:</b> Vim, typewriter, dark/light/Dracula + theme packs e editor.</li>"
        "</ul>"
        "<h3>Diálogos</h3>"
        "<p style=\"margin:0\">Compare &middot; CSS preview &middot; CSV table &middot; "
        "Markdown preview &middot; Hex viewer &middot; Find-in-files &middot; "
        "Command palette &middot; Word count &middot; Hash &middot; Macros &middot; "
        "Snippets &middot; Goto line &middot; Find/Replace &middot; Preferências &middot; "
        "Atalhos &middot; Theme editor &middot; Plugin manager.</p>"
        "</td>"

        "<td width=\"50%\" style=\"padding-left:14px\">"
        "<h3 style=\"margin-top:0\">Painéis (60+)</h3>"
        "<p style=\"margin:0 0 6px 0\"><b>Painéis:</b> Function list &middot; Document map &middot; "
        "File browser &middot; Exec output &middot; Terminal &middot; Color palette &middot; "
        "Image viewer/annot &middot; Screenshot &middot; Mermaid &middot; Mind map &middot; "
        "HTML/AsciiDoc preview &middot; Doc preview &middot; Code review/clones/call graph &middot; "
        "Secret scanner &middot; TODOs &middot; Grep &middot; Regex tester &middot; "
        "Merge resolver &middot; Sysinfo &middot; Sysmon &middot; DevTools &middot; OpenAPI.</p>"
        "<p style=\"margin:0 0 6px 0\"><b>Dados:</b> SQLite &middot; DB shell &middot; CLI DB &middot; "
        "JSONL &middot; JSON path &middot; jq &middot; MD Table &middot; CSV chart &middot; "
        "SQL Schema &middot; YAML &middot; Hex viewer &middot; .env &middot; Archive &middot; "
        "Clipboard &middot; GPG &middot; SSL Cert.</p>"
        "<p style=\"margin:0 0 6px 0\"><b>Rede:</b> FTP &middot; SFTP &middot; SSHFS &middot; "
        "SSH Exec &middot; Túneis SSH &middot; Port scan &middot; REST &middot; GraphQL &middot; "
        "RSS &middot; Pastebin &middot; Gist &middot; Git Log &middot; Tradução .po.</p>"
        "<p style=\"margin:0 0 6px 0\"><b>DevOps:</b> Build watch &middot; Cron &middot; "
        "File watcher &middot; Profiler &middot; Test runner &middot; Docker &middot; "
        "kubectl &middot; systemd &middot; Log tail.</p>"
        "<p style=\"margin:0\"><b>Util:</b> Calendário &middot; Calculadora &middot; QR &middot; "
        "TODO &middot; Notas &middot; Notebook &middot; Pomodoro &middot; Time tracker &middot; "
        "Conversor &middot; Cheatsheets &middot; Estatísticas &middot; "
        "<b>Cofre de senhas</b> (AES-256/PBKDF2).</p>"
        "<h3>Integrações por subprocess</h3>"
        "<p style=\"margin:0\">openssl &middot; gpg &middot; ssh &middot; sshfs &middot; "
        "fusermount &middot; docker &middot; kubectl &middot; systemctl &middot; journalctl &middot; "
        "rg &middot; jq &middot; asciidoctor &middot; tail &middot; ss &middot; gh &middot; "
        "python3 &middot; bash.</p>"
        "</td></tr></table>"

        "<p style=\"margin-top:14px\"><b>Hunspell</b> spell-check &middot; "
        "<b>uchardet</b> encoding detect &middot; "
        "<b>Python3-embed</b> plugins &middot; "
        "Build: %1 &middot; Qt %2</p>"
        "<p>Repositório: <a href=\"https://github.com/clip52/notepad-fedora\">"
        "github.com/clip52/notepad-fedora</a></p>")
        .arg(QStringLiteral(__DATE__), QString::fromLatin1(qVersion()));

    text->setHtml(html);
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);

    lay->addWidget(text, 1);
    lay->addWidget(btns);
    dlg.resize(900, 540);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// MultiView event handlers
// ---------------------------------------------------------------------------
void MainWindow::onMultiViewCurrentChanged(EditorTab* tab) {
    auto* sci = tab ? tab->editor() : nullptr;
    if (m_findDialog) m_findDialog->setActiveEditor(sci);
    if (m_languagesMenu) {
        m_languagesMenu->setActiveEditor(sci);
        if (tab) m_languagesMenu->syncCheckedLanguage(LexerMap::lexerNameForPath(tab->filePath()));
    }
    if (m_functionListPanel && m_functionListPanel->isVisible()) {
        m_functionListPanel->setActiveEditor(sci,
            tab ? LexerMap::lexerNameForPath(tab->filePath()) : QString());
    }
    if (m_documentMapPanel && m_documentMapPanel->isVisible()) {
        m_documentMapPanel->setActiveEditor(sci);
    }
    if (m_autoCompleter) m_autoCompleter->setActiveEditor(sci);
    if (m_bookmarks)    m_bookmarks->setActiveEditor(sci);
    if (m_macros)       m_macros->setActiveEditor(sci);
    if (m_eolMenu) {
        m_eolMenu->setActiveEditor(sci);
        if (sci) m_eolMenu->syncCurrentMode();
    }
    if (m_braceMatcher)   m_braceMatcher->setActiveEditor(sci);
    if (m_spellChecker)   m_spellChecker->setActiveEditor(sci);
    if (m_editEnhance)    m_editEnhance->setActiveEditor(sci);
    if (m_whitespaceView && sci) m_whitespaceView->applyTo(sci);
    if (m_gitStatus && tab && !tab->filePath().isEmpty())
        m_gitStatus->queryStatus(tab->filePath());
    else if (m_statusGit) m_statusGit->setText(QString());
    // M7: keep folding / encoding menus and LSP panel in sync with the active tab.
    if (m_foldingMenu)      m_foldingMenu->setActiveEditor(sci);
    if (m_encodingConvert)  m_encodingConvert->setActiveEditor(sci, tab ? tab->filePath() : QString());
    if (m_lspFeatures)      m_lspFeatures->setActiveTab(tab);
    if (m_lspFeatures && tab && !tab->filePath().isEmpty()) {
        m_lspFeatures->requestInlayHintsForVisibleRange();
        // M13: refresh outline + semantic colorization for the new tab.
        m_lspFeatures->requestOutlineSymbolsCurrent();
        m_lspFeatures->requestSemanticTokensCurrent();
    }
    if (m_mergePanel) m_mergePanel->setActiveEditor(sci);
    // M16: keep the AI panel's "selection context" pointing at the current sel.
    if (m_aiPanel && sci) {
        m_aiPanel->setSelectionContext(QString::fromUtf8(sci->getSelText()));
    }
    // M18: route the JSON path panel at the active editor.
    if (m_jsonPathPanel) m_jsonPathPanel->setActiveEditor(sci);
    // M30/33/34: sync code-review / chart / md table panels.
    if (m_review)   m_review->setActiveFile(tab ? tab->filePath() : QString());
    if (m_csvChart) m_csvChart->setActiveEditor(sci);
    if (m_mdTable)  m_mdTable->setActiveEditor(sci);
    if (m_textStats)   m_textStats->setActiveEditor(sci);
    if (m_pastebin)    m_pastebin->setActiveEditor(sci);
    if (m_codeClones)  m_codeClones->setActiveEditor(sci);
    if (m_callGraph)   m_callGraph->setActiveEditor(sci);
    if (m_secrets)     m_secrets->setActiveEditor(sci);
    if (m_hexPanel)    m_hexPanel->setActiveEditor(sci);
    if (m_sqlSchema)   m_sqlSchema->setActiveEditor(sci);
    if (m_yamlVal)     m_yamlVal->setActiveEditor(sci);
    if (m_jq)          m_jq->setActiveEditor(sci);
    if (m_htmlPrev)    m_htmlPrev->setActiveEditor(sci);
    if (m_gist)        m_gist->setActiveEditor(sci);
    if (m_gpg)         m_gpg->setActiveEditor(sci);
    if (m_adoc)        m_adoc->setActiveEditor(sci);
    if (m_stickyScroll)     m_stickyScroll->setActiveEditor(sci);
    if (m_lspDiagnosticsPanel) {
        const QString p = tab ? tab->filePath() : QString();
        m_lspDiagnosticsPanel->setActiveFile(p);
        if (m_lsp && !p.isEmpty()) {
            m_lspDiagnosticsPanel->setDiagnostics(p, m_lsp->diagnosticsFor(p));
        }
    }
    // M9: notify plugins.
    if (m_pluginHost) m_pluginHost->emitOnTabChanged(tab ? tab->filePath() : QString());
    // M10: keep the git log panel pointed at the active file.
    if (m_gitLogPanel && m_gitLogPanel->isVisible())
        m_gitLogPanel->setAnchorFile(tab ? tab->filePath() : QString());
    updateWindowTitle();
    updateStatusBar();
}

void MainWindow::onMultiViewTabCloseRequested(EditorTab* tab) {
    if (!tab) return;
    if (!maybeSaveTab(tab)) return;
    // M7: detach per-file modules before destroying the editor widget.
    if (auto* sci = tab->editor()) {
        if (m_colorMarkers)  m_colorMarkers->detach(sci);
        if (m_gitDiffGutter) m_gitDiffGutter->detach(sci);
    }
    if (m_lsp && !tab->filePath().isEmpty()) m_lsp->didClose(tab->filePath());
    auto loc = m_multiView->locateTab(tab);
    if (loc.first && loc.second >= 0) {
        loc.first->removeTab(loc.second);
    }
    delete tab;
    if (m_multiView->tabCount() == 0) onFileNew();
}

void MainWindow::onCurrentTabModified(bool /*modified*/) {
    auto* t = qobject_cast<EditorTab*>(sender());
    if (!t) return;
    setTabTitle(t, t->tabTitle());
    if (t == currentTab()) updateWindowTitle();
}

void MainWindow::onCurrentTabFilePathChanged(const QString& path) {
    auto* t = qobject_cast<EditorTab*>(sender());
    if (!t) return;
    setTabTitle(t, t->tabTitle());
    setTabTooltip(t, t->displayPath());
    if (t == currentTab()) updateWindowTitle();
    if (m_gitStatus && !path.isEmpty()) m_gitStatus->queryStatus(path);
    // M7: re-attach per-file modules to the new path (Save As / rename).
    if (auto* sci = t->editor(); sci && !path.isEmpty()) {
        if (m_colorMarkers)  m_colorMarkers->attach(sci, path);
        if (m_gitDiffGutter) m_gitDiffGutter->attach(sci, path);
        if (m_encodingConvert && t == currentTab())
            m_encodingConvert->setActiveEditor(sci, path);
    }
    if (t == currentTab()) updateStatusBar();   // M8: refresh on rename
}

void MainWindow::onCursorPositionChanged(int line, int column) {
    auto* t = qobject_cast<EditorTab*>(sender());
    if (t == currentTab()) m_statusPosition->setText(tr("Ln %1, Col %2").arg(line).arg(column));
}

void MainWindow::onRecentFileTriggered() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!a) return;
    openFile(a->data().toString());
}

// ---------------------------------------------------------------------------
// Close
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// M4 slot implementations
// ---------------------------------------------------------------------------
#define WITH_SCI_DO(call) do { if (auto* t = currentTab(); t && t->editor()) { call; } } while (0)
#define SCI_OR_NULL  (currentTab() ? currentTab()->editor() : nullptr)

void MainWindow::onEditTrimWhitespace()  { WITH_SCI_DO(EditOperations::trimTrailingWhitespace(t->editor())); }
void MainWindow::onEditUpperCase()       { WITH_SCI_DO(EditOperations::toUpperSelection(t->editor())); }
void MainWindow::onEditLowerCase()       { WITH_SCI_DO(EditOperations::toLowerSelection(t->editor())); }
void MainWindow::onEditTitleCase()       { WITH_SCI_DO(EditOperations::toTitleSelection(t->editor())); }
void MainWindow::onEditSortAsc()         { WITH_SCI_DO(EditOperations::sortLinesAscending(t->editor())); }
void MainWindow::onEditSortDesc()        { WITH_SCI_DO(EditOperations::sortLinesDescending(t->editor())); }
void MainWindow::onEditSortUnique()      { WITH_SCI_DO(EditOperations::sortLinesUnique(t->editor())); }
void MainWindow::onEditDuplicateLine()   { WITH_SCI_DO(EditOperations::duplicateLine(t->editor())); }
void MainWindow::onEditMoveLineUp()      { WITH_SCI_DO(EditOperations::moveLineUp(t->editor())); }
void MainWindow::onEditMoveLineDown()    { WITH_SCI_DO(EditOperations::moveLineDown(t->editor())); }
void MainWindow::onEditTabsToSpaces()    { WITH_SCI_DO(EditOperations::tabsToSpaces(t->editor(), Settings::instance().tabWidth())); }
void MainWindow::onEditSpacesToTabs()    { WITH_SCI_DO(EditOperations::spacesToTabs(t->editor(), Settings::instance().tabWidth())); }

void MainWindow::onBookmarkToggle()  { if (m_bookmarks) m_bookmarks->toggleAtCaret(); }
void MainWindow::onBookmarkNext()    { if (m_bookmarks) m_bookmarks->gotoNext(); }
void MainWindow::onBookmarkPrev()    { if (m_bookmarks) m_bookmarks->gotoPrevious(); }
void MainWindow::onBookmarkClearAll(){ if (m_bookmarks) m_bookmarks->clearAll(); }

void MainWindow::onBookmarkList() {
    auto* sci = SCI_OR_NULL;
    if (!sci || !m_bookmarks) return;
    BookmarkDialog dlg(m_bookmarks, sci, this);
    connect(&dlg, &BookmarkDialog::gotoLineRequested, this, [this](int line) {
        if (auto* s = SCI_OR_NULL) { s->gotoLine(line - 1); s->scrollCaret(); }
    });
    dlg.exec();
}

void MainWindow::onToolsWordCount() {
    if (!m_wordCountDialog) m_wordCountDialog = new WordCountDialog(this);
    if (auto* t = currentTab()) m_wordCountDialog->load(t->editor(), t->displayPath());
    m_wordCountDialog->show();
    m_wordCountDialog->raise();
    m_wordCountDialog->activateWindow();
}

void MainWindow::onToolsJsonPretty() {
    auto* sci = SCI_OR_NULL; if (!sci) return;
    QString err; auto r = JsonXmlFormatter::jsonPretty(sci, &err);
    if (r == JsonXmlFormatter::Result::ParseError)
        QMessageBox::warning(this, tr("JSON Pretty"), tr("Parse error: %1").arg(err));
}
void MainWindow::onToolsJsonMinify() {
    auto* sci = SCI_OR_NULL; if (!sci) return;
    QString err; auto r = JsonXmlFormatter::jsonMinify(sci, &err);
    if (r == JsonXmlFormatter::Result::ParseError)
        QMessageBox::warning(this, tr("JSON Minify"), tr("Parse error: %1").arg(err));
}
void MainWindow::onToolsXmlPretty() {
    auto* sci = SCI_OR_NULL; if (!sci) return;
    QString err; auto r = JsonXmlFormatter::xmlPretty(sci, &err);
    if (r == JsonXmlFormatter::Result::ParseError)
        QMessageBox::warning(this, tr("XML Pretty"), tr("Parse error: %1").arg(err));
}
void MainWindow::onToolsXmlMinify() {
    auto* sci = SCI_OR_NULL; if (!sci) return;
    QString err; auto r = JsonXmlFormatter::xmlMinify(sci, &err);
    if (r == JsonXmlFormatter::Result::ParseError)
        QMessageBox::warning(this, tr("XML Minify"), tr("Parse error: %1").arg(err));
}

void MainWindow::onToolsPickColor() {
    if (auto* sci = SCI_OR_NULL) ColorPickerHelper::pickAndReplace(sci, this);
}

void MainWindow::onToolsMacroDialog() {
    auto* sci = SCI_OR_NULL;
    if (!m_macroDialog) m_macroDialog = new MacroDialog(m_macros, sci, this);
    m_macroDialog->show();
    m_macroDialog->raise();
    m_macroDialog->activateWindow();
}

// ---------------------------------------------------------------------------
// M5 slot implementations
// ---------------------------------------------------------------------------
void MainWindow::onFilePrint() {
    if (auto* sci = SCI_OR_NULL) PrintHelper::printDocument(sci, this);
}
void MainWindow::onFilePrintPreview() {
    if (auto* sci = SCI_OR_NULL) PrintHelper::previewDocument(sci, this);
}
void MainWindow::onFileReloadFromDisk() {
    auto* t = currentTab();
    if (!t || t->filePath().isEmpty()) return;
    QString err;
    if (!EditEnhancements::reloadFromDisk(t->editor(), t->filePath(), &err)) {
        QMessageBox::warning(this, tr("Reload failed"), err);
        return;
    }
    t->setModified(false);
    applyEditorPreferences(t);
    applyThemeAndLexer(t);
}
void MainWindow::onFileOpenFolder() {
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Open Folder"));
    if (folder.isEmpty()) return;
    if (m_recentProjects) m_recentProjects->use(folder);
    if (m_fileBrowserPanel) {
        m_fileBrowserPanel->setRootPath(folder);
        m_fileBrowserPanel->show();
        if (m_actToggleFileBrowser) m_actToggleFileBrowser->setChecked(true);
    }
}

void MainWindow::onSearchGotoMatchingBrace() {
    if (m_braceMatcher) m_braceMatcher->gotoMatchingBrace();
}

void MainWindow::onViewToggleWhitespace() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!m_whitespaceView || !a) return;
    m_whitespaceView->setWhitespaceVisible(a->isChecked());
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) if (auto* t = tabAt(i)) m_whitespaceView->applyTo(t->editor());
}
void MainWindow::onViewToggleEol() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!m_whitespaceView || !a) return;
    m_whitespaceView->setEolVisible(a->isChecked());
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) if (auto* t = tabAt(i)) m_whitespaceView->applyTo(t->editor());
}
void MainWindow::onViewToggleIndentGuides() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!m_whitespaceView || !a) return;
    m_whitespaceView->setIndentGuidesVisible(a->isChecked());
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) if (auto* t = tabAt(i)) m_whitespaceView->applyTo(t->editor());
}

void MainWindow::onToolsHash() {
    if (!m_hashDialog) m_hashDialog = new HashDialog(this);
    if (auto* t = currentTab()) {
        const QByteArray sel = t->editor()->getSelText();
        if (!sel.isEmpty()) m_hashDialog->load(sel, tr("selection (%1 bytes)").arg(sel.size()));
        else                m_hashDialog->load(t->editor()->getText(t->editor()->textLength() + 1), t->displayPath());
    }
    m_hashDialog->show();
    m_hashDialog->raise();
    m_hashDialog->activateWindow();
}

void MainWindow::onToolsSnippets() {
    if (!m_snippetsDialog) m_snippetsDialog = new SnippetsDialog(m_snippets, this);
    m_snippetsDialog->show();
    m_snippetsDialog->raise();
    m_snippetsDialog->activateWindow();
}

void MainWindow::onToolsToggleSpellCheck() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!m_spellChecker || !a) return;
    m_spellChecker->setEnabled(a->isChecked());
    if (auto* sci = SCI_OR_NULL) m_spellChecker->setActiveEditor(sci);
}

void MainWindow::onExternalFileChanged(const QString& path) {
    int idx = findTabByPath(path);
    if (idx < 0) return;
    auto* t = tabAt(idx);
    if (!t) return;
    const auto ans = QMessageBox::question(this, tr("File changed externally"),
        tr("'%1' was modified outside the editor.\nReload from disk?").arg(path),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (ans == QMessageBox::Yes) {
        QString err;
        if (EditEnhancements::reloadFromDisk(t->editor(), path, &err)) {
            t->setModified(false);
            applyEditorPreferences(t);
            applyThemeAndLexer(t);
        } else {
            QMessageBox::warning(this, tr("Reload failed"), err);
        }
    }
}
void MainWindow::onExternalFileRemoved(const QString& path) {
    QMessageBox::warning(this, tr("File removed"),
        tr("'%1' no longer exists on disk.").arg(path));
}

#undef WITH_SCI_DO
#undef SCI_OR_NULL

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // M10: snippet tabstop navigation. We watch the active editor; when the
    // user presses Tab / Shift+Tab during an active session, we hijack the
    // key. Tab without an active session attempts a fresh snippet expansion
    // — if a snippet matches the word at the caret, the session begins.
    if (event->type() == QEvent::KeyPress) {
        auto* sci = qobject_cast<ScintillaEdit*>(watched);
        if (sci && m_snippets) {
            auto* ke = static_cast<QKeyEvent*>(event);
            const bool noMods    = ke->modifiers() == Qt::NoModifier;
            const bool shiftOnly = ke->modifiers() == Qt::ShiftModifier;

            if (ke->key() == Qt::Key_Escape) {
                if (m_snippets->hasActiveSession(sci)) {
                    m_snippets->cancelSession(sci);
                    return true;
                }
                // M21: dismiss AI ghost on Esc.
                if (m_aiGhost && m_aiGhost->dismissCurrent(sci)) return true;
            }
            if (ke->key() == Qt::Key_Tab && noMods) {
                if (m_snippets->hasActiveSession(sci)) {
                    m_snippets->advanceTabstop(sci);
                    return true;
                }
                // M21: accept AI ghost suggestion if visible.
                if (m_aiGhost && m_aiGhost->acceptCurrent(sci)) return true;
                // No session — try to start one by expanding the trigger word.
                auto* t = currentTab();
                if (t) {
                    const QString lex = LexerMap::lexerNameForPath(t->filePath());
                    if (m_snippets->tryExpand(sci, lex)) return true;
                }
            }
            if (ke->key() == Qt::Key_Backtab
                || (ke->key() == Qt::Key_Tab && shiftOnly)) {
                if (m_snippets->hasActiveSession(sci)) {
                    m_snippets->retreatTabstop(sci);
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // M12: only the Primary window writes the global session — Secondary
    // windows skip it so we don't overwrite Primary's tab list with theirs.
    if (m_mode == Mode::Primary && m_session) {
        QStringList paths;
        const int n = m_multiView->tabCount();
        for (int i = 0; i < n; ++i) {
            if (auto* t = tabAt(i)) {
                if (!t->filePath().isEmpty()) paths << t->filePath();
            }
        }
        int active = 0;
        if (auto* cur = currentTab()) {
            const int idx = paths.indexOf(cur->filePath());
            if (idx >= 0) active = idx;
        }
        m_session->saveSession(paths, active);
    }

    for (int i = m_multiView->tabCount() - 1; i >= 0; --i) {
        if (!maybeSaveTab(tabAt(i))) { event->ignore(); return; }
    }

    // Window state is per-window in Qt's saveGeometry/saveState — but our
    // QSettings keys are global, so only Primary persists. Secondary windows
    // open at their previous geometry on a per-process basis (default Qt).
    if (m_mode == Mode::Primary) {
        Settings::instance().setWindowGeometry(saveGeometry());
        Settings::instance().setWindowState(saveState());
        Settings::instance().save();
    }
    if (m_lsp) m_lsp->shutdownAll();
    if (m_pluginHost) m_pluginHost->shutdown();   // only Primary owns the host
    if (m_mode == Mode::Primary && m_crashRecovery) m_crashRecovery->shutdownClean();
    event->accept();
}

// ---------------------------------------------------------------------------
// M6 slots
// ---------------------------------------------------------------------------
void MainWindow::onToolsCodeFormat() {
    if (!m_codeFormatter) return;
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    m_codeFormatter->formatActiveEditor(t->editor());
}

void MainWindow::onThemePackSelected() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!a) return;
    const auto id = static_cast<ThemePackId>(a->data().toInt());
    ThemePack::applyToApp(qApp, id);
    const Settings& s = Settings::instance();
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) {
        if (auto* tab = tabAt(i); tab && tab->editor()) {
            ThemePack::applyToEditor(tab->editor(), id, s.fontFamily(), s.fontSize());
        }
    }
    ThemePack::save(id);
}

void MainWindow::onGitStatusReady(const QString& path, const GitStatus& status) {
    if (!m_statusGit) return;
    auto* t = currentTab();
    if (!t || t->filePath() != path) return;
    if (status.state == GitStatus::State::NotInRepo) {
        m_statusGit->setText(QString());
        return;
    }
    QString glyph = GitStatusService::stateGlyph(status.state);
    QString text = QString("git: %1 %2").arg(glyph, status.branch);
    if (status.ahead)  text += QString(" ↑%1").arg(status.ahead);
    if (status.behind) text += QString(" ↓%1").arg(status.behind);
    m_statusGit->setText(text);
}

void MainWindow::onWorkspaceOpen() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir Workspace"), QString(),
                                                      tr("Workspaces (*.nppproj.json)"));
    if (path.isEmpty()) return;
    if (!m_workspace->load(path)) {
        QMessageBox::warning(this, tr("Workspace"), m_workspace->lastError());
        return;
    }
    applyWorkspaceData();
    rebuildRecentWorkspacesMenu();
}

void MainWindow::onWorkspaceSave() {
    if (!m_workspace) return;
    QString path = m_workspace->currentPath();
    if (path.isEmpty()) { onWorkspaceSaveAs(); return; }
    auto& d = m_workspace->mutableData();
    d.openFiles.clear();
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) {
        if (auto* t = tabAt(i); t && !t->filePath().isEmpty()) {
            WorkspaceFile wf;
            wf.path = t->filePath();
            wf.active = (t == currentTab());
            d.openFiles.append(wf);
        }
    }
    if (!m_workspace->save(path)) {
        QMessageBox::warning(this, tr("Workspace"), m_workspace->lastError());
    }
    rebuildRecentWorkspacesMenu();
}

void MainWindow::onWorkspaceSaveAs() {
    QString path = QFileDialog::getSaveFileName(this, tr("Salvar Workspace"),
                                                QStringLiteral("workspace.nppproj.json"),
                                                tr("Workspaces (*.nppproj.json)"));
    if (path.isEmpty()) return;
    auto& d = m_workspace->mutableData();
    d.openFiles.clear();
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) {
        if (auto* t = tabAt(i); t && !t->filePath().isEmpty()) {
            WorkspaceFile wf;
            wf.path = t->filePath();
            wf.active = (t == currentTab());
            d.openFiles.append(wf);
        }
    }
    if (!m_workspace->save(path)) {
        QMessageBox::warning(this, tr("Workspace"), m_workspace->lastError());
    }
    rebuildRecentWorkspacesMenu();
}

void MainWindow::onRecentWorkspaceTriggered() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!a) return;
    const QString path = a->data().toString();
    if (path.isEmpty()) return;
    if (!m_workspace->load(path)) {
        QMessageBox::warning(this, tr("Workspace"), m_workspace->lastError());
        return;
    }
    applyWorkspaceData();
    rebuildRecentWorkspacesMenu();
}

void MainWindow::rebuildRecentWorkspacesMenu() {
    if (!m_menuRecentWorkspaces) return;
    m_menuRecentWorkspaces->clear();
    const QStringList recent = Workspace::recentWorkspaces();
    if (recent.isEmpty()) {
        auto* a = m_menuRecentWorkspaces->addAction(tr("(vazio)"));
        a->setEnabled(false);
        return;
    }
    for (const QString& p : recent) {
        auto* a = m_menuRecentWorkspaces->addAction(QFileInfo(p).fileName() + "  " + p);
        a->setData(p);
        connect(a, &QAction::triggered, this, &MainWindow::onRecentWorkspaceTriggered);
    }
    m_menuRecentWorkspaces->addSeparator();
    auto* clr = m_menuRecentWorkspaces->addAction(tr("Limpar lista"));
    connect(clr, &QAction::triggered, this, [this]{
        Workspace::clearRecent();
        rebuildRecentWorkspacesMenu();
    });
}

void MainWindow::applyWorkspaceData() {
    if (!m_workspace) return;
    const auto& d = m_workspace->data();
    EditorTab* lastActive = nullptr;
    for (const WorkspaceFile& wf : d.openFiles) {
        if (wf.path.isEmpty()) continue;
        openFile(wf.path);
        if (wf.active) {
            const int idx = findTabByPath(wf.path);
            if (idx >= 0) lastActive = tabAt(idx);
        }
    }
    if (lastActive) setActiveTab(lastActive);
    // M13: tell the test runner about the workspace folder so auto-detection
    // can pick the right framework without the user re-typing the path.
    if (m_testRunner)
        m_testRunner->setWorkspaceFolder(QFileInfo(m_workspace->currentPath()).absolutePath());
}

// ---------------------------------------------------------------------------
// M7 slots
// ---------------------------------------------------------------------------
void MainWindow::onMultiCursorAddBelow() {
    if (auto* t = currentTab(); t && t->editor()) MultiCursor::addCursorBelow(t->editor());
}
void MainWindow::onMultiCursorAddAbove() {
    if (auto* t = currentTab(); t && t->editor()) MultiCursor::addCursorAbove(t->editor());
}
void MainWindow::onMultiCursorSelectAllOccurrences() {
    if (auto* t = currentTab(); t && t->editor()) MultiCursor::selectAllOccurrences(t->editor());
}
void MainWindow::onMultiCursorAddNextOccurrence() {
    if (auto* t = currentTab(); t && t->editor()) MultiCursor::addNextOccurrence(t->editor());
}

void MainWindow::onSmartHighlightToggle() {
    if (!m_actSmartHighlight) return;
    SmartHighlight::shared().setEnabled(m_actSmartHighlight->isChecked());
}

void MainWindow::onColorMarkerToggle() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!a || !m_colorMarkers) return;
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    m_colorMarkers->toggle(t->editor(), static_cast<MarkColor>(a->data().toInt()));
}
void MainWindow::onColorMarkerNext() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!a || !m_colorMarkers) return;
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    m_colorMarkers->next(t->editor(), static_cast<MarkColor>(a->data().toInt()));
}
void MainWindow::onColorMarkerClearAll() {
    auto* a = qobject_cast<QAction*>(sender());
    if (!a || !m_colorMarkers) return;
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    m_colorMarkers->clearAll(t->editor(), static_cast<MarkColor>(a->data().toInt()));
}

void MainWindow::onGitDiffGutterToggle() {
    if (!m_gitDiffGutter || !m_actGitDiffGutter) return;
    m_gitDiffGutter->setEnabled(m_actGitDiffGutter->isChecked());
    if (m_actGitDiffGutter->isChecked()) {
        // Re-attach all open named tabs so markers are recomputed immediately.
        const int n = m_multiView->tabCount();
        for (int i = 0; i < n; ++i) {
            auto* t = tabAt(i);
            if (t && t->editor() && !t->filePath().isEmpty())
                m_gitDiffGutter->attach(t->editor(), t->filePath());
        }
    }
}

void MainWindow::onLspDiagnosticsUpdated(const QString& filePath,
                                         const QList<LspDiagnostic>& diags) {
    if (!m_lspDiagnosticsPanel) return;
    m_lspDiagnosticsPanel->setDiagnostics(filePath, diags);
}

void MainWindow::onLspDiagnosticActivated(const QString& filePath, int line, int column) {
    int idx = findTabByPath(filePath);
    if (idx < 0) { openFile(filePath); idx = findTabByPath(filePath); }
    if (idx < 0) return;
    setActiveTab(tabAt(idx));
    if (auto* t = currentTab(); t && t->editor()) {
        // LSP positions are 0-based; gotoLine uses 0-based directly.
        t->editor()->gotoLine(line);
        const int pos = static_cast<int>(t->editor()->positionFromLine(line)) + column;
        t->editor()->gotoPos(pos);
        t->editor()->scrollCaret();
    }
}

void MainWindow::onLspServerError(const QString& serverName, const QString& message) {
    if (m_lspDiagnosticsPanel)
        m_lspDiagnosticsPanel->showStatusMessage(tr("%1: %2").arg(serverName, message));
}

void MainWindow::onCurrentTabTextModified() {
    // Reserved for future hooks; LSP didChange is wired directly in connectTabSignals.
}

// ---------------------------------------------------------------------------
// M9 plugin manager
// ---------------------------------------------------------------------------
void MainWindow::onToolsPluginManager() {
    PluginManagerDialog dlg(m_pluginHost, this);
    dlg.exec();
}

void MainWindow::onViewToggleTerminal() {
    if (!m_terminalPanel || !m_actToggleTerminal) return;
    m_terminalPanel->setVisible(m_actToggleTerminal->isChecked());
}

void MainWindow::onViewToggleFtp() {
    if (!m_ftpPanel || !m_actToggleFtp) return;
    m_ftpPanel->setVisible(m_actToggleFtp->isChecked());
}

void MainWindow::onViewToggleSftp() {
    if (!m_sftpPanel || !m_actToggleSftp) return;
    m_sftpPanel->setVisible(m_actToggleSftp->isChecked());
}

void MainWindow::onToolsKeybindings() {
    if (!m_keybindings) return;
    KeybindingsDialog dlg(m_keybindings, this);
    dlg.exec();
}

void MainWindow::onFileNewWindow() {
    auto* w = new MainWindow(nullptr, Mode::Secondary);
    w->show();
    // WA_DeleteOnClose handles cleanup; QApplication::quitOnLastWindowClosed
    // (Qt default) tears the process down when the last MainWindow exits.
}

void MainWindow::onToolsGitBranches() {
    auto* t = currentTab();
    const QString anchor = t ? t->filePath() : QString();
    if (anchor.isEmpty()) {
        QMessageBox::information(this, tr("Branches"),
            tr("Abra um arquivo do repositório primeiro."));
        return;
    }
    GitBranchDialog dlg(anchor, this);
    dlg.exec();
}

void MainWindow::onToolsGitStash() {
    auto* t = currentTab();
    const QString anchor = t ? t->filePath() : QString();
    if (anchor.isEmpty()) {
        QMessageBox::information(this, tr("Stash"),
            tr("Abra um arquivo do repositório primeiro."));
        return;
    }
    GitStashDialog dlg(anchor, this);
    dlg.exec();
}

void MainWindow::onToolsTasksRun() {
    if (!m_tasksRunner) return;
    QString workspace;
    if (m_workspace) workspace = QFileInfo(m_workspace->currentPath()).absolutePath();
    if (workspace.isEmpty()) {
        // Fall back to the active file's directory — useful for one-off scripts.
        if (auto* t = currentTab()) workspace = QFileInfo(t->filePath()).absolutePath();
    }
    if (workspace.isEmpty()) {
        QMessageBox::information(this, tr("Tarefas"),
            tr("Sem workspace ativo. Abra um arquivo ou um workspace primeiro."));
        return;
    }
    QString err;
    QList<TasksRunner::Task> tasks = m_tasksRunner->load(workspace, &err);
    if (tasks.isEmpty()) {
        QMessageBox::information(this, tr("Tarefas"), err.isEmpty() ?
            tr("Nenhuma tarefa definida em .notepadpp/tasks.json.") : err);
        return;
    }

    QList<QuickPickDialog::Item> items;
    for (int i = 0; i < tasks.size(); ++i) {
        QuickPickDialog::Item it;
        it.label = tasks[i].label;
        it.subtitle = QStringLiteral("%1 %2").arg(tasks[i].command, tasks[i].args.join(' '));
        it.data = i;
        items.append(it);
    }
    QuickPickDialog dlg(this, tr("Rodar tarefa"), items);
    if (dlg.exec() != QDialog::Accepted) return;
    const int picked = dlg.pickedData().toInt();
    if (picked < 0 || picked >= tasks.size()) return;

    auto* t = currentTab();
    const QString activeFile = t ? t->filePath() : QString();
    const auto resolved = m_tasksRunner->resolve(tasks[picked], workspace, activeFile);

    // Surface the run in the existing exec output panel — same UX as
    // Tools > Run Command. Build a `cd && command args…` line so the panel's
    // single-shell-spawn pattern picks up the cwd properly.
    if (!m_execOutputPanel) return;
    m_execOutputPanel->setWorkingDirectory(resolved.cwd);
    m_execOutputPanel->show();
    if (m_actToggleExecOutput) m_actToggleExecOutput->setChecked(true);
    QStringList parts; parts << resolved.command;
    for (const QString& a : resolved.args) {
        // Naive quoting: wrap args containing spaces in single quotes.
        if (a.contains(' ')) parts << QStringLiteral("'%1'").arg(a);
        else                 parts << a;
    }
    m_execOutputPanel->runCommand(parts.join(' '));
}

void MainWindow::onViewToggleCalendar()   { if (m_calendarPanel)   m_calendarPanel->setVisible(!m_calendarPanel->isVisible()); }
void MainWindow::onViewToggleCalculator() { if (m_calculatorPanel) m_calculatorPanel->setVisible(!m_calculatorPanel->isVisible()); }
void MainWindow::onViewToggleTodo()       { if (m_todoPanel)       m_todoPanel->setVisible(!m_todoPanel->isVisible()); }
void MainWindow::onViewToggleQr()         { if (m_qrPanel)         m_qrPanel->setVisible(!m_qrPanel->isVisible()); }
void MainWindow::onViewToggleMermaid()    { if (m_mermaidPanel)    m_mermaidPanel->setVisible(!m_mermaidPanel->isVisible()); }
void MainWindow::onViewToggleMergeResolver() { if (m_mergePanel) m_mergePanel->setVisible(!m_mergePanel->isVisible()); }
void MainWindow::onViewToggleTestRunner()    {
    if (!m_testRunner) return;
    m_testRunner->setVisible(!m_testRunner->isVisible());
    // First time the user opens it, seed the workspace folder from active file.
    if (m_testRunner->isVisible()) {
        QString ws;
        if (m_workspace && !m_workspace->currentPath().isEmpty())
            ws = QFileInfo(m_workspace->currentPath()).absolutePath();
        else if (auto* t = currentTab())
            ws = QFileInfo(t->filePath()).absolutePath();
        if (!ws.isEmpty()) m_testRunner->setWorkspaceFolder(ws);
    }
}

void MainWindow::onToolsToggleFormatOnSave()
{
    if (!m_formatOnSave) return;
    auto* a = qobject_cast<QAction*>(sender());
    const bool on = a ? a->isChecked() : !m_formatOnSave->isEnabled();
    m_formatOnSave->setEnabled(on);
    statusBar()->showMessage(on ? tr("Format-on-save: ligado") : tr("Format-on-save: desligado"), 3000);
}

void MainWindow::onLspSemanticTokensReady(const QString& filePath,
                                          const QList<LspSemanticToken>& tokens)
{
    int idx = findTabByPath(filePath);
    if (idx < 0) return;
    auto* t = tabAt(idx);
    if (!t || !t->editor()) return;
    auto* sci = t->editor();

    // Indicators 16..22 reserved for semantic-token colorization. Each token
    // type maps to an indicator + a foreground color overriding the lexer's.
    // Using INDIC_TEXTFORE keeps lexer-applied bold/italic so we layer
    // semantic info on top instead of replacing the syntax styling.
    struct Mapping { const char* type; int indic; sptr_t color; };
    static const Mapping kMap[] = {
        { "function",      16, 0x0040D7E5 },   // soft yellow
        { "method",        16, 0x0040D7E5 },
        { "type",          17, 0x00C8A464 },   // teal
        { "class",         17, 0x00C8A464 },
        { "interface",     17, 0x00C8A464 },
        { "struct",        17, 0x00C8A464 },
        { "enum",          17, 0x00C8A464 },
        { "enumMember",    17, 0x00C8A464 },
        { "typeParameter", 17, 0x00C8A464 },
        { "namespace",     17, 0x00C8A464 },
        { "parameter",     18, 0x00C89464 },   // blue-ish
        { "variable",      19, 0x00B5BDC2 },   // soft white
        { "property",      19, 0x00B5BDC2 },
        { "keyword",       20, 0x00B886F0 },   // magenta
        { "modifier",      20, 0x00B886F0 },
        { "macro",         20, 0x00B886F0 },
        { "comment",       21, 0x00808080 },   // gray
        { "string",        22, 0x004CB1FF },   // orange
        { "number",        22, 0x004CB1FF },
        { "regexp",        22, 0x004CB1FF },
    };

    auto indicForType = [&](const QString& kind) -> int {
        for (const auto& m : kMap) if (kind == QLatin1String(m.type)) return m.indic;
        return -1;
    };
    auto colorForType = [&](const QString& kind) -> sptr_t {
        for (const auto& m : kMap) if (kind == QLatin1String(m.type)) return m.color;
        return 0;
    };

    // Configure each indicator once (style + color). Idempotent.
    for (int i = 16; i <= 22; ++i) {
        sci->indicSetStyle(i, INDIC_TEXTFORE);
    }
    for (const auto& m : kMap) sci->indicSetFore(m.indic, m.color);

    // Clear previous semantic indicators across the buffer.
    const sptr_t docLen = sci->length();
    for (int i = 16; i <= 22; ++i) {
        sci->setIndicatorCurrent(i);
        sci->indicatorClearRange(0, docLen);
    }

    // Apply each token. LSP positions are line + UTF-16 character — the
    // ASCII fast path matches Scintilla's byte offsets directly; for non-ASCII
    // we accept slight column drift (consistent with the rest of M8/M11).
    for (const LspSemanticToken& tok : tokens) {
        const int indic = indicForType(tok.tokenType);
        if (indic < 0) continue;
        Q_UNUSED(colorForType);
        const int lineStart = static_cast<int>(sci->positionFromLine(tok.line));
        QByteArray lineBytes = sci->getLine(tok.line);
        const int byteCol  = qMin(tok.column,  static_cast<int>(lineBytes.size()));
        const int byteLen  = qMin(tok.length,
                                  static_cast<int>(lineBytes.size()) - byteCol);
        if (byteLen <= 0) continue;
        sci->setIndicatorCurrent(indic);
        sci->indicatorFillRange(lineStart + byteCol, byteLen);
    }
}

void MainWindow::onLspOutlineSymbolsReady(const QString& filePath,
                                          const QList<LspSymbol>& syms)
{
    auto* t = currentTab();
    if (!t || t->filePath() != filePath) return;
    if (m_functionListPanel) m_functionListPanel->setLspSymbols(syms);
}

// ---------------------------------------------------------------------------
// M14 slots
// ---------------------------------------------------------------------------
void MainWindow::onViewToggleNotebook()
{
    if (!m_notebookPanel) return;
    m_notebookPanel->setVisible(!m_notebookPanel->isVisible());
    if (m_notebookPanel->isVisible()) {
        // If the active tab is an .ipynb, hand it to the panel.
        if (auto* t = currentTab(); t && t->filePath().endsWith(QStringLiteral(".ipynb"))) {
            m_notebookPanel->openFile(t->filePath());
        }
    }
}

void MainWindow::onViewToggleNotes()
{
    if (!m_notesPanel) return;
    m_notesPanel->setVisible(!m_notesPanel->isVisible());
}

void MainWindow::onViewToggleTypewriter()
{
    if (!m_typewriter) return;
    m_typewriter->setEnabled(!m_typewriter->isEnabled());
    statusBar()->showMessage(m_typewriter->isEnabled()
        ? tr("Typewriter mode: ligado") : tr("Typewriter mode: desligado"), 3000);
}

void MainWindow::onToolsToggleVimMode()
{
    if (!m_vim) return;
    m_vim->setEnabled(!m_vim->isEnabled());
    if (m_statusVim)
        m_statusVim->setText(m_vim->isEnabled() ? m_vim->modeLabel() : QString());
    statusBar()->showMessage(m_vim->isEnabled()
        ? tr("Vim mode: ligado (Esc volta pra Normal)")
        : tr("Vim mode: desligado"), 4000);
}

void MainWindow::onToolsThemeEditor()
{
    auto* t = currentTab();
    if (!t || !t->editor()) {
        QMessageBox::information(this, tr("Tema"),
            tr("Abra um arquivo primeiro."));
        return;
    }
    const QString lex = LexerMap::lexerNameForPath(t->filePath());
    ThemeEditorDialog dlg(t->editor(), lex, this);
    dlg.exec();
}

void MainWindow::onToolsQuickSwitch()
{
    QList<QuickPickDialog::Item> items;
    QSet<QString> seen;
    // First the open tabs — they're the most likely target.
    const int n = m_multiView ? m_multiView->tabCount() : 0;
    for (int i = 0; i < n; ++i) {
        auto* t = tabAt(i);
        if (!t) continue;
        QuickPickDialog::Item it;
        it.label    = QFileInfo(t->filePath()).fileName();
        if (it.label.isEmpty()) it.label = tr("(sem título %1)").arg(i + 1);
        it.subtitle = t->filePath();
        it.data     = t->filePath().isEmpty() ? QVariant(i)
                                              : QVariant(t->filePath());
        items.append(it);
        if (!t->filePath().isEmpty()) seen.insert(t->filePath());
    }
    // Then recent files not already open.
    const QStringList recent = Settings::instance().recentFiles();
    for (const QString& p : recent) {
        if (seen.contains(p)) continue;
        QuickPickDialog::Item it;
        it.label    = QFileInfo(p).fileName();
        it.subtitle = p;
        it.data     = QVariant(p);
        items.append(it);
    }
    QuickPickDialog dlg(this, tr("Quick Switch"), items);
    if (dlg.exec() != QDialog::Accepted) return;
    const QVariant v = dlg.pickedData();
    if (v.typeId() == QMetaType::QString) {
        const QString p = v.toString();
        const int idx = findTabByPath(p);
        if (idx >= 0) setActiveTab(tabAt(idx));
        else          openFile(p);
    } else if (v.typeId() == QMetaType::Int) {
        // Index into tabs (used for unsaved buffers).
        if (auto* t = tabAt(v.toInt())) setActiveTab(t);
    }
}

void MainWindow::onToolsExtractFunction()
{
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    const QByteArray sel = t->editor()->getSelText();
    if (sel.isEmpty()) {
        QMessageBox::information(this, tr("Refatorar"),
            tr("Selecione o bloco a extrair primeiro."));
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Extrair função"),
        tr("Nome da função:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    QString err;
    const QString lex = LexerMap::lexerNameForPath(t->filePath());
    if (!Refactor::extractFunction(t->editor(), lex, name.trimmed(), &err)) {
        QMessageBox::warning(this, tr("Refatorar"), err);
    }
}

// ---------------------------------------------------------------------------
// M15/M16 slots
// ---------------------------------------------------------------------------
void MainWindow::onViewToggleRest()  { if (m_restPanel)   m_restPanel->setVisible(!m_restPanel->isVisible()); }
void MainWindow::onViewToggleSqlite(){ if (m_sqlitePanel) m_sqlitePanel->setVisible(!m_sqlitePanel->isVisible()); }
void MainWindow::onViewToggleAi()    {
    if (!m_aiPanel) return;
    m_aiPanel->setVisible(!m_aiPanel->isVisible());
    // Refresh selection context when reopening.
    if (m_aiPanel->isVisible()) {
        if (auto* t = currentTab(); t && t->editor())
            m_aiPanel->setSelectionContext(QString::fromUtf8(t->editor()->getSelText()));
    }
}
void MainWindow::onViewTogglePoEditor() {
    if (!m_poPanel) return;
    m_poPanel->setVisible(!m_poPanel->isVisible());
    // If the active buffer is a .po, hand it to the panel for convenience.
    if (m_poPanel->isVisible()) {
        if (auto* t = currentTab(); t && t->filePath().endsWith(QStringLiteral(".po")))
            m_poPanel->openFile(t->filePath());
    }
}

void MainWindow::onToolsLoadCoverage()
{
    if (!m_coverage) return;
    const QString path = QFileDialog::getOpenFileName(this, tr("Coverage data"),
        QString(), tr("LCOV / gcov (*.info *.gcov);;Todos (*)"));
    if (path.isEmpty()) return;
    QString err;
    bool ok = path.endsWith(QStringLiteral(".gcov"))
                  ? m_coverage->loadGcovFile(path, &err)
                  : m_coverage->loadLcovTracefile(path, &err);
    if (!ok) QMessageBox::warning(this, tr("Coverage"), err);
    else     statusBar()->showMessage(tr("Coverage carregada: %1").arg(path), 4000);
}

// ---------------------------------------------------------------------------
// M17–M20 slots
// ---------------------------------------------------------------------------
void MainWindow::onToolsToggleAutoPair() {
    if (!m_autoPair) return;
    m_autoPair->setEnabled(!m_autoPair->isEnabled());
    statusBar()->showMessage(m_autoPair->isEnabled()
        ? tr("Auto-pair: ligado") : tr("Auto-pair: desligado"), 3000);
}
void MainWindow::onToolsToggleSmartIndent() {
    if (!m_smartIndent) return;
    m_smartIndent->setEnabled(!m_smartIndent->isEnabled());
    statusBar()->showMessage(m_smartIndent->isEnabled()
        ? tr("Smart indent: ligado") : tr("Smart indent: desligado"), 3000);
}
void MainWindow::onToolsSurroundSelection() {
    auto* t = currentTab();
    if (!t || !t->editor() || !m_autoPair) return;
    bool ok = false;
    const QString chars = QInputDialog::getText(this, tr("Surround"),
        tr("Caracteres de abertura+fechamento (ex.: () [] {} \"\" ** _ _):"),
        QLineEdit::Normal, QStringLiteral("()"), &ok);
    if (!ok || chars.isEmpty()) return;
    QChar open = chars[0];
    QChar close = (chars.size() > 1) ? chars[1] : open;
    m_autoPair->surround(t->editor(), open, close);
}

void MainWindow::onViewToggleImageViewer()    { if (m_imagePanel)     m_imagePanel->setVisible(!m_imagePanel->isVisible()); }
void MainWindow::onViewToggleRegexTester()    { if (m_regexPanel)     m_regexPanel->setVisible(!m_regexPanel->isVisible()); }
void MainWindow::onViewToggleUnitConverter()  { if (m_unitPanel)      m_unitPanel->setVisible(!m_unitPanel->isVisible()); }
void MainWindow::onViewToggleJsonPath()       {
    if (!m_jsonPathPanel) return;
    m_jsonPathPanel->setVisible(!m_jsonPathPanel->isVisible());
    if (m_jsonPathPanel->isVisible()) {
        if (auto* t = currentTab()) m_jsonPathPanel->setActiveEditor(t->editor());
    }
}
void MainWindow::onViewToggleColorPalette()   { if (m_colorPalette)   m_colorPalette->setVisible(!m_colorPalette->isVisible()); }
void MainWindow::onViewToggleSysInfo()        { if (m_sysInfoPanel)   m_sysInfoPanel->setVisible(!m_sysInfoPanel->isVisible()); }

void MainWindow::onToolsGitFetch() {
    auto* t = currentTab(); if (!t) return;
    QString log, err;
    if (!GitOps::fetch(t->filePath(), QString(), log, &err)) {
        QMessageBox::warning(this, tr("Git"), err);
        return;
    }
    QMessageBox::information(this, tr("Git fetch"), log.isEmpty() ? tr("OK") : log);
}
void MainWindow::onToolsGitPull() {
    auto* t = currentTab(); if (!t) return;
    QString log, err;
    if (!GitOps::pull(t->filePath(), log, &err)) {
        QMessageBox::warning(this, tr("Git"), err);
        return;
    }
    QMessageBox::information(this, tr("Git pull"), log.isEmpty() ? tr("OK") : log);
}
void MainWindow::onToolsGitPush() {
    auto* t = currentTab(); if (!t) return;
    QString log, err;
    if (!GitOps::push(t->filePath(), QString(), QString(), log, &err)) {
        QMessageBox::warning(this, tr("Git"), err);
        return;
    }
    QMessageBox::information(this, tr("Git push"), log.isEmpty() ? tr("OK") : log);
}
void MainWindow::onToolsGitignoreEdit() {
    auto* t = currentTab(); if (!t) { QMessageBox::information(this, tr("gitignore"), tr("Abra um arquivo do repo primeiro.")); return; }
    GitignoreDialog dlg(t->filePath(), this);
    dlg.exec();
}
void MainWindow::onToolsPreCommitInstall() {
    auto* t = currentTab(); if (!t) { QMessageBox::information(this, tr("Hook"), tr("Abra um arquivo do repo primeiro.")); return; }
    PreCommitDialog dlg(t->filePath(), this);
    dlg.exec();
}
void MainWindow::onToolsApplyPatch() {
    auto* t = currentTab(); if (!t) return;
    const QString p = QFileDialog::getOpenFileName(this, tr("Patch"),
        QString(), tr("Patches (*.patch *.diff);;Todos (*)"));
    if (p.isEmpty()) return;
    QFile f(p); if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Patch"), tr("Não consegui abrir o patch."));
        return;
    }
    const QString text = QString::fromUtf8(f.readAll()); f.close();
    QString err;
    if (!GitOps::applyPatch(t->filePath(), text, /*check*/false, &err)) {
        QMessageBox::warning(this, tr("git apply"), err);
        return;
    }
    QMessageBox::information(this, tr("Patch"), tr("Aplicado com sucesso."));
}

void MainWindow::onToolsCodeMetrics() {
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    const QByteArray bytes = t->editor()->getText(t->editor()->textLength() + 1);
    const QString lex = LexerMap::lexerNameForPath(t->filePath());
    const auto r = CodeMetrics::analyze(QString::fromUtf8(bytes), lex);
    QString msg = tr(
        "Total:        %1 linhas\n"
        "Código:       %2\n"
        "Comentário:   %3\n"
        "Em branco:    %4\n"
        "Funções:      %5\n"
        "Cyclomatic:   %6")
        .arg(r.linesTotal).arg(r.linesCode).arg(r.linesComment)
        .arg(r.linesBlank).arg(r.functions).arg(r.cyclomatic);
    QMessageBox::information(this, tr("Métricas de código (%1)").arg(lex.isEmpty() ? tr("genérico") : lex), msg);
}

namespace {
QString aiSelectionOf(EditorTab* t) {
    if (!t || !t->editor()) return {};
    QByteArray sel = t->editor()->getSelText();
    if (sel.isEmpty()) sel = t->editor()->getText(t->editor()->textLength() + 1);
    return QString::fromUtf8(sel);
}
}

void MainWindow::onToolsAiExplain() {
    if (!m_aiPanel) return;
    auto* t = currentTab(); if (!t) return;
    m_aiPanel->setVisible(true); m_aiPanel->raise();
    m_aiPanel->setSelectionContext(aiSelectionOf(t));
    statusBar()->showMessage(tr("Use 'Usar seleção' + escreva 'Explique este código' no painel AI."), 5000);
}
void MainWindow::onToolsAiTranslate() {
    if (!m_aiPanel) return;
    auto* t = currentTab(); if (!t) return;
    bool ok = false;
    const QString lang = QInputDialog::getText(this, tr("Traduzir"),
        tr("Idioma alvo (ex.: en, es, fr):"), QLineEdit::Normal, QStringLiteral("en"), &ok);
    if (!ok || lang.isEmpty()) return;
    m_aiPanel->setVisible(true); m_aiPanel->raise();
    const QString sel = aiSelectionOf(t);
    m_aiPanel->setSelectionContext(QStringLiteral("Traduza para %1, preservando formatação:\n\n%2").arg(lang, sel));
    statusBar()->showMessage(tr("Painel AI: marque 'Usar seleção' e tecle Enviar."), 5000);
}
void MainWindow::onToolsAiCommitMsg() {
    if (!m_aiPanel) return;
    auto* t = currentTab(); if (!t) { QMessageBox::information(this, tr("AI"), tr("Abra um arquivo do repo primeiro.")); return; }
    // Pega diff staged (ou tudo, se nada staged).
    QProcess gd;
    gd.setWorkingDirectory(QFileInfo(t->filePath()).absolutePath());
    gd.start(QStringLiteral("git"), { QStringLiteral("diff"), QStringLiteral("--cached"), QStringLiteral("--no-color") });
    gd.waitForFinished(8000);
    QString diff = QString::fromUtf8(gd.readAllStandardOutput());
    if (diff.trimmed().isEmpty()) {
        gd.start(QStringLiteral("git"), { QStringLiteral("diff"), QStringLiteral("--no-color") });
        gd.waitForFinished(8000);
        diff = QString::fromUtf8(gd.readAllStandardOutput());
    }
    if (diff.trimmed().isEmpty()) { QMessageBox::information(this, tr("AI"), tr("Sem diff.")); return; }
    m_aiPanel->setVisible(true); m_aiPanel->raise();
    m_aiPanel->setSelectionContext(QStringLiteral(
        "Gere uma mensagem de commit concisa (1 linha imperativa, opcionalmente +corpo) "
        "para o diff abaixo:\n\n%1").arg(diff));
    statusBar()->showMessage(tr("Painel AI: marque 'Usar seleção' e tecle Enviar."), 5000);
}

// ---------------------------------------------------------------------------
// M21–M26 slots
// ---------------------------------------------------------------------------
void MainWindow::onToolsToggleAiGhost()
{
    if (!m_aiGhost) return;
    m_aiGhost->setEnabled(!m_aiGhost->isEnabled());
    statusBar()->showMessage(m_aiGhost->isEnabled()
        ? tr("AI ghost: ligado (Tab aceita, Esc descarta)")
        : tr("AI ghost: desligado"), 4000);
}
void MainWindow::onViewToggleDevTools()    { if (m_devTools)   m_devTools->setVisible(!m_devTools->isVisible()); }
void MainWindow::onViewToggleDbShell()     { if (m_dbShell)    m_dbShell->setVisible(!m_dbShell->isVisible()); }
void MainWindow::onViewToggleJsonLines()   { if (m_jsonl)      m_jsonl->setVisible(!m_jsonl->isVisible()); }
void MainWindow::onViewToggleDocPreview()  { if (m_docPreview) m_docPreview->setVisible(!m_docPreview->isVisible()); }
void MainWindow::onViewToggleClipboard()   { if (m_clipboard)  m_clipboard->setVisible(!m_clipboard->isVisible()); }
void MainWindow::onViewToggleArchive()     { if (m_archive)    m_archive->setVisible(!m_archive->isVisible()); }
void MainWindow::onViewToggleTimer()       { if (m_timer)      m_timer->setVisible(!m_timer->isVisible()); }

void MainWindow::onToolsSaveLayout()
{
    if (!m_layouts) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Salvar layout"),
        tr("Nome do preset:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    if (m_layouts->save(name.trimmed())) {
        statusBar()->showMessage(tr("Layout '%1' salvo.").arg(name), 4000);
    }
}
void MainWindow::onToolsApplyLayout()
{
    if (!m_layouts) return;
    const QStringList names = m_layouts->list();
    if (names.isEmpty()) {
        QMessageBox::information(this, tr("Layouts"), tr("Sem layouts salvos."));
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getItem(this, tr("Aplicar layout"),
        tr("Preset:"), names, 0, false, &ok);
    if (!ok || name.isEmpty()) return;
    if (m_layouts->apply(name)) {
        statusBar()->showMessage(tr("Layout '%1' aplicado.").arg(name), 4000);
    }
}

// ---------------------------------------------------------------------------
// M27–M35 slots
// ---------------------------------------------------------------------------
void MainWindow::onToolsToggleAutoCorrect() {
    if (!m_autoCorrect) return;
    m_autoCorrect->setEnabled(!m_autoCorrect->isEnabled());
    statusBar()->showMessage(m_autoCorrect->isEnabled()
        ? tr("AutoCorrect: ligado") : tr("AutoCorrect: desligado"), 3000);
}
void MainWindow::onViewToggleGraphQl()  { if (m_graphql)  m_graphql->setVisible(!m_graphql->isVisible()); }
void MainWindow::onViewToggleReview()   {
    if (!m_review) return;
    m_review->setVisible(!m_review->isVisible());
    if (m_review->isVisible()) {
        if (auto* t = currentTab()) m_review->setActiveFile(t->filePath());
    }
}
void MainWindow::onViewToggleProfile()  { if (m_profile)  m_profile->setVisible(!m_profile->isVisible()); }
void MainWindow::onViewToggleImgAnnot() { if (m_imgAnnot) m_imgAnnot->setVisible(!m_imgAnnot->isVisible()); }
void MainWindow::onViewToggleCsvChart() {
    if (!m_csvChart) return;
    m_csvChart->setVisible(!m_csvChart->isVisible());
    if (m_csvChart->isVisible()) {
        if (auto* t = currentTab(); t && t->editor()) m_csvChart->setActiveEditor(t->editor());
    }
}
void MainWindow::onViewToggleMdTable() {
    if (!m_mdTable) return;
    m_mdTable->setVisible(!m_mdTable->isVisible());
    if (m_mdTable->isVisible()) {
        if (auto* t = currentTab(); t && t->editor()) m_mdTable->setActiveEditor(t->editor());
    }
}
void MainWindow::onViewToggleCron()    { if (m_cron)    m_cron->setVisible(!m_cron->isVisible()); }
void MainWindow::onViewToggleWatcher() { if (m_watcher) m_watcher->setVisible(!m_watcher->isVisible()); }

void MainWindow::onViewToggleTextStats() {
    if (!m_textStats) return;
    m_textStats->setVisible(!m_textStats->isVisible());
    if (m_textStats->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_textStats->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleMindMap()      { if (m_mindMap)     m_mindMap->setVisible(!m_mindMap->isVisible()); }
void MainWindow::onViewTogglePastebin() {
    if (!m_pastebin) return;
    m_pastebin->setVisible(!m_pastebin->isVisible());
    if (m_pastebin->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_pastebin->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleTimeTracker()  { if (m_timeTracker) m_timeTracker->setVisible(!m_timeTracker->isVisible()); }
void MainWindow::onViewToggleCodeClones() {
    if (!m_codeClones) return;
    m_codeClones->setVisible(!m_codeClones->isVisible());
    if (m_codeClones->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_codeClones->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleCallGraph() {
    if (!m_callGraph) return;
    m_callGraph->setVisible(!m_callGraph->isVisible());
    if (m_callGraph->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_callGraph->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleSecrets() {
    if (!m_secrets) return;
    m_secrets->setVisible(!m_secrets->isVisible());
    if (m_secrets->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_secrets->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleScreenshot()   { if (m_screenshot)  m_screenshot->setVisible(!m_screenshot->isVisible()); }
void MainWindow::onViewToggleRss()          { if (m_rss)         m_rss->setVisible(!m_rss->isVisible()); }
void MainWindow::onViewToggleCliShell()     { if (m_cliShell)    m_cliShell->setVisible(!m_cliShell->isVisible()); }
void MainWindow::onViewToggleHex() {
    if (!m_hexPanel) return;
    m_hexPanel->setVisible(!m_hexPanel->isVisible());
    if (m_hexPanel->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_hexPanel->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleSqlSchema() {
    if (!m_sqlSchema) return;
    m_sqlSchema->setVisible(!m_sqlSchema->isVisible());
    if (m_sqlSchema->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_sqlSchema->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleBuildWatch()   { if (m_buildWatch)  m_buildWatch->setVisible(!m_buildWatch->isVisible()); }

void MainWindow::onViewToggleTodoAgg()  { if (m_todoAgg)  m_todoAgg->setVisible(!m_todoAgg->isVisible()); }
void MainWindow::onViewToggleGrep()     { if (m_grep)     m_grep->setVisible(!m_grep->isVisible()); }
void MainWindow::onViewToggleYamlVal() {
    if (!m_yamlVal) return;
    m_yamlVal->setVisible(!m_yamlVal->isVisible());
    if (m_yamlVal->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_yamlVal->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleDocker()   { if (m_docker)   m_docker->setVisible(!m_docker->isVisible()); }
void MainWindow::onViewToggleJq() {
    if (!m_jq) return;
    m_jq->setVisible(!m_jq->isVisible());
    if (m_jq->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_jq->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleHtmlPrev() {
    if (!m_htmlPrev) return;
    m_htmlPrev->setVisible(!m_htmlPrev->isVisible());
    if (m_htmlPrev->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_htmlPrev->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleGist() {
    if (!m_gist) return;
    m_gist->setVisible(!m_gist->isVisible());
    if (m_gist->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_gist->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleEnvMgr()   { if (m_envMgr)   m_envMgr->setVisible(!m_envMgr->isVisible()); }
void MainWindow::onViewToggleSshExec()  { if (m_sshExec)  m_sshExec->setVisible(!m_sshExec->isVisible()); }
void MainWindow::onViewToggleCheats()   { if (m_cheats)   m_cheats->setVisible(!m_cheats->isVisible()); }
void MainWindow::onViewToggleVault()    { if (m_vault)    m_vault->setVisible(!m_vault->isVisible()); }
void MainWindow::onViewToggleKubectl()  { if (m_kubectl)  m_kubectl->setVisible(!m_kubectl->isVisible()); }
void MainWindow::onViewToggleSysmon()   { if (m_sysmon)   m_sysmon->setVisible(!m_sysmon->isVisible()); }
void MainWindow::onViewToggleGpg() {
    if (!m_gpg) return;
    m_gpg->setVisible(!m_gpg->isVisible());
    if (m_gpg->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_gpg->setActiveEditor(t->editor());
}
void MainWindow::onViewToggleSsl()      { if (m_ssl)      m_ssl->setVisible(!m_ssl->isVisible()); }
void MainWindow::onViewToggleAdoc() {
    if (!m_adoc) return;
    m_adoc->setVisible(!m_adoc->isVisible());
    if (m_adoc->isVisible())
        if (auto* t = currentTab(); t && t->editor()) m_adoc->setActiveEditor(t->editor());
}
void MainWindow::onViewTogglePortScan() { if (m_portScan) m_portScan->setVisible(!m_portScan->isVisible()); }
void MainWindow::onViewToggleSystemd()  { if (m_systemd)  m_systemd->setVisible(!m_systemd->isVisible()); }
void MainWindow::onViewToggleTunnels()  { if (m_tunnels)  m_tunnels->setVisible(!m_tunnels->isVisible()); }
void MainWindow::onViewToggleApiBrowser() { if (m_apiBrowser) m_apiBrowser->setVisible(!m_apiBrowser->isVisible()); }
void MainWindow::onViewToggleLogTail()  { if (m_logTail)  m_logTail->setVisible(!m_logTail->isVisible()); }
void MainWindow::onViewToggleSshfs()    { if (m_sshfs)    m_sshfs->setVisible(!m_sshfs->isVisible()); }

void MainWindow::onEditExpandSelection() {
    if (auto* t = currentTab(); t && t->editor()) SmartSelection::expand(t->editor());
}
void MainWindow::onEditShrinkSelection() {
    if (auto* t = currentTab(); t && t->editor()) SmartSelection::shrink(t->editor());
}
void MainWindow::onEditRenameInScope() {
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Renomear no escopo"),
        tr("Novo nome:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    const int n = SmartSelection::renameInScope(t->editor(), name.trimmed());
    statusBar()->showMessage(tr("%1 substituições no escopo.").arg(n), 4000);
}

void MainWindow::onToolsValidateJsonSchema()
{
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    // The active buffer is the *instance*. Ask the user for a schema file.
    const QString schemaPath = QFileDialog::getOpenFileName(this,
        tr("Schema JSON"), QString(), tr("JSON Schema (*.json *.schema.json);;Todos (*)"));
    if (schemaPath.isEmpty()) return;

    QFile sf(schemaPath);
    if (!sf.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Schema"), tr("Não consegui abrir o schema."));
        return;
    }
    QJsonParseError pe{};
    const QJsonDocument schemaDoc = QJsonDocument::fromJson(sf.readAll(), &pe);
    sf.close();
    if (pe.error != QJsonParseError::NoError || !schemaDoc.isObject()) {
        QMessageBox::warning(this, tr("Schema"), tr("Schema JSON inválido: %1").arg(pe.errorString()));
        return;
    }

    const QByteArray bufBytes = t->editor()->getText(t->editor()->textLength() + 1);
    QJsonParseError ie{};
    const QJsonDocument instDoc = QJsonDocument::fromJson(bufBytes, &ie);
    if (ie.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("Schema"),
            tr("Buffer não é JSON válido: %1").arg(ie.errorString()));
        return;
    }

    QJsonValue inst = instDoc.isArray() ? QJsonValue(instDoc.array())
                                         : QJsonValue(instDoc.object());
    const QStringList errors = JsonSchemaValidator::validate(inst, schemaDoc.object());
    if (errors.isEmpty()) {
        QMessageBox::information(this, tr("Schema"), tr("✓ Buffer válido contra o schema."));
    } else {
        QMessageBox::warning(this, tr("Schema"),
            tr("%1 erro(s):\n\n%2").arg(errors.size()).arg(errors.join('\n')));
    }
}

void MainWindow::onViewToggleGitLog() {
    if (!m_gitLogPanel || !m_actToggleGitLog) return;
    m_gitLogPanel->setVisible(m_actToggleGitLog->isChecked());
    if (m_actToggleGitLog->isChecked()) {
        if (auto* t = currentTab()) m_gitLogPanel->setAnchorFile(t->filePath());
    }
}

void MainWindow::onToolsGitBlame() {
    auto* t = currentTab();
    if (!t || !t->editor() || t->filePath().isEmpty()) {
        QToolTip::showText(QCursor::pos(), tr("Salve o arquivo antes de usar blame."));
        return;
    }
    auto* sci = t->editor();
    const int line = static_cast<int>(sci->lineFromPosition(sci->currentPos())) + 1;
    GitOps::BlameLine bl;
    QString err;
    if (!GitOps::blameLine(t->filePath(), line, bl, &err)) {
        QToolTip::showText(QCursor::pos(), tr("git blame: %1").arg(err));
        return;
    }
    const QString shortSha = bl.sha.left(8);
    const QString text = tr("%1  %2  %3\n%4")
                             .arg(shortSha, bl.authorDate, bl.author, bl.summary);
    QToolTip::showText(QCursor::pos(), text);
}

void MainWindow::onToolsGitCommit() {
    auto* t = currentTab();
    const QString anchor = t ? t->filePath() : QString();
    if (anchor.isEmpty()) {
        QMessageBox::information(this, tr("Commit"),
            tr("Abra um arquivo do repositório que você quer commitar."));
        return;
    }
    GitCommitDialog dlg(anchor, this);
    dlg.exec();
}

void MainWindow::onToolsSshConnect() {
    SshConnectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    if (!m_terminalPanel) return;

    // Re-purpose the terminal panel for the SSH session. Single panel keeps
    // the dock layout simple — the user can run ssh / sftp / scp from the
    // shell prompt the same panel already exposes for local commands.
    m_terminalPanel->show();
    m_terminalPanel->raise();
    if (m_actToggleTerminal) m_actToggleTerminal->setChecked(true);
    m_terminalPanel->setWindowTitle(
        tr("Terminal — ssh %1").arg(dlg.host()));
    m_terminalPanel->startCommand(QStringLiteral("ssh"), dlg.sshArgs(),
                                  tr("Terminal — ssh %1").arg(dlg.host()));
}

void MainWindow::onExecOutputLocationActivated(const QString& filePath, int line, int column) {
    // ExecOutputPanel uses 1-based line; reuse the LSP definition handler
    // which expects 0-based and applies a column-byte fallback.
    onLspDefinitionResolved(filePath, line - 1, column - 1);
}

// ---------------------------------------------------------------------------
// M8 LSP feature slots
// ---------------------------------------------------------------------------
void MainWindow::onLspGotoDefinition() {
    if (m_lspFeatures) m_lspFeatures->requestDefinitionAtCaret();
}

void MainWindow::onLspShowHover() {
    if (m_lspFeatures) m_lspFeatures->requestHoverAtCaret();
}

void MainWindow::onLspTriggerCompletion() {
    if (m_lspFeatures) m_lspFeatures->requestCompletionAtCaret();
}

void MainWindow::onLspHoverReady(const QString& text, int globalX, int globalY) {
    if (text.isEmpty()) return;
    QToolTip::showText(QPoint(globalX, globalY), text);
}

void MainWindow::onLspDefinitionResolved(const QString& filePath, int line, int column) {
    if (filePath.isEmpty()) return;
    int idx = findTabByPath(filePath);
    if (idx < 0) {
        openFile(filePath);
        idx = findTabByPath(filePath);
    }
    if (idx < 0) return;
    setActiveTab(tabAt(idx));
    if (auto* t = currentTab(); t && t->editor()) {
        auto* sci = t->editor();
        sci->gotoLine(line);
        // Approximate column→byte conversion: walk current line and stop at `column`
        // UTF-16 code units. ASCII files match exactly; non-ASCII is best-effort.
        QByteArray lineBytes = sci->getLine(line);
        const int lineStart  = static_cast<int>(sci->positionFromLine(line));
        const int targetByte = qMin(column, static_cast<int>(lineBytes.size()));
        sci->gotoPos(lineStart + targetByte);
        sci->scrollCaret();
    }
}

// ---------------------------------------------------------------------------
// M9 LSP slots
// ---------------------------------------------------------------------------
namespace {

// Tiny LSP SymbolKind → glyph for the picker dialogs. Matches the well-known
// LSP enum values (1..26).
QString lspSymbolGlyph(int kind) {
    switch (kind) {
        case 5:  return QStringLiteral("[C]");   // Class
        case 6:  return QStringLiteral("[m]");   // Method
        case 7:  return QStringLiteral("[p]");   // Property
        case 9:  return QStringLiteral("[fn]");  // Constructor
        case 10: return QStringLiteral("[E]");   // Enum
        case 11: return QStringLiteral("[I]");   // Interface
        case 12: return QStringLiteral("[fn]");  // Function
        case 13: return QStringLiteral("[v]");   // Variable
        case 14: return QStringLiteral("[c]");   // Constant
        case 22: return QStringLiteral("[S]");   // Struct
        case 23: return QStringLiteral("[ev]");  // Event
        case 26: return QStringLiteral("[T]");   // TypeParameter
        default: return QStringLiteral("[?]");
    }
}

// Same UTF-8 byte ↔ UTF-16 unit walker as LspFeatures (kept local to avoid
// bringing the header dependency in for one helper).
int bytesForUtf16Prefix(const char* data, int byteCount, int chars) {
    int units = 0, i = 0;
    while (i < byteCount && units < chars) {
        const unsigned char c = static_cast<unsigned char>(data[i]);
        int cpLen; unsigned int cp;
        if      (c < 0x80)            { cpLen = 1; cp = c; }
        else if ((c & 0xE0) == 0xC0)  { cpLen = 2; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0)  { cpLen = 3; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0)  { cpLen = 4; cp = c & 0x07; }
        else                          { cpLen = 1; cp = 0xFFFD; }
        if (i + cpLen > byteCount) break;
        for (int k = 1; k < cpLen; ++k)
            cp = (cp << 6) | (static_cast<unsigned char>(data[i + k]) & 0x3F);
        const int newUnits = units + ((cp > 0xFFFF) ? 2 : 1);
        if (newUnits > chars) break;
        units = newUnits;
        i += cpLen;
    }
    return i;
}

} // namespace

void MainWindow::onLspSignatureHelp() {
    if (m_lspFeatures) m_lspFeatures->requestSignatureHelpAtCaret();
}
void MainWindow::onLspFindReferences() {
    if (m_lspFeatures) m_lspFeatures->requestReferencesAtCaret(true);
}
void MainWindow::onLspDocumentSymbols() {
    if (m_lspFeatures) m_lspFeatures->requestDocumentSymbolsCurrent();
}

void MainWindow::onLspWorkspaceSymbols() {
    if (!m_lspFeatures) return;
    // Live-query dialog: each filter keystroke kicks a new workspace/symbol
    // request. We open the dialog with an empty list and replace contents
    // every time the server responds.
    auto* dlg = new QuickPickDialog(this, tr("Símbolos do workspace (LSP)"), {});
    dlg->setLiveQueryMode(true);
    connect(dlg, &QuickPickDialog::queryChanged, this, [this, dlg](const QString& text) {
        if (text.trimmed().size() < 2) { dlg->replaceItems({}); return; }
        m_lspFeatures->requestWorkspaceSymbolsForQuery(text);
    });
    // One-shot listener: when symbols arrive, push them into the dialog.
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(m_lspFeatures, &LspFeatures::workspaceSymbolsReady,
                    this, [dlg, conn](const QList<LspSymbol>& syms) {
        QList<QuickPickDialog::Item> items;
        for (const LspSymbol& s : syms) {
            QuickPickDialog::Item it;
            it.label    = lspSymbolGlyph(s.kind) + QStringLiteral(" ") + s.name;
            it.subtitle = (s.containerName.isEmpty() ? QString() : s.containerName + QStringLiteral(" — "))
                        + QFileInfo(s.filePath).fileName()
                        + QStringLiteral(":") + QString::number(s.line + 1);
            it.data     = QVariant::fromValue(s);
            items.append(it);
        }
        dlg->replaceItems(items);
    });
    if (dlg->exec() == QDialog::Accepted) {
        const QVariant v = dlg->pickedData();
        if (v.canConvert<LspSymbol>()) {
            const LspSymbol s = v.value<LspSymbol>();
            onLspDefinitionResolved(s.filePath, s.line, s.column);
        }
    }
    QObject::disconnect(*conn);
    dlg->deleteLater();
}

void MainWindow::onLspRename() {
    if (!m_lspFeatures) return;
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Renomear símbolo"),
        tr("Novo nome:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || newName.isEmpty()) return;
    m_lspFeatures->requestRenameAtCaret(newName);
}

void MainWindow::onLspSignatureHelpReady(const QString& text, int /*x*/, int /*y*/) {
    if (text.isEmpty()) return;
    auto* t = currentTab();
    if (!t || !t->editor()) return;
    t->editor()->callTipShow(t->editor()->currentPos(), text.toUtf8().constData());
}

void MainWindow::onLspReferencesReady(const QList<LspLocation>& locs) {
    if (locs.isEmpty()) {
        QMessageBox::information(this, tr("Referências"),
            tr("Nenhuma referência encontrada."));
        return;
    }
    QList<QuickPickDialog::Item> items;
    for (const LspLocation& l : locs) {
        QuickPickDialog::Item it;
        it.label    = QFileInfo(l.filePath).fileName()
                    + QStringLiteral(":") + QString::number(l.line + 1)
                    + QStringLiteral(":") + QString::number(l.column + 1);
        it.subtitle = l.filePath;
        it.data     = QVariant::fromValue(l);
        items.append(it);
    }
    QuickPickDialog dlg(this, tr("Referências (%1)").arg(locs.size()), items);
    if (dlg.exec() == QDialog::Accepted) {
        const QVariant v = dlg.pickedData();
        if (v.canConvert<LspLocation>()) {
            const LspLocation l = v.value<LspLocation>();
            onLspDefinitionResolved(l.filePath, l.line, l.column);
        }
    }
}

void MainWindow::onLspDocumentSymbolsReady(const QList<LspSymbol>& syms) {
    if (syms.isEmpty()) {
        QMessageBox::information(this, tr("Símbolos"),
            tr("O servidor não retornou símbolos para este arquivo."));
        return;
    }
    QList<QuickPickDialog::Item> items;
    for (const LspSymbol& s : syms) {
        QuickPickDialog::Item it;
        it.label    = lspSymbolGlyph(s.kind) + QStringLiteral(" ") + s.name;
        it.subtitle = (s.containerName.isEmpty() ? QString() : s.containerName + QStringLiteral(" — "))
                    + QStringLiteral("linha ") + QString::number(s.line + 1);
        it.data     = QVariant::fromValue(s);
        items.append(it);
    }
    QuickPickDialog dlg(this, tr("Símbolos do documento"), items);
    if (dlg.exec() == QDialog::Accepted) {
        const QVariant v = dlg.pickedData();
        if (v.canConvert<LspSymbol>()) {
            const LspSymbol s = v.value<LspSymbol>();
            onLspDefinitionResolved(s.filePath, s.line, s.column);
        }
    }
}

void MainWindow::onLspWorkspaceSymbolsReady(const QList<LspSymbol>&) {
    // Handled inline by the connect() in onLspWorkspaceSymbols(): the dialog
    // is updated through that one-shot connection so we don't need a slot
    // body here. Kept to keep the signal/slot wiring symmetric.
}

// ---------------------------------------------------------------------------
// M11: code actions / inlay hints
// ---------------------------------------------------------------------------
void MainWindow::onLspCodeActions() {
    if (m_lspFeatures) m_lspFeatures->requestCodeActionsAtCaret();
}

void MainWindow::onLspInlayHintsRefresh() {
    if (m_lspFeatures) m_lspFeatures->requestInlayHintsForVisibleRange();
}

void MainWindow::onLspCodeActionsReady(const QList<LspCodeAction>& actions) {
    if (actions.isEmpty()) {
        statusBar()->showMessage(tr("Nenhuma code action disponível para esta linha."), 4000);
        return;
    }
    QList<QuickPickDialog::Item> items;
    for (int i = 0; i < actions.size(); ++i) {
        QuickPickDialog::Item it;
        it.label    = actions[i].title;
        it.subtitle = actions[i].isCommandOnly
                          ? tr("(precisa do servidor — não aplicável aqui)")
                          : actions[i].kind;
        it.data     = i;   // index back into the captured actions list
        items.append(it);
    }
    QuickPickDialog dlg(this, tr("Code actions"), items);
    if (dlg.exec() != QDialog::Accepted) return;
    const int picked = dlg.pickedData().toInt(/*ok*/nullptr);
    if (picked < 0 || picked >= actions.size()) return;
    const LspCodeAction& chosen = actions[picked];
    if (chosen.isCommandOnly) {
        QMessageBox::information(this, tr("Code action"),
            tr("Esta ação requer execução server-side, ainda não suportada."));
        return;
    }
    // Reuse the rename apply pipeline — same WorkspaceEdit shape, same byte conversion.
    onLspRenameEditsReady(chosen.edits, chosen.title);
}

void MainWindow::onLspInlayHintsReady(const QString& filePath, const QList<LspInlayHint>& hints) {
    int idx = findTabByPath(filePath);
    if (idx < 0) return;
    auto* t = tabAt(idx);
    if (!t || !t->editor()) return;
    auto* sci = t->editor();

    // Render via Scintilla EOL annotations (one per line). Earlier annotations
    // for this buffer are cleared first so stale hints don't accumulate.
    sci->annotationClearAll();
    sci->annotationSetVisible(static_cast<int>(Scintilla::AnnotationVisible::Boxed));
    // Aggregate hints per line, joined with tab separators so multiple hints
    // on the same line stay on one annotation row.
    QHash<int, QStringList> byLine;
    for (const LspInlayHint& h : hints) {
        QString label = h.label;
        if (h.kind == 2) label = QStringLiteral("‹ ") + label + QStringLiteral(" ›");   // parameter
        if (h.kind == 1) label = QStringLiteral(": ") + label;                          // type
        byLine[h.line].append(label);
    }
    for (auto it = byLine.constBegin(); it != byLine.constEnd(); ++it) {
        sci->annotationSetText(it.key(), it.value().join(QStringLiteral("  ")).toUtf8().constData());
        // Use style 0 (default) — themes already render annotations subtly.
        sci->annotationSetStyle(it.key(), 0);
    }
}

void MainWindow::onLspRenameEditsReady(const QList<LspTextEdit>& edits, const QString& newName) {
    if (edits.isEmpty()) {
        QMessageBox::information(this, tr("Renomear"),
            tr("O servidor não retornou edições aplicáveis."));
        return;
    }
    // Group edits by file so we can apply them in reverse position order
    // (back to front) per buffer — that keeps earlier offsets valid.
    QHash<QString, QList<LspTextEdit>> byFile;
    for (const LspTextEdit& e : edits) byFile[e.filePath].append(e);

    int totalApplied = 0;
    QStringList touchedFiles;
    for (auto it = byFile.begin(); it != byFile.end(); ++it) {
        const QString& path = it.key();
        QList<LspTextEdit>& fileEdits = it.value();

        // Sort by (startLine, startColumn) descending.
        std::sort(fileEdits.begin(), fileEdits.end(),
                  [](const LspTextEdit& a, const LspTextEdit& b) {
                      if (a.startLine != b.startLine) return a.startLine > b.startLine;
                      return a.startColumn > b.startColumn;
                  });

        // Open the file if it's not already a tab.
        int idx = findTabByPath(path);
        if (idx < 0) {
            openFile(path);
            idx = findTabByPath(path);
        }
        if (idx < 0) continue;
        EditorTab* tab = tabAt(idx);
        if (!tab || !tab->editor()) continue;
        auto* sci = tab->editor();

        for (const LspTextEdit& e : fileEdits) {
            const int startLineByte = static_cast<int>(sci->positionFromLine(e.startLine));
            const int endLineByte   = static_cast<int>(sci->positionFromLine(e.endLine));
            QByteArray startLineBuf = sci->getLine(e.startLine);
            QByteArray endLineBuf   = sci->getLine(e.endLine);
            const int startByte = startLineByte + bytesForUtf16Prefix(
                startLineBuf.constData(), startLineBuf.size(), e.startColumn);
            const int endByte   = endLineByte + bytesForUtf16Prefix(
                endLineBuf.constData(), endLineBuf.size(), e.endColumn);
            sci->setSel(startByte, endByte);
            sci->replaceSel(e.newText.toUtf8().constData());
            ++totalApplied;
        }
        touchedFiles << path;
    }

    QMessageBox::information(this, tr("Renomear símbolo"),
        tr("Aplicadas %1 edições em %2 arquivo(s) para \"%3\". "
           "Salve manualmente para persistir.")
           .arg(totalApplied).arg(touchedFiles.size()).arg(newName));
}

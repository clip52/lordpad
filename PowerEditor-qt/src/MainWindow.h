#pragma once
#include <QMainWindow>
#include <QString>

class QLabel;
class QAction;
class QActionGroup;
class EditorTab;
class FindReplaceDialog;
class ComparePanel;
class CssPreviewPane;
class CsvTableView;
class MultiView;
class LanguagesMenu;
class FunctionListPanel;
class DocumentMapPanel;
class FileBrowserPanel;
class ExecOutputPanel;
class TerminalPanel;
class FtpPanel;
class SftpPanel;
class GitLogPanel;
class CalendarPanel;
class CalculatorPanel;
class TodoPanel;
class QrPanel;
class MermaidPanel;
class TasksRunner;
class MergeResolverPanel;
class TestRunnerPanel;
class FormatOnSave;
class NotebookPanel;
class NotesPanel;
class TypewriterMode;
class VimMode;
class RestClientPanel;
class SqlitePanel;
class AiPanel;
class PoEditorPanel;
class CoverageGutter;
class AutoPair;
class SmartIndent;
class ImageViewerPanel;
class RegexTesterPanel;
class UnitConverterPanel;
class JsonPathPanel;
class ColorPalettePanel;
class SysInfoPanel;
class AiGhostCompletion;
class LayoutPresets;
class DevToolsPanel;
class DbShellPanel;
class JsonLinesPanel;
class DocPreviewPanel;
class ClipboardHistoryPanel;
class ArchiveBrowserPanel;
class TimerPanel;
class AutoCorrect;
class GraphQlPanel;
class CodeReviewPanel;
class ProfileRunnerPanel;
class ImageAnnotPanel;
class CsvChartPanel;
class MdTablePanel;
class CronEditorPanel;
class FileWatcherPanel;
class TextStatsPanel;
class MindMapPanel;
class PastebinPanel;
class TimeTrackerPanel;
class CodeClonesPanel;
class CallGraphPanel;
class SecretScannerPanel;
class ScreenshotPanel;
class RssReaderPanel;
class CliShellPanel;
class HexViewerPanel;
class SqlSchemaPanel;
class BuildWatchPanel;
class TodoAggregatorPanel;
class GrepPanel;
class YamlValidatorPanel;
class DockerPanel;
class JqRunnerPanel;
class HtmlPreviewPanel;
class GistPanel;
class EnvManagerPanel;
class SshExecPanel;
class CheatsheetPanel;
class VaultPanel;
class KubectlPanel;
class SystemMonitorPanel;
class GpgPanel;
class SslCertViewerPanel;
class AsciiDocPreviewPanel;
class PortScanPanel;
class SystemdServicesPanel;
class TunnelManagerPanel;
class ApiBrowserPanel;
class LogTailPanel;
class SshfsPanel;
class MarkdownPreviewPane;
class HexViewer;
class FindInFilesDialog;
class CommandPalette;
class AutoSavePolicy;
class SessionManager;
class AutoCompleter;
class BookmarkManager;
class BookmarkDialog;
class WordCountDialog;
class MacroRecorder;
class MacroDialog;
class EolMenu;
class SpellChecker;
class ExternalFileWatcher;
class BraceMatcher;
class WhitespaceView;
class Snippets;
class SnippetsDialog;
class RecentProjects;
class EditEnhancements;
class HashDialog;
class TabExtras;
class CodeFormatter;
class GitStatusService;
class CrashRecovery;
class Workspace;
class ColorMarkers;
class FoldingMenu;
class GitDiffGutter;
class LspClient;
class LspFeatures;
class LspDiagnosticsPanel;
class EncodingConvert;
class ViewModes;
class PythonPluginHost;
class KeybindingsManager;
class StickyScroll;
class QMenu;
struct GitStatus;
struct LspDiagnostic;
struct LspLocation;
struct LspSymbol;
struct LspTextEdit;
struct LspCodeAction;
struct LspInlayHint;
struct LspSemanticToken;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    // M12 multi-window: Primary owns the plugin host, the keybindings table
    // and the global session. Secondary windows are fully-featured editors
    // (LSP, git, sftp, terminal, todos os panels do M11/M12) mas pulam
    // plugin host (Python só inicializa 1× por processo) e session restore.
    enum class Mode { Primary, Secondary };

    explicit MainWindow(QWidget* parent = nullptr, Mode mode = Mode::Primary);
    ~MainWindow() override;

    void openFile(const QString& path);
    Mode windowMode() const { return m_mode; }

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onFileClose();
    void onFileExit();

    void onEditUndo();
    void onEditRedo();
    void onEditCut();
    void onEditCopy();
    void onEditPaste();
    void onEditSelectAll();

    void onSearchFind();
    void onSearchReplace();
    void onSearchGoToLine();

    void onViewSetThemeLight();
    void onViewSetThemeDark();
    void onViewSetThemeDracula();
    void onViewToggleLineNumbers();
    void onViewToggleWordWrap();
    void onViewSplitHorizontal();
    void onViewSplitVertical();
    void onViewUnsplit();
    void onViewMoveTabToOtherGroup();

    void onLanguageSelected(const QString& lexerName);

    void onToolsPreferences();
    void onToolsCompare();
    void onToolsCssPreview();
    void onToolsCsvView();
    void onToolsMarkdownPreview();
    void onToolsHexViewer();
    void onToolsFindInFiles();
    void onToolsCommandPalette();
    void onToolsRunCommand();          // focuses the Exec panel cmdline

    void onViewToggleFunctionList();
    void onViewToggleDocumentMap();
    void onViewToggleFileBrowser();
    void onViewToggleExecOutput();

    void onFileBrowserOpenFile(const QString& path);
    void onFindInFilesOpen(const QString& path, int line);
    void onFunctionListGoto(int line);
    void onAutoSaveTick();

    // Edit operations
    void onEditTrimWhitespace();
    void onEditUpperCase();
    void onEditLowerCase();
    void onEditTitleCase();
    void onEditSortAsc();
    void onEditSortDesc();
    void onEditSortUnique();
    void onEditDuplicateLine();
    void onEditMoveLineUp();
    void onEditMoveLineDown();
    void onEditTabsToSpaces();
    void onEditSpacesToTabs();

    // Bookmarks
    void onBookmarkToggle();
    void onBookmarkNext();
    void onBookmarkPrev();
    void onBookmarkClearAll();
    void onBookmarkList();

    // Tools (M4)
    void onToolsWordCount();
    void onToolsJsonPretty();
    void onToolsJsonMinify();
    void onToolsXmlPretty();
    void onToolsXmlMinify();
    void onToolsPickColor();
    void onToolsMacroDialog();

    // M5
    void onFilePrint();
    void onFilePrintPreview();
    void onFileReloadFromDisk();
    void onFileOpenFolder();
    void onSearchGotoMatchingBrace();
    void onViewToggleWhitespace();
    void onViewToggleEol();
    void onViewToggleIndentGuides();
    void onToolsHash();
    void onToolsSnippets();
    void onToolsToggleSpellCheck();
    void onExternalFileChanged(const QString& path);
    void onExternalFileRemoved(const QString& path);

    // M6 slots
    void onToolsCodeFormat();
    void onWorkspaceOpen();
    void onWorkspaceSave();
    void onWorkspaceSaveAs();
    void onRecentWorkspaceTriggered();
    void onThemePackSelected();
    void onGitStatusReady(const QString& path, const GitStatus& status);

    // M7 slots
    void onMultiCursorAddBelow();
    void onMultiCursorAddAbove();
    void onMultiCursorSelectAllOccurrences();
    void onMultiCursorAddNextOccurrence();
    void onSmartHighlightToggle();
    void onColorMarkerToggle();   // QAction holds MarkColor in data()
    void onColorMarkerNext();
    void onColorMarkerClearAll();
    void onGitDiffGutterToggle();
    void onLspDiagnosticsUpdated(const QString& filePath,
                                 const QList<LspDiagnostic>& diags);
    void onLspDiagnosticActivated(const QString& filePath, int line, int column);
    void onLspServerError(const QString& serverName, const QString& message);
    void onCurrentTabTextModified();

    // M8 LSP feature hooks
    void onLspGotoDefinition();
    void onLspShowHover();
    void onLspTriggerCompletion();
    void onLspHoverReady(const QString& text, int globalX, int globalY);
    void onLspDefinitionResolved(const QString& filePath, int line, int column);

    // M9 plugin slots
    void onToolsPluginManager();

    // M9 LSP slots
    void onLspSignatureHelp();
    void onLspFindReferences();
    void onLspDocumentSymbols();
    void onLspWorkspaceSymbols();
    void onLspRename();
    void onLspSignatureHelpReady(const QString& text, int globalX, int globalY);
    void onLspReferencesReady(const QList<LspLocation>& locs);
    void onLspDocumentSymbolsReady(const QList<LspSymbol>& syms);
    void onLspWorkspaceSymbolsReady(const QList<LspSymbol>& syms);
    void onLspRenameEditsReady(const QList<LspTextEdit>& edits, const QString& newName);

    // M11
    void onLspCodeActions();
    void onLspInlayHintsRefresh();
    void onLspCodeActionsReady(const QList<LspCodeAction>& actions);
    void onLspInlayHintsReady(const QString& filePath, const QList<LspInlayHint>& hints);

    // M9 misc
    void onExecOutputLocationActivated(const QString& filePath, int line, int column);
    void onViewToggleTerminal();
    void onViewToggleFtp();
    void onViewToggleSftp();
    void onToolsSshConnect();
    void onToolsKeybindings();
    void onToolsGitBlame();
    void onToolsGitCommit();
    void onViewToggleGitLog();

    // M12
    void onFileNewWindow();
    void onToolsGitBranches();
    void onToolsGitStash();
    void onToolsTasksRun();
    void onViewToggleCalendar();
    void onViewToggleCalculator();
    void onViewToggleTodo();
    void onViewToggleQr();
    void onViewToggleMermaid();

    // M13
    void onViewToggleMergeResolver();
    void onViewToggleTestRunner();
    void onToolsToggleFormatOnSave();
    void onLspSemanticTokensReady(const QString& filePath, const QList<LspSemanticToken>& tokens);
    void onLspOutlineSymbolsReady(const QString& filePath, const QList<LspSymbol>& syms);

    // M14
    void onViewToggleNotebook();
    void onViewToggleNotes();
    void onViewToggleTypewriter();
    void onToolsToggleVimMode();
    void onToolsThemeEditor();
    void onToolsExtractFunction();

    // M15
    void onToolsQuickSwitch();
    void onViewToggleRest();
    void onViewToggleSqlite();

    // M16
    void onViewToggleAi();
    void onViewTogglePoEditor();
    void onToolsLoadCoverage();
    void onToolsValidateJsonSchema();

    // M17–M20
    void onToolsToggleAutoPair();
    void onToolsToggleSmartIndent();
    void onToolsSurroundSelection();
    void onViewToggleImageViewer();
    void onViewToggleRegexTester();
    void onViewToggleUnitConverter();
    void onViewToggleJsonPath();
    void onViewToggleColorPalette();
    void onViewToggleSysInfo();
    void onToolsGitFetch();
    void onToolsGitPull();
    void onToolsGitPush();
    void onToolsGitignoreEdit();
    void onToolsPreCommitInstall();
    void onToolsApplyPatch();
    void onToolsCodeMetrics();
    void onToolsAiExplain();
    void onToolsAiTranslate();
    void onToolsAiCommitMsg();

    // M21–M26
    void onToolsToggleAiGhost();
    void onViewToggleDevTools();
    void onViewToggleDbShell();
    void onViewToggleJsonLines();
    void onViewToggleDocPreview();
    void onViewToggleClipboard();
    void onViewToggleArchive();
    void onViewToggleTimer();
    void onToolsSaveLayout();
    void onToolsApplyLayout();

    // M27–M35
    void onToolsToggleAutoCorrect();
    void onViewToggleGraphQl();
    void onViewToggleReview();
    void onViewToggleProfile();
    void onViewToggleImgAnnot();
    void onViewToggleCsvChart();
    void onViewToggleMdTable();
    void onViewToggleCron();
    void onViewToggleWatcher();
    void onViewToggleTextStats();
    void onViewToggleMindMap();
    void onViewTogglePastebin();
    void onViewToggleTimeTracker();
    void onViewToggleCodeClones();
    void onViewToggleCallGraph();
    void onViewToggleSecrets();
    void onViewToggleScreenshot();
    void onViewToggleRss();
    void onViewToggleCliShell();
    void onViewToggleHex();
    void onViewToggleSqlSchema();
    void onViewToggleBuildWatch();
    void onViewToggleTodoAgg();
    void onViewToggleGrep();
    void onViewToggleYamlVal();
    void onViewToggleDocker();
    void onViewToggleJq();
    void onViewToggleHtmlPrev();
    void onViewToggleGist();
    void onViewToggleEnvMgr();
    void onViewToggleSshExec();
    void onViewToggleCheats();
    void onViewToggleVault();
    void onViewToggleKubectl();
    void onViewToggleSysmon();
    void onViewToggleGpg();
    void onViewToggleSsl();
    void onViewToggleAdoc();
    void onViewTogglePortScan();
    void onViewToggleSystemd();
    void onViewToggleTunnels();
    void onViewToggleApiBrowser();
    void onViewToggleLogTail();
    void onViewToggleSshfs();
    void onEditExpandSelection();
    void onEditShrinkSelection();
    void onEditRenameInScope();

    void onHelpAbout();

    void onMultiViewCurrentChanged(EditorTab* tab);
    void onMultiViewTabCloseRequested(EditorTab* tab);
    void onCurrentTabModified(bool modified);
    void onCurrentTabFilePathChanged(const QString& path);
    void onCursorPositionChanged(int line, int column);

    void onRecentFileTriggered();

private:
    EditorTab* currentTab() const;
    EditorTab* tabAt(int index) const;
    int findTabByPath(const QString& path) const;
    void setTabTitle(EditorTab* tab, const QString& title);
    void setTabTooltip(EditorTab* tab, const QString& tooltip);
    void setActiveTab(EditorTab* tab);

    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();

    bool maybeSaveTab(EditorTab* tab);
    bool saveTab(EditorTab* tab);
    bool saveTabAs(EditorTab* tab);
    void loadFileIntoTab(EditorTab* tab, const QString& path);
    void applyEditorPreferences(EditorTab* tab);
    void applyThemeAndLexer(EditorTab* tab);
    void applyThemeToAllTabs();
    void updateWindowTitle();
    void updateStatusBar();
    void rebuildRecentFilesMenu();
    void connectTabSignals(EditorTab* tab);
    void registerTabForRecovery(EditorTab* tab);
    void rebuildRecentWorkspacesMenu();
    void applyWorkspaceData();

    MultiView* m_multiView;
    LanguagesMenu* m_languagesMenu;
    FindReplaceDialog* m_findDialog;
    ComparePanel* m_comparePanel;
    CssPreviewPane* m_cssPreviewPane;
    CsvTableView* m_csvTableView;
    MarkdownPreviewPane* m_markdownPreviewPane;
    HexViewer* m_hexViewer;
    FindInFilesDialog* m_findInFilesDialog;
    CommandPalette* m_commandPalette;

    // Dock panels
    FunctionListPanel* m_functionListPanel;
    DocumentMapPanel*  m_documentMapPanel;
    FileBrowserPanel*  m_fileBrowserPanel;
    ExecOutputPanel*   m_execOutputPanel;
    TerminalPanel*     m_terminalPanel;
    FtpPanel*          m_ftpPanel;
    SftpPanel*         m_sftpPanel;
    GitLogPanel*       m_gitLogPanel;
    // M12 utility panels
    CalendarPanel*     m_calendarPanel = nullptr;
    CalculatorPanel*   m_calculatorPanel = nullptr;
    TodoPanel*         m_todoPanel = nullptr;
    QrPanel*           m_qrPanel = nullptr;
    MermaidPanel*      m_mermaidPanel = nullptr;
    TasksRunner*       m_tasksRunner = nullptr;
    // M13
    MergeResolverPanel* m_mergePanel = nullptr;
    TestRunnerPanel*    m_testRunner = nullptr;
    FormatOnSave*       m_formatOnSave = nullptr;
    // M14
    NotebookPanel*      m_notebookPanel = nullptr;
    NotesPanel*         m_notesPanel    = nullptr;
    TypewriterMode*     m_typewriter    = nullptr;
    VimMode*            m_vim           = nullptr;
    QLabel*             m_statusVim     = nullptr;
    // M15/M16
    RestClientPanel*    m_restPanel    = nullptr;
    SqlitePanel*        m_sqlitePanel  = nullptr;
    AiPanel*            m_aiPanel      = nullptr;
    PoEditorPanel*      m_poPanel      = nullptr;
    CoverageGutter*     m_coverage     = nullptr;
    // M17–M20
    AutoPair*           m_autoPair     = nullptr;
    SmartIndent*        m_smartIndent  = nullptr;
    ImageViewerPanel*   m_imagePanel   = nullptr;
    RegexTesterPanel*   m_regexPanel   = nullptr;
    UnitConverterPanel* m_unitPanel    = nullptr;
    JsonPathPanel*      m_jsonPathPanel = nullptr;
    ColorPalettePanel*  m_colorPalette = nullptr;
    SysInfoPanel*       m_sysInfoPanel = nullptr;
    // M21–M26
    AiGhostCompletion*    m_aiGhost     = nullptr;
    LayoutPresets*        m_layouts     = nullptr;
    DevToolsPanel*        m_devTools    = nullptr;
    DbShellPanel*         m_dbShell     = nullptr;
    JsonLinesPanel*       m_jsonl       = nullptr;
    DocPreviewPanel*      m_docPreview  = nullptr;
    ClipboardHistoryPanel* m_clipboard  = nullptr;
    ArchiveBrowserPanel*  m_archive     = nullptr;
    TimerPanel*           m_timer       = nullptr;
    // M27–M35
    AutoCorrect*          m_autoCorrect = nullptr;
    GraphQlPanel*         m_graphql     = nullptr;
    CodeReviewPanel*      m_review      = nullptr;
    ProfileRunnerPanel*   m_profile     = nullptr;
    ImageAnnotPanel*      m_imgAnnot    = nullptr;
    CsvChartPanel*        m_csvChart    = nullptr;
    MdTablePanel*         m_mdTable     = nullptr;
    CronEditorPanel*      m_cron        = nullptr;
    FileWatcherPanel*     m_watcher     = nullptr;
    // M36–M50
    TextStatsPanel*       m_textStats   = nullptr;
    MindMapPanel*         m_mindMap     = nullptr;
    PastebinPanel*        m_pastebin    = nullptr;
    TimeTrackerPanel*     m_timeTracker = nullptr;
    CodeClonesPanel*      m_codeClones  = nullptr;
    CallGraphPanel*       m_callGraph   = nullptr;
    SecretScannerPanel*   m_secrets     = nullptr;
    ScreenshotPanel*      m_screenshot  = nullptr;
    RssReaderPanel*       m_rss         = nullptr;
    CliShellPanel*        m_cliShell    = nullptr;
    HexViewerPanel*       m_hexPanel    = nullptr;
    SqlSchemaPanel*       m_sqlSchema   = nullptr;
    BuildWatchPanel*      m_buildWatch  = nullptr;
    // M51–M60
    TodoAggregatorPanel*  m_todoAgg     = nullptr;
    GrepPanel*            m_grep        = nullptr;
    YamlValidatorPanel*   m_yamlVal     = nullptr;
    DockerPanel*          m_docker      = nullptr;
    JqRunnerPanel*        m_jq          = nullptr;
    HtmlPreviewPanel*     m_htmlPrev    = nullptr;
    GistPanel*            m_gist        = nullptr;
    EnvManagerPanel*      m_envMgr      = nullptr;
    SshExecPanel*         m_sshExec     = nullptr;
    CheatsheetPanel*      m_cheats      = nullptr;
    VaultPanel*           m_vault       = nullptr;
    // M61–M70 + SSHFS
    KubectlPanel*         m_kubectl     = nullptr;
    SystemMonitorPanel*   m_sysmon      = nullptr;
    GpgPanel*             m_gpg         = nullptr;
    SslCertViewerPanel*   m_ssl         = nullptr;
    AsciiDocPreviewPanel* m_adoc        = nullptr;
    PortScanPanel*        m_portScan    = nullptr;
    SystemdServicesPanel* m_systemd     = nullptr;
    TunnelManagerPanel*   m_tunnels     = nullptr;
    ApiBrowserPanel*      m_apiBrowser  = nullptr;
    LogTailPanel*         m_logTail     = nullptr;
    SshfsPanel*           m_sshfs       = nullptr;

    // Lifecycle helpers
    AutoSavePolicy*   m_autoSave;
    SessionManager*   m_session;
    AutoCompleter*    m_autoCompleter;
    BookmarkManager*  m_bookmarks;
    MacroRecorder*    m_macros;
    EolMenu*          m_eolMenu;
    WordCountDialog*  m_wordCountDialog;
    MacroDialog*      m_macroDialog;
    BookmarkDialog*   m_bookmarkDialog;

    // M5 helpers
    SpellChecker*        m_spellChecker;
    ExternalFileWatcher* m_externalWatcher;
    BraceMatcher*        m_braceMatcher;
    WhitespaceView*      m_whitespaceView;
    Snippets*            m_snippets;
    RecentProjects*      m_recentProjects;
    EditEnhancements*    m_editEnhance;
    HashDialog*          m_hashDialog;
    SnippetsDialog*      m_snippetsDialog;

    // M6 helpers
    TabExtras*           m_tabExtras;
    CodeFormatter*       m_codeFormatter;
    GitStatusService*    m_gitStatus;
    CrashRecovery*       m_crashRecovery;
    Workspace*           m_workspace;
    QLabel*              m_statusGit;
    QMenu*               m_menuRecentWorkspaces;

    // M7 helpers
    ColorMarkers*        m_colorMarkers;
    FoldingMenu*         m_foldingMenu;
    GitDiffGutter*       m_gitDiffGutter;
    LspClient*           m_lsp;
    LspFeatures*         m_lspFeatures;
    LspDiagnosticsPanel* m_lspDiagnosticsPanel;
    EncodingConvert*     m_encodingConvert;
    ViewModes*           m_viewModes;
    PythonPluginHost*    m_pluginHost;
    QMenu*               m_menuPlugins;
    KeybindingsManager*  m_keybindings;
    StickyScroll*        m_stickyScroll;
    QAction*             m_actSmartHighlight;
    QAction*             m_actGitDiffGutter;
    QAction*             m_actToggleLspDiagnostics;

    QLabel* m_statusPosition;
    QLabel* m_statusEncoding;
    QLabel* m_statusEol;
    QLabel* m_statusLanguage;   // M8: lexer / language indicator

    // File menu
    QAction* m_actNew;
    QAction* m_actOpen;
    QAction* m_actSave;
    QAction* m_actSaveAs;
    QAction* m_actClose;
    QAction* m_actExit;
    QMenu*   m_menuRecent;

    // Edit menu
    QAction* m_actUndo;
    QAction* m_actRedo;
    QAction* m_actCut;
    QAction* m_actCopy;
    QAction* m_actPaste;
    QAction* m_actSelectAll;

    // Search menu
    QAction* m_actFind;
    QAction* m_actReplace;
    QAction* m_actGoToLine;

    // View menu
    QAction* m_actThemeLight;
    QAction* m_actThemeDark;
    QAction* m_actThemeDracula;
    QActionGroup* m_themeGroup;
    QAction* m_actToggleLineNumbers;
    QAction* m_actToggleWordWrap;
    QAction* m_actSplitHorizontal;
    QAction* m_actSplitVertical;
    QAction* m_actUnsplit;
    QAction* m_actMoveTabToOtherGroup;

    // Tools menu
    QAction* m_actPreferences;
    QAction* m_actCompare;
    QAction* m_actCssPreview;
    QAction* m_actCsvView;
    QAction* m_actMarkdownPreview;
    QAction* m_actHexViewer;
    QAction* m_actFindInFiles;
    QAction* m_actCommandPalette;
    QAction* m_actRunCommand;
    QAction* m_actToggleFunctionList;
    QAction* m_actToggleDocumentMap;
    QAction* m_actToggleFileBrowser;
    QAction* m_actToggleExecOutput;
    QAction* m_actToggleTerminal;
    QAction* m_actToggleFtp;
    QAction* m_actToggleSftp;
    QAction* m_actToggleGitLog;

    // M12: utility / multi-window
    Mode     m_mode = Mode::Primary;

    // Help menu
    QAction* m_actAbout;
};

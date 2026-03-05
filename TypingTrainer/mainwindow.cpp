#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFont>
#include <QFileInfo>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_keyboard = new KeyboardWidget(this);
    QVBoxLayout *kbLayout = new QVBoxLayout(ui->keyboardPlaceholder);
    kbLayout->setContentsMargins(0, 0, 0, 0);
    kbLayout->addWidget(m_keyboard);

    m_textDisplay = new TextDisplayWidget(this);
    QVBoxLayout *tdLayout = new QVBoxLayout(ui->textDisplayPlaceholder);
    tdLayout->setContentsMargins(0, 0, 0, 0);
    tdLayout->addWidget(m_textDisplay);

    QFont lineFont("Courier New", 16, QFont::Bold);
    lineFont.setStyleHint(QFont::Monospace);

    m_lblPrevLine = new QLabel(this);
    m_lblPrevLine->setFont(lineFont);
    m_lblPrevLine->setStyleSheet("QLabel { color: #aaaaaa; padding: 2px 8px; }");
    m_lblPrevLine->setTextFormat(Qt::RichText);
    m_lblPrevLine->setWordWrap(false);

    m_lblCurrLine = new QLabel(this);
    m_lblCurrLine->setFont(lineFont);
    m_lblCurrLine->setStyleSheet("QLabel { background: #f5f5f5; padding: 4px 8px;"
                                 " border-radius: 3px; }");
    m_lblCurrLine->setTextFormat(Qt::RichText);
    m_lblCurrLine->setWordWrap(false);

    tdLayout->insertWidget(0, m_lblPrevLine);
    tdLayout->insertWidget(1, m_lblCurrLine);

    m_sessionTimer = new QTimer(this);
    m_sessionTimer->setInterval(500);
    connect(m_sessionTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    setupConnections();
    scanLessons();
    loadSettings();
    goToPage(0);
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::setupConnections()
{
    connect(ui->btnStartTraining,  &QPushButton::clicked, this, &MainWindow::onStartTraining);
    connect(ui->btnRestart,        &QPushButton::clicked, this, &MainWindow::onRestartTraining);
    connect(ui->btnReturnToMain,   &QPushButton::clicked, this, &MainWindow::onReturnToMain);
    connect(ui->btnRandom,         &QPushButton::clicked, this, &MainWindow::onRandomLesson);
    connect(ui->btnReload,         &QPushButton::clicked, this, &MainWindow::onReloadLessons);
    connect(ui->actionExit,        &QAction::triggered,   this, &MainWindow::onExit);
    connect(ui->actionAbout,       &QAction::triggered,   this, &MainWindow::onAbout);
    connect(ui->actionRandom,      &QAction::triggered,   this, &MainWindow::onRandomLesson);
    connect(ui->actionReload,      &QAction::triggered,   this, &MainWindow::onReloadLessons);
    connect(ui->actionToggleSpeed, &QAction::triggered,   this, &MainWindow::onToggleSpeedMode);
    connect(ui->comboLesson, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLessonChanged);
}

void MainWindow::goToPage(int index)
{
    ui->stackScreens->setCurrentIndex(index);
    if (index == 1) {
        this->setFocus();
    }
}

void MainWindow::scanLessons()
{
    QString previousPath;
    int previousIndex = ui->comboLesson->currentIndex();
    if (previousIndex >= 0 && previousIndex < m_lessons.size())
        previousPath = m_lessons[previousIndex].filePath;

    m_lessons = LessonLoader::scanLessons(LessonLoader::lessonsDir());

    QSignalBlocker blocker(ui->comboLesson);
    ui->comboLesson->clear();

    for (const LessonEntry &entry : m_lessons)
        ui->comboLesson->addItem(entry.title, entry.filePath);

    bool hasLessons = !m_lessons.isEmpty();
    ui->btnStartTraining->setEnabled(hasLessons);
    ui->btnRandom->setEnabled(hasLessons);

    if (!hasLessons) {
        ui->lblLessonDesc->setText("No lessons found in: " + LessonLoader::lessonsDir());
        statusBar()->showMessage("Loaded 0 lessons.");
        return;
    }

    int restoreIndex = 0;
    if (!previousPath.isEmpty()) {
        for (int i = 0; i < m_lessons.size(); ++i) {
            if (m_lessons[i].filePath == previousPath) { restoreIndex = i; break; }
        }
    }

    ui->comboLesson->setCurrentIndex(restoreIndex);
    loadLesson(m_lessons[restoreIndex].filePath);

    statusBar()->showMessage("Loaded " + QString::number(m_lessons.size()) +
                             " lessons from " + LessonLoader::lessonsDir());
}

void MainWindow::loadLesson(const QString &filePath)
{
    QString text = LessonLoader::loadLesson(filePath);
    if (text.isEmpty()) {
        QMessageBox::warning(this, "Load Error", "Could not read lesson file:\n" + filePath);
        return;
    }
    m_model.loadText(text);
    m_lastWasError = false;
    updateTrainingUI();
    QFileInfo fi(filePath);
    ui->lblLessonDesc->setText(fi.baseName() + "  —  " + QString::number(fi.size()) + " bytes");
}

static QString htmlSpan(const QString &text, const QString &style)
{
    if (text.isEmpty()) return QString();
    return "<span style='" + style + "'>" + text.toHtmlEscaped() + "</span>";
}

void MainWindow::updateCurrentLineLabel()
{
    QString prev = m_model.previousLine();
    m_lblPrevLine->setText(prev.isEmpty() ? QString() :
                           htmlSpan(prev, "color:#aaaaaa;"));

    QString line    = m_model.currentLine();
    int     ci      = m_model.charIndex();
    QString typed   = line.left(ci);
    QString curChar = (ci < line.size()) ? line.mid(ci, 1) : QString();
    QString rest    = (ci + 1 < line.size()) ? line.mid(ci + 1) : QString();

    QString html;
    html += htmlSpan(typed, "color:#1a7a1a; background:#d4f5d4;");
    if (!curChar.isEmpty()) {
        if (m_lastWasError)
            html += htmlSpan(curChar, "color:white; background:#e53935;");
        else
            html += htmlSpan(curChar, "color:#333; background:#ffe082;"
                             " outline: 2px solid #f9a825;");
    }
    html += htmlSpan(rest, "color:#333333;");

    m_lblCurrLine->setText(html);
}

void MainWindow::updateTrainingUI()
{
    updateCurrentLineLabel();

    int globalOffset = 0;
    const QStringList &lines = m_model.lines();
    for (int i = 0; i < m_model.lineIndex() && i < lines.size(); ++i)
        globalOffset += lines[i].size() + 1;
    globalOffset += m_model.charIndex();

    m_textDisplay->setText(m_model.fullText());
    m_textDisplay->setTypedCount(globalOffset);

    int totalChars = m_model.fullText().size();
    if (totalChars > 0)
        ui->progressSession->setValue(globalOffset * 100 / totalChars);

    highlightNextKey();
    updateStatsLabels();
}

void MainWindow::highlightNextKey()
{
    QChar expected = m_model.expectedChar();
    if (expected.isNull() || m_model.isFinished()) {
        m_keyboard->clearHighlight();
        return;
    }
    m_keyboard->highlightNextKey(QString(expected));
}

QString MainWindow::formatTime(qint64 ms) const
{
    qint64 totalSec = ms / 1000;
    int minutes = static_cast<int>(totalSec / 60);
    int seconds = static_cast<int>(totalSec % 60);
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

double MainWindow::calcSpeed(qint64 ms) const
{
    if (ms < 1000) return 0.0;
    double minutes = ms / 60000.0;
    double cpm = m_correctKeystrokes / minutes;
    if (m_speedModeWPM)
        return cpm / 5.0;
    return cpm;
}

double MainWindow::calcAccuracy() const
{
    if (m_totalKeystrokes == 0) return 100.0;
    return 100.0 * m_correctKeystrokes / m_totalKeystrokes;
}

QString MainWindow::speedLabel(double speed) const
{
    int rounded = static_cast<int>(speed + 0.5);
    return QString::number(rounded) + (m_speedModeWPM ? " WPM" : " CPM");
}

void MainWindow::updateStatsLabels()
{
    qint64 ms = m_elapsedMs;
    if (m_sessionTimer->isActive() && m_elapsed.isValid())
        ms = m_elapsed.elapsed();

    ui->lblSessionTime->setText("⏱ Time: " + formatTime(ms));

    double speed = calcSpeed(ms);
    ui->lblSessionSpeed->setText("⌨ Speed: " + speedLabel(speed));

    double acc = calcAccuracy();
    ui->lblSessionAccuracy->setText(QString("✓ Accuracy: %1%").arg(qRound(acc)));
}

void MainWindow::resetSessionMetrics()
{
    m_totalKeystrokes    = 0;
    m_correctKeystrokes  = 0;
    m_elapsedMs          = 0;
    m_lastWasError       = false;
    ui->progressSession->setValue(0);
    updateStatsLabels();
}

void MainWindow::finishSession()
{
    m_sessionTimer->stop();
    m_elapsedMs = m_elapsed.elapsed();

    double speed = calcSpeed(m_elapsedMs);
    double acc   = calcAccuracy();

    ui->lblResultTime->setText(formatTime(m_elapsedMs));
    ui->lblResultSpeed->setText(speedLabel(speed));
    ui->lblResultAccuracy->setText(QString("%1%").arg(qRound(acc)));

    goToPage(2);
    statusBar()->showMessage("Session complete!");
}

void MainWindow::onTimerTick()
{
    updateStatsLabels();
}

void MainWindow::onLessonChanged(int index)
{
    if (index < 0 || index >= m_lessons.size()) return;
    loadLesson(m_lessons[index].filePath);
    saveSettings();
}

void MainWindow::onRandomLesson()
{
    if (m_lessons.isEmpty()) return;
    int i = QRandomGenerator::global()->bounded(m_lessons.size());
    ui->comboLesson->setCurrentIndex(i);
}

void MainWindow::onReloadLessons()
{
    scanLessons();
}

void MainWindow::onToggleSpeedMode()
{
    m_speedModeWPM = !m_speedModeWPM;
    ui->actionToggleSpeed->setText(m_speedModeWPM ? "Switch to CPM" : "Switch to WPM");
    updateStatsLabels();
    saveSettings();
}

void MainWindow::onStartTraining()
{
    m_model.reset();
    resetSessionMetrics();
    goToPage(1);
    updateTrainingUI();
    m_elapsed.start();
    m_sessionTimer->start();
}

void MainWindow::onRestartTraining()
{
    m_sessionTimer->stop();
    m_model.reset();
    resetSessionMetrics();
    goToPage(1);
    updateTrainingUI();
    m_elapsed.start();
    m_sessionTimer->start();
}

void MainWindow::onReturnToMain()
{
    m_sessionTimer->stop();
    m_elapsedMs = 0;
    m_keyboard->clearHighlight();
    goToPage(0);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About TypingTrainer",
                       "TypingTrainer v4.0\n\n");
}

void MainWindow::onExit() { close(); }

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->stackScreens->currentIndex() != 1) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    int key = event->key();

    if (key == Qt::Key_Backspace) {
        m_keyboard->pressKey("BACKSPACE");
        m_model.goBack();
        m_lastWasError = false;
        updateTrainingUI();
        return;
    }

    if (key == Qt::Key_Shift || key == Qt::Key_Control ||
        key == Qt::Key_Alt   || key == Qt::Key_Meta    ||
        key == Qt::Key_CapsLock || (key >= Qt::Key_F1 && key <= Qt::Key_F35)) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    QString text = event->text();
    if (text.isEmpty()) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    if (m_model.isFinished()) return;

    m_keyboard->pressKey(text);

    QChar expected = m_model.expectedChar();
    bool isEnterNeeded = expected.isNull() && !m_model.isFinished();

    if (isEnterNeeded) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            m_model.advance();
            m_lastWasError = false;
            updateTrainingUI();
        }
        return;
    }

    m_totalKeystrokes++;
    if (text[0] == expected) {
        m_correctKeystrokes++;
        m_lastWasError = false;
        m_model.advance();
    } else {
        m_lastWasError = true;
    }

    updateTrainingUI();

    if (m_model.isFinished()) {
        m_keyboard->clearHighlight();
        finishSession();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (ui->stackScreens->currentIndex() == 1) {
        QString text = event->text();
        int key = event->key();

        if (key == Qt::Key_Backspace)      m_keyboard->releaseKey("BACKSPACE");
        else if (key == Qt::Key_Return ||
                 key == Qt::Key_Enter)     m_keyboard->releaseKey("ENTER");
        else if (key == Qt::Key_Space)     m_keyboard->releaseKey("SPACE");
        else if (!text.isEmpty())          m_keyboard->releaseKey(text);
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings("TypingTrainer", "TypingTrainer");

    QString mode = settings.value("ui/speedMode", "CPM").toString();
    m_speedModeWPM = (mode == "WPM");
    ui->actionToggleSpeed->setText(m_speedModeWPM ? "Switch to CPM" : "Switch to WPM");

    QString lastLesson = settings.value("ui/lastLesson", "").toString();
    if (!lastLesson.isEmpty() && !m_lessons.isEmpty()) {
        for (int i = 0; i < m_lessons.size(); ++i) {
            if (m_lessons[i].filePath == lastLesson ||
                m_lessons[i].title == lastLesson) {
                QSignalBlocker blocker(ui->comboLesson);
                ui->comboLesson->setCurrentIndex(i);
                loadLesson(m_lessons[i].filePath);
                break;
            }
        }
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("TypingTrainer", "TypingTrainer");

    settings.setValue("ui/speedMode", m_speedModeWPM ? "WPM" : "CPM");

    int idx = ui->comboLesson->currentIndex();
    if (idx >= 0 && idx < m_lessons.size())
        settings.setValue("ui/lastLesson", m_lessons[idx].filePath);
}

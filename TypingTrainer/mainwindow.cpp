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

    setupConnections();
    scanLessons();
    goToPage(0);
}

MainWindow::~MainWindow()
{
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

    QString line     = m_model.currentLine();
    int     ci       = m_model.charIndex();
    QString typed    = line.left(ci);
    QString curChar  = (ci < line.size()) ? line.mid(ci, 1) : QString();
    QString rest     = (ci + 1 < line.size()) ? line.mid(ci + 1) : QString();

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

    highlightNextKey();
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

void MainWindow::onLessonChanged(int index)
{
    if (index < 0 || index >= m_lessons.size()) return;
    loadLesson(m_lessons[index].filePath);
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

void MainWindow::onStartTraining()
{
    m_model.reset();
    m_lastWasError = false;
    goToPage(1);
    updateTrainingUI();
}

void MainWindow::onRestartTraining()
{
    m_model.reset();
    m_lastWasError = false;
    goToPage(1);
    updateTrainingUI();
}

void MainWindow::onReturnToMain()
{
    m_keyboard->clearHighlight();
    goToPage(0);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About TypingTrainer",
                       "TypingTrainer v4.0\n\n"
                       "Practical Work #9 — Keyboard Events & Highlighting\n\n"
                       "Green  = correct typed characters\n"
                       "Yellow = current character to type\n"
                       "Red    = wrong key pressed\n"
                       "Keyboard hint = green key to press next");
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

    if (text[0] == expected) {
        m_lastWasError = false;
        m_model.advance();
    } else {
        m_lastWasError = true;
    }

    updateTrainingUI();

    if (m_model.isFinished()) {
        m_keyboard->clearHighlight();
        statusBar()->showMessage("Lesson finished! Press Restart to go again.");
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

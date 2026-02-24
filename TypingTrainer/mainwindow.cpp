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
    m_lblPrevLine->setTextFormat(Qt::PlainText);
    m_lblPrevLine->setWordWrap(false);

    m_lblCurrLine = new QLabel(this);
    m_lblCurrLine->setFont(lineFont);
    m_lblCurrLine->setStyleSheet("QLabel { color: #222222; background: #fffde7;"
                                 " padding: 2px 8px; border-radius: 3px; }");
    m_lblCurrLine->setTextFormat(Qt::PlainText);
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

    QAction *actStep = new QAction("Step Forward (Test)", this);
    actStep->setShortcut(QKeySequence("Ctrl+Right"));
    ui->menuSettings->addAction(actStep);
    connect(actStep, &QAction::triggered, this, &MainWindow::onStepForward);
}

void MainWindow::goToPage(int index)
{
    ui->stackScreens->setCurrentIndex(index);
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
        statusBar()->showMessage("Loaded 0 lessons from " + LessonLoader::lessonsDir());
        return;
    }

    int restoreIndex = 0;
    if (!previousPath.isEmpty()) {
        for (int i = 0; i < m_lessons.size(); ++i) {
            if (m_lessons[i].filePath == previousPath) {
                restoreIndex = i;
                break;
            }
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
        QMessageBox::warning(this, "Load Error",
                             "Could not read lesson file:\n" + filePath);
        return;
    }

    m_model.loadText(text);
    updateTrainingUI();

    QFileInfo fi(filePath);
    ui->lblLessonDesc->setText(fi.baseName() + "  —  " +
                               QString::number(fi.size()) + " bytes");
}

void MainWindow::updateTrainingUI()
{
    m_lblPrevLine->setText(m_model.previousLine());
    m_lblCurrLine->setText(m_model.currentLine());

    int globalOffset = 0;
    const QStringList &lines = m_model.lines();
    for (int i = 0; i < m_model.lineIndex() && i < lines.size(); ++i)
        globalOffset += lines[i].size() + 1;
    globalOffset += m_model.charIndex();

    m_textDisplay->setText(m_model.fullText());
    m_textDisplay->setTypedCount(globalOffset);

    statusBar()->showMessage(
        QString("Line %1/%2   Char %3/%4   Typed: \"%5\"   Next: \"%6\"")
            .arg(m_model.lineIndex() + 1)
            .arg(m_model.lineCount())
            .arg(m_model.charIndex())
            .arg(m_model.currentLine().size())
            .arg(m_model.typedPart())
            .arg(m_model.remainingPart().left(20))
    );
}

void MainWindow::onLessonChanged(int index)
{
    if (index < 0 || index >= m_lessons.size())
        return;
    loadLesson(m_lessons[index].filePath);
}

void MainWindow::onRandomLesson()
{
    if (m_lessons.isEmpty())
        return;
    int i = QRandomGenerator::global()->bounded(m_lessons.size());
    ui->comboLesson->setCurrentIndex(i);
}

void MainWindow::onReloadLessons()
{
    scanLessons();
}

void MainWindow::onStepForward()
{
    if (ui->stackScreens->currentIndex() != 1)
        return;
    m_model.advance();
    updateTrainingUI();
    if (m_model.isFinished())
        statusBar()->showMessage("Text finished.");
}

void MainWindow::onStartTraining()
{
    goToPage(1);
    updateTrainingUI();
}

void MainWindow::onRestartTraining()
{
    m_model.reset();
    goToPage(1);
    updateTrainingUI();
}

void MainWindow::onReturnToMain()
{
    goToPage(0);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About TypingTrainer",
                       "TypingTrainer v2.0\n\n");
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->stackScreens->currentIndex() == 1) {
        QString key = event->text();
        if (event->key() == Qt::Key_Backspace)       m_keyboard->pressKey("Backspace");
        else if (event->key() == Qt::Key_Return ||
                 event->key() == Qt::Key_Enter)       m_keyboard->pressKey("Enter");
        else if (event->key() == Qt::Key_Shift)       m_keyboard->pressKey("Shift");
        else if (event->key() == Qt::Key_Space)       m_keyboard->pressKey("Space");
        else if (!key.isEmpty())                       m_keyboard->pressKey(key);
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (ui->stackScreens->currentIndex() == 1) {
        QString key = event->text();
        if (event->key() == Qt::Key_Backspace)       m_keyboard->releaseKey("Backspace");
        else if (event->key() == Qt::Key_Return ||
                 event->key() == Qt::Key_Enter)       m_keyboard->releaseKey("Enter");
        else if (event->key() == Qt::Key_Shift)       m_keyboard->releaseKey("Shift");
        else if (event->key() == Qt::Key_Space)       m_keyboard->releaseKey("Space");
        else if (!key.isEmpty())                       m_keyboard->releaseKey(key);
    }
    QMainWindow::keyReleaseEvent(event);
}

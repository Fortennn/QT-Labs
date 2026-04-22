#include "LlamaWorkerThread.h"
#include <QDebug>
#include <llama.h>
#include <vector>
#include <string>

LlamaWorkerThread::LlamaWorkerThread(QObject *parent)
    : QThread(parent)
{
    llama_backend_init();
}

LlamaWorkerThread::~LlamaWorkerThread() {
    m_mutex.lock();
    m_quitThread = true;
    m_cond.wakeOne();
    m_mutex.unlock();
    
    wait();

    if (m_ctx) llama_free(m_ctx);
    if (m_model) llama_free_model(m_model);
    llama_backend_free();
}

bool LlamaWorkerThread::loadModel(const QString& modelPath) {
    if (m_model) {
        emit errorOccurred("Модель вже завантажена.");
        return false;
    }

    auto mparams = llama_model_default_params();
    mparams.n_gpu_layers = -1; 

    m_model = llama_load_model_from_file(modelPath.toUtf8().constData(), mparams);
    if (!m_model) {
        emit errorOccurred("Помилка завантаження моделі: " + modelPath);
        return false;
    }

    auto cparams = llama_context_default_params();
    cparams.n_ctx = 4096;
    cparams.n_batch = 512;

    m_ctx = llama_new_context_with_model(m_model, cparams);
    if (!m_ctx) {
        emit errorOccurred("Помилка створення контексту моделі.");
        return false;
    }

    emit modelLoaded(true);
    return true;
}

void LlamaWorkerThread::queuePrompt(const QString& systemPrompt, const QString& userPrompt) {
    QMutexLocker locker(&m_mutex);
    m_requests.enqueue({systemPrompt, userPrompt});
    m_stopRequested = false; 
    m_cond.wakeOne();
}

void LlamaWorkerThread::stopGeneration() {
    QMutexLocker locker(&m_mutex);
    m_stopRequested = true;
}

void LlamaWorkerThread::run() {
    while (true) {
        Request req;
        {
            QMutexLocker locker(&m_mutex);
            while (m_requests.isEmpty() && !m_quitThread) {
                m_cond.wait(&m_mutex);
            }
            if (m_quitThread) break;
            
            req = m_requests.dequeue();
        }

        processGeneration(req);
    }
}

void LlamaWorkerThread::processGeneration(const Request& req) {
    if (!m_model || !m_ctx) {
        emit errorOccurred("Модель не завантажена. Генерація неможлива.");
        return;
    }

    QString response = "";
    QString mockWord = "Це імпровізований стрім даних.\n> System: " + req.systemPrompt + "\n> User: " + req.userPrompt;
    
    for(QChar c : mockWord) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopRequested) break;
        }
        
        response += c;
        emit tokenGenerated(QString(c));
        QThread::msleep(15);
    }
    
    emit replyFinished(response);
}

#include "LlamaWorkerThread.h"
#include <QDebug>
#include <QFile>
#include <QThread>
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
    if (m_model) llama_model_free(m_model);
    llama_backend_free();
}

bool LlamaWorkerThread::loadModel(const QString& modelPath) {
    qDebug() << "[JARVIS CORE] ----------------------------------------";
    qDebug() << "[JARVIS CORE] ПОЧИНАЄМО СПРОБУ ЗАВАНТАЖЕННЯ МОДЕЛІ";
    qDebug() << "[JARVIS CORE] Шлях до файлу:" << modelPath;
    
    QFile modelFile(modelPath);
    if (!modelFile.exists()) {
        qDebug() << "[JARVIS ERROR] Файл фізично НЕ ІСНУЄ за цим шляхом!";
        emit errorOccurred("Файл моделі не знайдено за шляхом: " + modelPath);
        return false;
    }
    qDebug() << "[JARVIS CORE] Файл існує. Розмір:" << modelFile.size() / (1024 * 1024) << "MB";

    if (m_ctx) {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }

    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
    }

    auto mparams = llama_model_default_params();
    mparams.n_gpu_layers = 0; // Строго CPU
    // use_mmap = true (за замовчуванням) - необхідно для завантаження 4.5 GB без виснаження РАМ

    // b4827+: оновлені назви функцій
    m_model = llama_model_load_from_file(modelPath.toUtf8().constData(), mparams);
    if (!m_model) {
        qDebug() << "[JARVIS ERROR] llama_model_load_from_file повернув NULL!";
        emit errorOccurred("Помилка llama.cpp під час парсингу файлу.");
        return false;
    }
    
    qDebug() << "[JARVIS CORE] Модель успішно прочитана. Створюємо контекст...";

    int contextSize;
    {
        QMutexLocker locker(&m_mutex);
        contextSize = m_contextSize;
    }

    auto cparams = llama_context_default_params();
    cparams.n_ctx = static_cast<uint32_t>(contextSize);
    cparams.n_batch = static_cast<uint32_t>(contextSize); // Повинно бути >= максимальній довжині промпту
    // Використовуємо всі доступні ядра CPU
    int cpuThreads = QThread::idealThreadCount();
    qDebug() << "[JARVIS CORE] Використовую потоків:" << cpuThreads;
    cparams.n_threads = cpuThreads;
    cparams.n_threads_batch = cpuThreads;

    // b4827+: llama_init_from_model замість llama_new_context_with_model
    m_ctx = llama_init_from_model(m_model, cparams);
    if (!m_ctx) {
        qDebug() << "[JARVIS ERROR] llama_init_from_model повернув NULL! Замало ОЗУ?";
        emit errorOccurred("Помилка створення контексту. Можливо не вистачає ОЗУ.");
        return false;
    }

    qDebug() << "[JARVIS CORE] Контекст виділено успішно. Готово до роботи!";
    emit modelLoaded(true);
    return true;
}

void LlamaWorkerThread::setParams(float temperature, int contextSize) {
    QMutexLocker locker(&m_mutex);
    m_temperature = temperature;
    m_contextSize = contextSize;
}

void LlamaWorkerThread::queueLoadModel(const QString& modelPath) {
    QMutexLocker locker(&m_mutex);
    m_modelPathToLoad = modelPath;
    m_cond.wakeOne();
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

void LlamaWorkerThread::clearHistory() {
    QMutexLocker locker(&m_mutex);
    m_history.clear();
    qDebug() << "[JARVIS CORE] Пам'ять очищена.";
}

void LlamaWorkerThread::run() {
    while (true) {
        Request req;
        QString loadPath;
        {
            QMutexLocker locker(&m_mutex);
            while (m_requests.isEmpty() && m_modelPathToLoad.isEmpty() && !m_quitThread) {
                m_cond.wait(&m_mutex);
            }
            if (m_quitThread) break;
            
            if (!m_modelPathToLoad.isEmpty()) {
                loadPath = m_modelPathToLoad;
                m_modelPathToLoad.clear();
            } else if (!m_requests.isEmpty()) {
                req = m_requests.dequeue();
            }
        }

        if (!loadPath.isEmpty()) {
            loadModel(loadPath);
        } else if (!req.userPrompt.isEmpty()) {
            processGeneration(req);
        }
    }
}

void LlamaWorkerThread::processGeneration(const Request& req) {
    if (!m_model || !m_ctx) {
        emit errorOccurred("Генерація зірвалась: Модель не завантажена. Покладіть gguf файл!");
        return;
    }

    QString promptStr = QString("<|im_start|>system\n%1<|im_end|>\n").arg(req.systemPrompt);

    int historyStart = qMax(0, m_history.size() - 10);
    for (int i = historyStart; i < m_history.size(); ++i) {
        promptStr += QString("<|im_start|>user\n%1<|im_end|>\n").arg(m_history[i].user);
        promptStr += QString("<|im_start|>assistant\n%1<|im_end|>\n").arg(m_history[i].assistant);
    }

    promptStr += QString("<|im_start|>user\n%1<|im_end|>\n").arg(req.userPrompt);
    promptStr += "<|im_start|>assistant\n";

    const std::string prompt = promptStr.toStdString();
    const llama_vocab* vocab = llama_model_get_vocab(m_model);
    if (!vocab) {
        emit errorOccurred("Помилка: llama_model_get_vocab повернув NULL!");
        return;
    }

    std::vector<llama_token> tokens(prompt.length() + 8);
    int nPromptTokens = llama_tokenize(vocab, prompt.c_str(), static_cast<int32_t>(prompt.size()), tokens.data(), static_cast<int32_t>(tokens.size()), true, true);
    if (nPromptTokens < 0) {
        tokens.resize(static_cast<size_t>(-nPromptTokens));
        nPromptTokens = llama_tokenize(vocab, prompt.c_str(), static_cast<int32_t>(prompt.size()), tokens.data(), static_cast<int32_t>(tokens.size()), true, true);
    }

    if (nPromptTokens <= 0) {
        emit errorOccurred("Помилка токенізації промпту.");
        return;
    }

    tokens.resize(static_cast<size_t>(nPromptTokens));
    llama_kv_cache_clear(m_ctx);

    llama_batch batch = llama_batch_get_one(tokens.data(), nPromptTokens);
    if (llama_decode(m_ctx, batch) != 0) {
        emit errorOccurred("Llama.cpp Error: Не вдалося декодувати стартовий промпт.");
        return;
    }

    float generationTemperature;
    {
        QMutexLocker locker(&m_mutex);
        generationTemperature = m_temperature;
    }

    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(generationTemperature));
    llama_sampler_chain_add(sampler, llama_sampler_init_min_p(0.1f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    QString fullResponse;
    bool insideTag = false;
    constexpr int kMaxGeneratedTokens = 1024;

    for (int i = 0; i < kMaxGeneratedTokens; ++i) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopRequested) {
                break;
            }
        }

        llama_token newToken = llama_sampler_sample(sampler, m_ctx, -1);
        llama_sampler_accept(sampler, newToken);

        if (llama_vocab_is_eog(vocab, newToken)) {
            break;
        }

        char buf[256];
        const int len = llama_token_to_piece(vocab, newToken, buf, sizeof(buf), 0, false);
        if (len > 0) {
            const QString piece = QString::fromUtf8(buf, len);
            fullResponse += piece;

            QString visiblePiece;
            visiblePiece.reserve(piece.size());
            for (const QChar ch : piece) {
                if (insideTag) {
                    if (ch == ']') {
                        insideTag = false;
                    }
                    continue;
                }

                if (ch == '[') {
                    insideTag = true;
                    continue;
                }

                visiblePiece.append(ch);
            }

            if (!visiblePiece.isEmpty()) {
                emit tokenGenerated(visiblePiece);
            }
        }

        batch = llama_batch_get_one(&newToken, 1);
        if (llama_decode(m_ctx, batch) != 0) {
            emit errorOccurred("Помилка під час декодування наступного токена.");
            break;
        }

    }

    llama_sampler_free(sampler);

    {
        QMutexLocker locker(&m_mutex);
        m_history.append({req.userPrompt, fullResponse.trimmed()});
    }

    emit replyFinished(fullResponse);
}

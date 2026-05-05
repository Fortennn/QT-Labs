#include "LlamaWorkerThread.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <llama.h>
#include <vector>
#include <string>

LlamaWorkerThread::PromptTemplate
LlamaWorkerThread::detectTemplate(const QString& modelPath) {
    const QString name = QFileInfo(modelPath).fileName().toLower();
    // Order matters: more specific tokens before generic ones.
    if (name.contains(QStringLiteral("llama-3"))
        || name.contains(QStringLiteral("llama3"))
        || name.contains(QStringLiteral("meta-llama"))) {
        return PromptTemplate::Llama3;
    }
    if (name.contains(QStringLiteral("mistral"))
        || name.contains(QStringLiteral("mixtral"))) {
        return PromptTemplate::Mistral;
    }
    if (name.contains(QStringLiteral("gemma"))) {
        return PromptTemplate::Gemma;
    }
    // dolphin / qwen / openchat / yi / nous / hermes / phi-3-medium-instruct
    // and most fine-tunes default to ChatML.
    return PromptTemplate::ChatML;
}

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

    int gpuLayers;
    {
        QMutexLocker locker(&m_mutex);
        gpuLayers = m_gen.gpuLayers;
    }
    auto mparams = llama_model_default_params();
    mparams.n_gpu_layers = gpuLayers;          // 0=CPU, 99=offload all
    // use_mmap = true (за замовчуванням) - необхідно для завантаження 4.5 GB без виснаження РАМ
    qDebug() << "[JARVIS CORE] GPU layers requested:" << gpuLayers;

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
        contextSize = m_gen.contextSize;
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

    {
        QMutexLocker locker(&m_mutex);
        m_loadedModelPath = modelPath;
    }

    qDebug() << "[JARVIS CORE] Контекст виділено успішно. Готово до роботи!";
    emit modelLoaded(true);
    return true;
}

void LlamaWorkerThread::setParams(float temperature, int contextSize) {
    QMutexLocker locker(&m_mutex);
    m_gen.temperature = temperature;
    if (contextSize >= 512) m_gen.contextSize = contextSize;
}

void LlamaWorkerThread::setGenParams(const GenParams& params) {
    QMutexLocker locker(&m_mutex);
    m_gen = params;
    if (m_gen.contextSize < 512)  m_gen.contextSize  = 512;
    if (m_gen.maxTokens   < 32)   m_gen.maxTokens    = 32;
    if (m_gen.topK        < 1)    m_gen.topK         = 1;
    if (m_gen.topP        <= 0.f) m_gen.topP         = 0.95f;
    if (m_gen.minP        < 0.f)  m_gen.minP         = 0.f;
    if (m_gen.repeatPenalty <= 0.f) m_gen.repeatPenalty = 1.f;
    if (m_gen.temperature  < 0.f) m_gen.temperature  = 0.f;
    if (m_gen.gpuLayers    < 0)   m_gen.gpuLayers    = 0;
}

LlamaWorkerThread::GenParams LlamaWorkerThread::genParams() const {
    QMutexLocker locker(&m_mutex);
    return m_gen;
}

void LlamaWorkerThread::setSystemPromptOverride(const QString& override) {
    QMutexLocker locker(&m_mutex);
    m_systemPromptOverride = override;
}

QString LlamaWorkerThread::systemPromptOverride() const {
    QMutexLocker locker(&m_mutex);
    return m_systemPromptOverride;
}

void LlamaWorkerThread::queueLoadModel(const QString& modelPath) {
    QMutexLocker locker(&m_mutex);
    m_modelPathToLoad = modelPath;
    m_cond.wakeOne();
}

void LlamaWorkerThread::queuePrompt(const QString& systemPrompt, const QString& userPrompt) {
    QMutexLocker locker(&m_mutex);
    m_requests.enqueue({systemPrompt, userPrompt});
    m_busy = true;
    m_stopRequested = false; 
    m_cond.wakeOne();
}

bool LlamaWorkerThread::isBusy() const {
    QMutexLocker locker(&m_mutex);
    return m_busy || !m_requests.isEmpty();
}

QString LlamaWorkerThread::loadedModelName() const {
    QMutexLocker locker(&m_mutex);
    if (m_loadedModelPath.isEmpty()) return {};
    return QFileInfo(m_loadedModelPath).fileName();
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
        // Drop the busy flag once the queue is fully drained so isBusy()
        // returns false for the LAN web server's concurrency gate.
        {
            QMutexLocker locker(&m_mutex);
            if (m_requests.isEmpty()) m_busy = false;
        }
    }
}

void LlamaWorkerThread::processGeneration(const Request& req) {
    if (!m_model || !m_ctx) {
        emit errorOccurred("Генерація зірвалась: Модель не завантажена. Покладіть gguf файл!");
        return;
    }

    QString systemPrompt;
    GenParams gen;
    QString loadedPath;
    {
        QMutexLocker locker(&m_mutex);
        gen = m_gen;
        systemPrompt = m_systemPromptOverride.isEmpty()
                           ? req.systemPrompt
                           : m_systemPromptOverride;
        loadedPath = m_loadedModelPath;
    }

    PromptTemplate tmpl = gen.promptTemplate;
    if (tmpl == PromptTemplate::Auto) tmpl = detectTemplate(loadedPath);

    int historyStart = qMax(0, m_history.size() - 10);
    QString promptStr;

    switch (tmpl) {
    case PromptTemplate::Llama3: {
        promptStr  = QStringLiteral("<|begin_of_text|>");
        promptStr += QString("<|start_header_id|>system<|end_header_id|>\n\n%1<|eot_id|>")
                         .arg(systemPrompt);
        for (int i = historyStart; i < m_history.size(); ++i) {
            promptStr += QString("<|start_header_id|>user<|end_header_id|>\n\n%1<|eot_id|>")
                             .arg(m_history[i].user);
            promptStr += QString("<|start_header_id|>assistant<|end_header_id|>\n\n%1<|eot_id|>")
                             .arg(m_history[i].assistant);
        }
        promptStr += QString("<|start_header_id|>user<|end_header_id|>\n\n%1<|eot_id|>")
                         .arg(req.userPrompt);
        promptStr += QStringLiteral("<|start_header_id|>assistant<|end_header_id|>\n\n");
        break;
    }
    case PromptTemplate::Mistral: {
        // Mistral chat template: [INST] <<SYS>>...<</SYS>> userMsg [/INST] reply </s>[INST] ... [/INST]
        bool firstTurn = true;
        for (int i = historyStart; i < m_history.size(); ++i) {
            if (firstTurn) {
                promptStr += QString("[INST] <<SYS>>\n%1\n<</SYS>>\n\n%2 [/INST] ")
                                 .arg(systemPrompt, m_history[i].user);
                firstTurn = false;
            } else {
                promptStr += QString("[INST] %1 [/INST] ").arg(m_history[i].user);
            }
            promptStr += m_history[i].assistant + QStringLiteral("</s>");
        }
        if (firstTurn) {
            promptStr += QString("[INST] <<SYS>>\n%1\n<</SYS>>\n\n%2 [/INST] ")
                             .arg(systemPrompt, req.userPrompt);
        } else {
            promptStr += QString("[INST] %1 [/INST] ").arg(req.userPrompt);
        }
        break;
    }
    case PromptTemplate::Gemma: {
        // Gemma has no real "system" role — fold it into the first user turn.
        bool injectedSystem = false;
        for (int i = historyStart; i < m_history.size(); ++i) {
            QString u = m_history[i].user;
            if (!injectedSystem) {
                u = systemPrompt + "\n\n" + u;
                injectedSystem = true;
            }
            promptStr += QString("<start_of_turn>user\n%1<end_of_turn>\n").arg(u);
            promptStr += QString("<start_of_turn>model\n%1<end_of_turn>\n").arg(m_history[i].assistant);
        }
        QString u = req.userPrompt;
        if (!injectedSystem) u = systemPrompt + "\n\n" + u;
        promptStr += QString("<start_of_turn>user\n%1<end_of_turn>\n").arg(u);
        promptStr += QStringLiteral("<start_of_turn>model\n");
        break;
    }
    case PromptTemplate::ChatML:
    case PromptTemplate::Auto:
    default: {
        promptStr = QString("<|im_start|>system\n%1<|im_end|>\n").arg(systemPrompt);
        for (int i = historyStart; i < m_history.size(); ++i) {
            promptStr += QString("<|im_start|>user\n%1<|im_end|>\n").arg(m_history[i].user);
            promptStr += QString("<|im_start|>assistant\n%1<|im_end|>\n").arg(m_history[i].assistant);
        }
        promptStr += QString("<|im_start|>user\n%1<|im_end|>\n").arg(req.userPrompt);
        promptStr += QStringLiteral("<|im_start|>assistant\n");
        break;
    }
    }

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

    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    // Repeat-penalty applied first so subsequent samplers narrow the field.
    llama_sampler_chain_add(sampler,
        llama_sampler_init_penalties(/*last_n=*/64,
                                     /*repeat=*/  gen.repeatPenalty,
                                     /*freq=*/    0.0f,
                                     /*present=*/ 0.0f));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(gen.topK));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(gen.topP, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_min_p(gen.minP, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(gen.temperature));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    QString fullResponse;
    bool insideTag = false;
    const int maxGeneratedTokens = qMax(32, gen.maxTokens);

    for (int i = 0; i < maxGeneratedTokens; ++i) {
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

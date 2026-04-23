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

    if (m_model) {
        qDebug() << "[JARVIS ERROR] Модель вже завантажена!";
        emit errorOccurred("Модель вже завантажена.");
        return false;
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

    auto cparams = llama_context_default_params();
    cparams.n_ctx = 2048;
    cparams.n_batch = 512;
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

    // Форматуємо промпт за стандартом ChatML (ідеально для Llama-3 і Dolphin)
    QString formattedPrompt = QString(
        "<|im_start|>system\n%1<|im_end|>\n"
        "<|im_start|>user\n%2<|im_end|>\n"
        "<|im_start|>assistant\n")
        .arg(req.systemPrompt)
        .arg(req.userPrompt);

    std::string prompt = formattedPrompt.toStdString();
    qDebug() << "[GEN] \u0421\u0422\u0410\u0420\u0422. \u0414\u043e\u0432\u0436\u0438\u043d\u0430 \u043f\u0440\u043e\u043c\u043f\u0442\u0443:" << prompt.length() << "\u0441\u0438\u043c\u0432\u043e\u043b\u0456\u0432";

    // b4827+: \u0442\u043e\u043a\u0435\u043d\u0456\u0437\u0430\u0446\u0456\u044f \u0447\u0435\u0440\u0435\u0437 llama_vocab
    const llama_vocab * vocab = llama_model_get_vocab(m_model);
    if (!vocab) {
        emit errorOccurred("\u041f\u043e\u043c\u0438\u043b\u043a\u0430: llama_model_get_vocab \u043f\u043e\u0432\u0435\u0440\u043d\u0443\u0432 NULL!");
        return;
    }
    qDebug() << "[GEN] Vocab \u043e\u0442\u0440\u0438\u043c\u0430\u043d\u043e.";

    std::vector<llama_token> tokens_list(prompt.length() + 8);
    int n_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.length(), tokens_list.data(), tokens_list.size(), true, true);
    if (n_tokens < 0) {
        tokens_list.resize(-n_tokens);
        n_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.length(), tokens_list.data(), tokens_list.size(), true, true);
    }
    tokens_list.resize(n_tokens);
    qDebug() << "[GEN] \u0422\u043e\u043a\u0435\u043d\u0456\u0437\u0430\u0446\u0456\u044f: " << n_tokens << "\u0442\u043e\u043a\u0435\u043d\u0456\u0432";

    llama_kv_cache_clear(m_ctx);
    qDebug() << "[GEN] KV \u043a\u0435\u0448 \u043e\u0447\u0438\u0449\u0435\u043d\u043e.";

    QString fullResponse = "";
    
    llama_batch batch = llama_batch_get_one(tokens_list.data(), n_tokens);
    qDebug() << "[GEN] batch \u0441\u0442\u0432\u043e\u0440\u0435\u043d\u043e, \u0432\u0438\u043a\u043b\u0438\u043a\u0430\u0454\u043c\u043e llama_decode...";
    
    if (llama_decode(m_ctx, batch)) {
        emit errorOccurred("Llama.cpp Error: \u041d\u0435 \u0432\u0434\u0430\u043b\u043e\u0441\u044f \u0434\u0435\u043a\u043e\u0434\u0443\u0432\u0430\u0442\u0438 \u0441\u0442\u0430\u0440\u0442\u043e\u0432\u0438\u0439 \u043f\u0440\u043e\u043c\u043f\u0442.");
        return;
    }
    qDebug() << "[GEN] llama_decode \u0443\u0441\u043f\u0456\u0448\u043d\u043e! \u0456\u043d\u0456\u0446\u0456\u0430\u043b\u0456\u0437\u0443\u0454\u043c\u043e \u0441\u0435\u043c\u043f\u043b\u0435\u0440...";

    int n_cur = n_tokens;
    int n_max = 1024;

    // Покращене налаштування семплерів для стабільності та уникнення "галюцинацій"
    llama_sampler * sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.7f));         // Трохи менше хаосу
    llama_sampler_chain_add(sampler, llama_sampler_init_min_p(0.05f, 1));   // Відсікає малоймовірне сміття (чеську мову тощо)
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    qDebug() << "[GEN] \u0421\u0435\u043c\u043f\u043b\u0435\u0440 \u0441\u0442\u0432\u043e\u0440\u0435\u043d\u043e. \u0421\u0422\u0410\u0420\u0422\u0423\u0404\u041c\u041e \u0426\u0418\u041a\u041b \u0413\u0415\u041d\u0415\u0420\u0410\u0426\u0406\u0407!";
    
    while (n_cur <= n_tokens + n_max) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopRequested) break;
        }

        llama_token new_token_id = llama_sampler_sample(sampler, m_ctx, -1);
        llama_sampler_accept(sampler, new_token_id);
        
        if (llama_vocab_is_eog(vocab, new_token_id)) {
            qDebug() << "[GEN] EOG \u0442\u043e\u043a\u0435\u043d \u043e\u0442\u0440\u0438\u043c\u0430\u043d\u043e, \u0437\u0443\u043f\u0438\u043d\u044f\u0454\u043c\u043e \u0446\u0438\u043a\u043b.";
            break;
        }

        char buf[256];
        int len = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, false);
        if (len <= 0) continue;
        
        QString piece = QString::fromUtf8(buf, len);
        fullResponse += piece;
        emit tokenGenerated(piece);

        batch = llama_batch_get_one(&new_token_id, 1);
        if (llama_decode(m_ctx, batch)) {
            qDebug() << "[GEN] \u041f\u043e\u043c\u0438\u043b\u043a\u0430 decode \u043d\u0430 \u0442\u043e\u043a\u0435\u043d\u0456" << n_cur;
            emit errorOccurred("\u041f\u043e\u043c\u0438\u043b\u043a\u0430 \u043f\u0456\u0434 \u0447\u0430\u0441 \u0434\u0435\u043a\u043e\u0434\u0443\u0432\u0430\u043d\u043d\u044f \u043d\u0430\u0441\u0442\u0443\u043f\u043d\u043e\u0433\u043e \u0442\u043e\u043a\u0435\u043d\u0430.");
            break;
        }
        n_cur++;
    }

    qDebug() << "[GEN] \u0426\u0438\u043a\u043b \u0437\u0430\u0432\u0435\u0440\u0448\u0435\u043d\u043e. \u0412\u0456\u0434\u043f\u043e\u0432\u0456\u0434\u044c:" << fullResponse.left(50);
    llama_sampler_free(sampler);
    emit replyFinished(fullResponse);
}

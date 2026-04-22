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
    mparams.n_gpu_layers = 99; // Увімкнути максимальне прискорення на GPU, якщо доступне

    m_model = llama_load_model_from_file(modelPath.toUtf8().constData(), mparams);
    if (!m_model) {
        emit errorOccurred("Помилка завантаження моделі: " + modelPath + " (Перевірте, чи файл існує і чи не пошкоджений)");
        return false;
    }

    auto cparams = llama_context_default_params();
    cparams.n_ctx = 4096; // Контекст на 4096 токенів
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
    
    // Токенізація
    std::vector<llama_token> tokens_list(prompt.length() + 8);
    int n_tokens = llama_tokenize(m_model, prompt.c_str(), prompt.length(), tokens_list.data(), tokens_list.size(), true, true);
    if (n_tokens < 0) {
        tokens_list.resize(-n_tokens);
        n_tokens = llama_tokenize(m_model, prompt.c_str(), prompt.length(), tokens_list.data(), tokens_list.size(), true, true);
    }
    tokens_list.resize(n_tokens);

    // Очищаємо кеш контексту для "чистого" старту (поки що без пам'яті минулих повідомлень)
    llama_kv_cache_clear(m_ctx);

    QString fullResponse = "";
    
    // Запускаємо базовий batch
    llama_batch batch = llama_batch_get_one(tokens_list.data(), n_tokens, 0, 0);
    
    if (llama_decode(m_ctx, batch)) {
        emit errorOccurred("Llama.cpp Error: Не вдалося декодувати стартовий промпт.");
        return;
    }

    int n_cur = n_tokens;
    int n_max = 1024; // Максимальна довжина відповіді асистента
    
    // Цикл генерації тексту
    while (n_cur <= n_max + n_tokens) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopRequested) break;
        }

        // Отримуємо логіти для останнього токена
        float * logits = llama_get_logits_ith(m_ctx, batch.n_tokens - 1);
        int n_vocab = llama_n_vocab(m_model);
        
        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);
        for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
            candidates.push_back(llama_token_data{ token_id, logits[token_id], 0.0f });
        }
        llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };

        // Застосовуємо алгоритми вибору наступного слова
        llama_sample_top_k(m_ctx, &candidates_p, 40, 1);
        llama_sample_top_p(m_ctx, &candidates_p, 0.95f, 1);
        llama_sample_temp(m_ctx, &candidates_p, 0.8f);

        // Просимо модель вгадати наступний токен
        llama_token new_token_id = llama_sample_token(m_ctx, &candidates_p);
        
        // Перевіряємо чи модель вирішила що це кінець відповіді (<|im_end|>)
        if (llama_token_is_eog(m_model, new_token_id)) break;

        // Конвертуємо число-токен в текст
        char buf[128];
        int len = llama_token_to_piece(m_model, new_token_id, buf, sizeof(buf), 0, true);
        if (len < 0) continue;
        
        QString piece = QString::fromUtf8(buf, len);
        fullResponse += piece;
        
        // Віддаємо шматок слова в UI!
        emit tokenGenerated(piece);

        // Подаємо згенерований токен назад в модель для наступного кроку
        batch = llama_batch_get_one(&new_token_id, 1, n_cur, 0);
        if (llama_decode(m_ctx, batch)) {
             emit errorOccurred("Помилка під час декодування наступного токена.");
             break;
        }
        n_cur++;
    }

    emit replyFinished(fullResponse);
}

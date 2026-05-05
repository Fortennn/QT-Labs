#ifndef LLAMA_WORKER_THREAD_H
#define LLAMA_WORKER_THREAD_H

#include <QList>
#include <QMutex>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QWaitCondition>

struct llama_model;
struct llama_context;

class LlamaWorkerThread : public QThread {
    Q_OBJECT

public:
    // Which prompt template to wrap the user/assistant turns in. "Auto"
    // picks one from the model's filename (llama / chatml / mistral /
    // gemma); the rest force a specific format.
    enum class PromptTemplate {
        Auto = 0,
        ChatML,        // <|im_start|>role\n...<|im_end|>
        Llama3,        // <|begin_of_text|><|start_header_id|>...
        Mistral,       // [INST] ... [/INST]
        Gemma          // <start_of_turn>user/model
    };

    // Tunable generation parameters. Mirror the llama.cpp sampler chain
    // configured in processGeneration().
    struct GenParams {
        float temperature   = 0.80f;
        float topP          = 0.95f;
        int   topK          = 40;
        float minP          = 0.10f;
        float repeatPenalty = 1.10f;
        int   maxTokens     = 1024;
        int   contextSize   = 2048;
        // GPU offload — number of transformer layers pushed to the GPU.
        // 0 = CPU only, 99 = "everything you can fit". llama.cpp clamps
        // this to the actual layer count, so 99 is the canonical "all".
        int   gpuLayers     = 99;
        // Prompt format (affects the wrapper tokens around system / user
        // / assistant turns). "Auto" sniffs the model filename.
        PromptTemplate promptTemplate = PromptTemplate::Auto;
    };

    // Pick the most likely prompt template for a given model file path.
    // Used both for "Auto" mode and to seed the SettingsDialog combo box.
    static PromptTemplate detectTemplate(const QString& modelPath);

    explicit LlamaWorkerThread(QObject* parent = nullptr);
    ~LlamaWorkerThread() override;

    bool loadModel(const QString& modelPath);

    // Legacy two-knob setter — still used by older call sites.
    // Equivalent to copying the current params, overwriting these two
    // fields, and forwarding to setGenParams().
    void setParams(float temperature, int contextSize);

    // Update *all* generation knobs at once. Picked up on the next
    // processGeneration() invocation; contextSize on the next loadModel().
    void setGenParams(const GenParams& params);
    GenParams genParams() const;

    // Override the system prompt sent to the model. Empty -> use whatever
    // the caller passes to queuePrompt() / Config::SYSTEM_PROMPT.
    void setSystemPromptOverride(const QString& override);
    QString systemPromptOverride() const;

    void queueLoadModel(const QString& modelPath);
    void queuePrompt(const QString& systemPrompt, const QString& userPrompt);
    void stopGeneration();
    void clearHistory();

    // True while the worker is generating a response or has pending requests.
    // Lock-free read of an atomic flag — safe to call from any thread.
    bool    isBusy() const;
    // Filename of the currently loaded model (e.g. "llama-3.1-8b.gguf"),
    // or empty if no model is loaded. Read snapshot of m_loadedModelPath.
    QString loadedModelName() const;

signals:
    void modelLoaded(bool success);
    void tokenGenerated(const QString& token);
    void replyFinished(const QString& fullResponse);
    void errorOccurred(const QString& errorMsg);

protected:
    void run() override;

private:
    llama_model*   m_model = nullptr;
    llama_context* m_ctx   = nullptr;

    mutable QMutex m_mutex;
    QWaitCondition m_cond;

    struct ChatTurn {
        QString user;
        QString assistant;
    };
    QList<ChatTurn> m_history;

    struct Request {
        QString systemPrompt;
        QString userPrompt;
    };
    QQueue<Request> m_requests;
    QString m_modelPathToLoad;
    QString m_loadedModelPath;

    GenParams m_gen;                  // protected by m_mutex
    QString   m_systemPromptOverride; // protected by m_mutex

    bool m_stopRequested = false;
    bool m_quitThread    = false;
    // True from queuePrompt() until the corresponding processGeneration()
    // returns. Read via isBusy() to gate concurrent web requests so the
    // worker only handles one prompt at a time.
    bool m_busy          = false;

    void processGeneration(const Request& req);
};

#endif // LLAMA_WORKER_THREAD_H

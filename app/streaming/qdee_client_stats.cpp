#include "qdee_client_stats.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QString>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <chrono>

namespace qdee_client_stats {

static std::atomic<bool> s_running{false};
static std::atomic<bool> s_stopRequested{false};
static std::atomic<bool> s_sessionActive{false};
static std::atomic<unsigned int> s_fps{0};
static std::atomic<unsigned int> s_encodeMs{0};
static std::atomic<unsigned int> s_rttMs{0};
static std::atomic<unsigned int> s_dropped{0};
static std::atomic<unsigned int> s_bitrateKbps{0};
static QThread* s_thread = nullptr;
static QString s_cachedPath;

static QString path_for_instance(int instance_id)
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    return base + QString("/qdee-client-stats-%1.json").arg(instance_id);
}

static void writer_loop(int /*instance_id*/)
{
    while (!s_stopRequested.load()) {
        QJsonObject json;
        json["cmd"] = QStringLiteral("telemetry");
        json["fps"] = static_cast<int>(s_fps.load());
        json["encode_ms"] = static_cast<int>(s_encodeMs.load());
        json["rtt_ms"] = static_cast<int>(s_rttMs.load());
        json["bitrate_kbps"] = static_cast<int>(s_bitrateKbps.load());
        json["dropped"] = static_cast<int>(s_dropped.load());
        json["session_active"] = s_sessionActive.load();

        QJsonDocument doc(json);
        QByteArray payload = doc.toJson(QJsonDocument::Compact);

        QString tmpPath = s_cachedPath + ".tmp";
        QFile tmp(tmpPath);
        if (tmp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            tmp.write(payload);
            tmp.flush();
            tmp.close();
            QFile::remove(s_cachedPath);
            tmp.rename(s_cachedPath);
        }

        for (int i = 0; i < 50 && !s_stopRequested.load(); ++i) {
            QThread::msleep(10);
        }
    }
    QFile::remove(s_cachedPath);
}

void start(int instance_id)
{
    if (s_running.load()) return;
    s_cachedPath = path_for_instance(instance_id);
    s_stopRequested.store(false);
    s_running.store(true);
    s_thread = QThread::create([instance_id]() { writer_loop(instance_id); });
    s_thread->start();
}

void stop()
{
    if (!s_running.load()) return;
    s_stopRequested.store(true);
    if (s_thread) {
        s_thread->wait(3000);
        delete s_thread;
        s_thread = nullptr;
    }
    s_running.store(false);
}

void set_fps(unsigned int fps) { s_fps.store(fps); }
void set_encode_ms(unsigned int ms) { s_encodeMs.store(ms); }
void set_rtt_ms(unsigned int ms) { s_rttMs.store(ms); }
void add_dropped(unsigned int count) { s_dropped.fetch_add(count); }
void set_bitrate_kbps(unsigned int kbps) { s_bitrateKbps.store(kbps); }
void set_session_active(bool active) { s_sessionActive.store(active); }

QString stats_file_path(int instance_id) { return path_for_instance(instance_id); }

}

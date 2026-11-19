#include "car_audio/common/json.hpp"
#include "car_audio/common/types.hpp"
#include "car_audio/common/wav_io.hpp"
#include "car_audio/config/audio_config.hpp"
#include "car_audio/engine/audio_engine.hpp"
#include "car_audio/engine/audio_sink.hpp"
#include "car_audio/engine/audio_source.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace car_audio;

namespace {

std::shared_ptr<IAudioSource> make_source(const std::string& signal, double freq_hz,
                                          double duration_sec, bool loop) {
    AudioBuffer buf;
    if (signal == "chime") {
        buf = synth::generate_chime(freq_hz, freq_hz * 1.5, duration_sec, 0.5f);
    } else if (signal == "adas") {
        buf = synth::generate_adas_beep(freq_hz, duration_sec, 4.0, 0.7f);
    } else {
        buf = synth::generate_sine(freq_hz, duration_sec, 0.4f);
    }
    return std::make_shared<BufferAudioSource>(buf, loop);
}

void print_status(const AudioEngine& engine) {
    double sim_sec = static_cast<double>(engine.processed_frames()) / kEngineSampleRate;
    std::cout << "\n[Engine Status @ t=" << std::fixed << std::setprecision(2) << sim_sec
              << "s | Speed=" << engine.vehicle_speed() << " km/h]\n";
    auto streams = engine.active_streams();
    if (streams.empty()) {
        std::cout << "  (no active streams)\n";
        return;
    }
    for (const auto& s : streams) {
        std::cout << "  Stream #" << s.id << " | Zone=" << to_string(s.zone)
                  << " | Usage=" << to_string(s.usage)
                  << " | State=" << to_string(s.focus_state)
                  << " | Bus=" << s.bus_id
                  << " | Gain=" << std::setprecision(3) << s.current_gain << "\n";
    }
}

int run_scenario(AudioEngine& engine, const std::string& scenario_path,
                 const std::string& out_dir) {
    JsonValue doc = JsonValue::parse_file(scenario_path);
    std::cout << "Running scenario: " << doc.get_or("name", "Unnamed").as_string() << "\n";

    auto sink = std::make_shared<WavFileSink>(out_dir);
    engine.attach_sink(sink);

    struct ScheduledEvent {
        double time_sec;
        JsonValue raw;
    };
    std::vector<ScheduledEvent> events;
    for (const auto& ev : doc.at("events").as_array()) {
        events.push_back({ev.at("time_sec").as_double(), ev});
    }
    std::sort(events.begin(), events.end(),
              [](const ScheduledEvent& a, const ScheduledEvent& b) {
                  return a.time_sec < b.time_sec;
              });

    double total_sec = doc.get_or("total_duration_sec", 5.0).as_double(5.0);
    double cur_sec = 0.0;
    size_t ev_idx = 0;
    std::map<std::string, StreamId> tagged_streams;

    constexpr double kStepSec = 0.01;  // 10ms simulation tick
    while (cur_sec < total_sec) {
        while (ev_idx < events.size() && events[ev_idx].time_sec <= cur_sec + 1e-6) {
            const auto& ev = events[ev_idx].raw;
            std::string action = ev.at("action").as_string();
            if (action == "set_speed") {
                float spd = static_cast<float>(ev.at("speed_kmh").as_double());
                engine.set_vehicle_speed(spd);
                std::cout << "[t=" << std::fixed << std::setprecision(2) << cur_sec
                          << "s] Speed -> " << spd << " km/h\n";
            } else if (action == "start_stream") {
                auto zone = parse_audio_zone(ev.at("zone").as_string());
                auto usage = parse_audio_usage(ev.at("usage").as_string());
                auto focus = parse_focus_gain(ev.at("focus").as_string());
                std::string sig = ev.get_or("signal", "sine").as_string();
                double freq = ev.get_or("freq_hz", 440.0).as_double(440.0);
                double dur = ev.get_or("duration_sec", 2.0).as_double(2.0);
                bool loop = ev.get_or("loop", false).as_bool(false);
                auto src = make_source(sig, freq, dur, loop);
                auto id = engine.start_stream(zone, usage, focus, src);
                std::string tag = ev.get_or("tag", "").as_string();
                if (id.has_value()) {
                    if (!tag.empty()) tagged_streams[tag] = *id;
                    std::cout << "[t=" << cur_sec << "s] Started #" << *id
                              << " (" << to_string(zone) << "/" << to_string(usage) << ")\n";
                } else {
                    std::cout << "[t=" << cur_sec << "s] REJECTED (" << to_string(zone) << "/"
                              << to_string(usage) << ")\n";
                }
                print_status(engine);
            } else if (action == "stop_stream") {
                std::string tag = ev.at("tag").as_string();
                auto it = tagged_streams.find(tag);
                if (it != tagged_streams.end()) {
                    engine.stop_stream(it->second);
                    std::cout << "[t=" << cur_sec << "s] Stopped tag=" << tag << "\n";
                    print_status(engine);
                }
            }
            ++ev_idx;
        }
        engine.process_block(480);  // 10ms @ 48kHz
        cur_sec += kStepSec;
    }

    sink->flush();
    std::cout << "\nScenario complete. Exported bus WAV files to: " << out_dir << "\n";
    return 0;
}

void print_help() {
    std::cout << "\nCommands:\n"
              << "  play <zone> <usage> <focus> [freq_hz] [dur_sec]\n"
              << "       zone : DRIVER | PASSENGER | REAR_LEFT | REAR_RIGHT | CABIN_GLOBAL\n"
              << "       usage: MEDIA | NAVIGATION | VOICE_COMMAND | CALL | ADAS_WARNING | EMERGENCY_ECALL\n"
              << "       focus: GAIN | GAIN_TRANSIENT | GAIN_TRANSIENT_MAY_DUCK | GAIN_TRANSIENT_EXCLUSIVE\n"
              << "  stop <stream_id>\n"
              << "  vol <group> <step>        (media|navigation|voice|call|safety, 0..40)\n"
              << "  speed <kmh>\n"
              << "  fb <fade> <balance>       (-1.0 .. 1.0)\n"
              << "  eq <zone> <preset>        (Flat|Rock|Vocal|BassBoost)\n"
              << "  route <zone> <usage> <bus_id>\n"
              << "  step <seconds>            (advance simulation)\n"
              << "  status\n"
              << "  flush <out_dir>           (export recorded bus WAVs)\n"
              << "  quit\n\n";
}

void run_interactive(AudioEngine& engine) {
    auto sink = std::make_shared<WavFileSink>("out_interactive");
    engine.attach_sink(sink);
    std::cout << "CarAudioEngine Interactive Simulator. Type 'help' for commands.\n";

    std::string line;
    while (true) {
        std::cout << "car-audio> ";
        if (!std::getline(std::cin, line)) break;
        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd)) continue;

        try {
            if (cmd == "quit" || cmd == "exit") {
                break;
            } else if (cmd == "help") {
                print_help();
            } else if (cmd == "status") {
                print_status(engine);
            } else if (cmd == "play") {
                std::string z_str, u_str, f_str;
                double freq = 440.0, dur = 5.0;
                if (!(iss >> z_str >> u_str >> f_str)) {
                    std::cout << "Usage: play <zone> <usage> <focus> [freq] [dur]\n";
                    continue;
                }
                iss >> freq >> dur;
                auto id = engine.start_stream(parse_audio_zone(z_str), parse_audio_usage(u_str),
                                              parse_focus_gain(f_str),
                                              make_source("sine", freq, dur, false));
                if (id) {
                    std::cout << "Started stream #" << *id << "\n";
                } else {
                    std::cout << "Stream rejected by focus policy or missing route.\n";
                }
            } else if (cmd == "stop") {
                StreamId id = 0;
                if (iss >> id) engine.stop_stream(id);
            } else if (cmd == "vol") {
                std::string grp;
                int step = 20;
                if (iss >> grp >> step) engine.set_volume(grp, step);
            } else if (cmd == "speed") {
                float spd = 0.0f;
                if (iss >> spd) engine.set_vehicle_speed(spd);
            } else if (cmd == "fb") {
                float fade = 0.0f, bal = 0.0f;
                if (iss >> fade >> bal) engine.set_fade_balance(fade, bal);
            } else if (cmd == "eq") {
                std::string z_str, preset;
                if (iss >> z_str >> preset) {
                    engine.set_zone_eq_preset(parse_audio_zone(z_str), preset);
                }
            } else if (cmd == "route") {
                std::string z_str, u_str, bus;
                if (iss >> z_str >> u_str >> bus) {
                    engine.set_route_override(parse_audio_zone(z_str), parse_audio_usage(u_str),
                                              bus);
                }
            } else if (cmd == "step") {
                double sec = 1.0;
                if (iss >> sec) engine.run_simulation(sec);
                print_status(engine);
            } else if (cmd == "flush") {
                std::string dir;
                if (iss >> dir) {
                    auto custom_sink = std::make_shared<WavFileSink>(dir);
                    // Flush current sink
                    sink->flush();
                    std::cout << "Flushed WAVs.\n";
                } else {
                    sink->flush();
                    std::cout << "Flushed WAVs to out_interactive/.\n";
                }
            } else {
                std::cout << "Unknown command. Type 'help'.\n";
            }
        } catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << "\n";
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string config_path;
    std::string scenario_path;
    std::string out_dir = "sim_output";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "--scenario" && i + 1 < argc) {
            scenario_path = argv[++i];
        } else if (arg == "--out" && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: car_audio_cli [--config <json>] [--scenario <json> --out <dir>]\n";
            return 0;
        }
    }

    AudioConfig cfg =
        config_path.empty() ? AudioConfig::default_config() : AudioConfig::load_from_json(config_path);
    AudioEngine engine(std::move(cfg));

    if (!scenario_path.empty()) {
        return run_scenario(engine, scenario_path, out_dir);
    }
    run_interactive(engine);
    return 0;
}

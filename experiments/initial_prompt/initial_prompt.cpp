// Experimental characterization only; not part of tclwhisper or its API.
#include <whisper.h>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static_assert(CHAR_BIT == 8 && sizeof(float) == 4 &&
              std::numeric_limits<float>::is_iec559, "requires IEEE-754 binary32");
static const char prompt[] =
    "Iik', squawk, ILS, NOTAM, waypoint, VFR, heading, KREPE, Chihuahua.";

static std::string json(const std::string &s) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 0x20) out << "\\u" << std::hex << std::setw(4)
                               << std::setfill('0') << static_cast<unsigned>(c);
        else out << c;
    }
    out << '"';
    return out.str();
}

static std::vector<float> read_pcm(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path);
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
    if (in.bad() || bytes.empty() || bytes.size() % 4 || bytes.size()/4 > INT_MAX)
        throw std::runtime_error("invalid PCM size/read: " + path);
    std::vector<float> pcm(bytes.size()/4);
    for (size_t i = 0; i < pcm.size(); ++i) {
        const uint32_t bits = static_cast<uint32_t>(bytes[4*i]) |
            (static_cast<uint32_t>(bytes[4*i+1]) << 8) |
            (static_cast<uint32_t>(bytes[4*i+2]) << 16) |
            (static_cast<uint32_t>(bytes[4*i+3]) << 24);
        std::memcpy(&pcm[i], &bits, sizeof(bits));
    }
    return pcm;
}

static void log_to_file(enum ggml_log_level, const char *text, void *data) {
    auto *file = static_cast<FILE *>(data);
    std::fputs(text, file);
    std::fflush(file);
}

struct CloseLog {
    void operator()(FILE *file) const {
        whisper_log_set(nullptr, nullptr);
        std::fclose(file);
    }
};

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "usage: initial_prompt MODEL AUDIO_DIRECTORY OUTPUT_DIRECTORY\n";
        return 2;
    }
    try {
        const std::string model = argv[1], audio_dir = argv[2], out_dir = argv[3];
        std::unique_ptr<FILE, CloseLog> log(
            std::fopen((out_dir + "/upstream.log").c_str(), "w"));
        std::ofstream results(out_dir + "/results.jsonl");
        if (!log || !results) throw std::runtime_error("cannot open result files");
        whisper_log_set(log_to_file, log.get());
        const auto context_params = whisper_context_default_params();
        auto base = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        // Match the binding's recognition parameters and presentation flags.
        base.language = "spanish";
        base.n_threads = 4;
        base.carry_initial_prompt = false;
        base.print_progress = false;
        base.print_realtime = false;
        base.print_timestamps = false;
        base.print_special = false;
        if (base.initial_prompt || base.prompt_tokens || !base.no_context ||
            base.detect_language || base.translate || whisper_lang_id("spanish") != 3)
            throw std::runtime_error("unexpected upstream defaults/language");
        results << "{\"type\":\"configuration\",\"model\":" << json(model)
                << ",\"whisper_version\":" << json(whisper_version())
                << ",\"initial_prompt\":" << json(prompt)
                << ",\"language\":\"spanish\",\"n_threads\":4"
                << ",\"carry_initial_prompt\":false,\"no_context\":true"
                << ",\"detect_language\":false,\"translate\":false"
                << ",\"strategy\":\"greedy\",\"greedy_best_of\":" << base.greedy.best_of
                << ",\"context_use_gpu\":" << (context_params.use_gpu ? "true" : "false")
                << ",\"context_flash_attn\":" << (context_params.flash_attn ? "true" : "false")
                << ",\"system_info\":" << json(whisper_print_system_info()) << "}\n";
        results.flush();
        using Clock = std::chrono::steady_clock;
        for (int i = 1; i <= 5; ++i) {
            const std::string audio = "test" + std::to_string(i) + ".f32";
            const auto pcm = read_pcm(audio_dir + "/" + audio);
            for (int prompted = 0; prompted <= 1; ++prompted) {
                const std::string mode = prompted ? "prompt" : "baseline";
                auto params = base;
                if (prompted) params.initial_prompt = prompt;
                std::cout << "START " << audio << " " << mode << std::endl;
                std::fprintf(log.get(), "\nEXPERIMENT %s %s\n", audio.c_str(), mode.c_str());
                std::fflush(log.get());
                const auto init_start = Clock::now();
                std::unique_ptr<whisper_context, decltype(&whisper_free)> ctx(
                    whisper_init_from_file_with_params(model.c_str(), context_params), &whisper_free);
                const double init_s = std::chrono::duration<double>(Clock::now()-init_start).count();
                if (!ctx) throw std::runtime_error("model initialization failed");
                const auto start = Clock::now();
                const int rc = whisper_full(ctx.get(), params, pcm.data(), static_cast<int>(pcm.size()));
                const double infer_s = std::chrono::duration<double>(Clock::now()-start).count();
                std::string text;
                const int segments = rc == 0 ? whisper_full_n_segments(ctx.get()) : 0;
                for (int j = 0; j < segments; ++j) text += whisper_full_get_segment_text(ctx.get(), j);
                std::ostringstream row;
                row << std::fixed << std::setprecision(9)
                    << "{\"type\":\"result\",\"audio\":" << json(audio)
                    << ",\"mode\":" << json(mode) << ",\"model\":" << json(model)
                    << ",\"samples\":" << pcm.size() << ",\"duration_s\":" << pcm.size()/16000.0
                    << ",\"init_s\":" << init_s << ",\"inference_s\":" << infer_s
                    << ",\"return_code\":" << rc << ",\"segments\":" << segments
                    << ",\"text\":" << json(text) << "}";
                results << row.str() << '\n';
                results.flush();
                std::cout << row.str() << std::endl;
                if (rc != 0) throw std::runtime_error("whisper_full failed");
            }
        }
        whisper_log_set(nullptr, nullptr);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}

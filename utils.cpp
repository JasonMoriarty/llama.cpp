#include "utils.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <regex>
#include <iostream>
#include <iterator>
#include <queue>
#include <string>
#include <math.h>

 #if defined(_MSC_VER) || defined(__MINGW32__)
 #include <malloc.h> // using malloc.h with MSC/MINGW
 #elif !defined(__FreeBSD__) && !defined(__NetBSD__)
 #include <alloca.h>
 #endif

bool gpt_params_parse(int argc, char ** argv, gpt_params & params) {
    // determine sensible default number of threads.
    // std::thread::hardware_concurrency may not be equal to the number of cores, or may return 0.
#ifdef __linux__
    std::ifstream cpuinfo("/proc/cpuinfo");
    params.n_threads = std::count(std::istream_iterator<std::string>(cpuinfo),
                                  std::istream_iterator<std::string>(),
                                  std::string("processor"));
#endif
    if (params.n_threads == 0) {
        params.n_threads = std::max(1, (int32_t) std::thread::hardware_concurrency());
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-s" || arg == "--seed") {
            params.seed = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--threads") {
            params.n_threads = std::stoi(argv[++i]);
        } else if (arg == "-p" || arg == "--prompt") {
            params.prompt = argv[++i];
        } else if (arg == "-f" || arg == "--file") {
            std::ifstream file(argv[++i]);
            std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), back_inserter(params.prompt));
            if (params.prompt.back() == '\n') {
                params.prompt.pop_back();
            }
        } else if (arg == "-n" || arg == "--n_predict") {
            params.n_predict = std::stoi(argv[++i]);
        } else if (arg == "--top_k") {
            params.top_k = std::stoi(argv[++i]);
        } else if (arg == "-c" || arg == "--ctx_size") {
            params.n_ctx = std::stoi(argv[++i]);
        } else if (arg == "--memory_f16") {
            params.memory_f16 = true;
        } else if (arg == "--top_p") {
            params.top_p = std::stof(argv[++i]);
        } else if (arg == "--temp") {
            params.temp = std::stof(argv[++i]);
        } else if (arg == "--repeat_last_n") {
            params.repeat_last_n = std::stoi(argv[++i]);
        } else if (arg == "--repeat_penalty") {
            params.repeat_penalty = std::stof(argv[++i]);
        } else if (arg == "-b" || arg == "--batch_size") {
            params.n_batch = std::stoi(argv[++i]);
        } else if (arg == "-m" || arg == "--model") {
            params.model = argv[++i];
        } else if (arg == "-i" || arg == "--interactive") {
            params.interactive = true;
        } else if (arg == "-ins" || arg == "--instruct") {
            params.instruct = true;
        } else if (arg == "--color") {
            params.use_color = true;
        } else if (arg == "-r" || arg == "--reverse-prompt") {
            params.antiprompt.push_back(argv[++i]);
        } else if (arg == "--ignore-eos") {
            params.ignore_eos = true;
        } else if (arg == "-h" || arg == "--help") {
            gpt_print_usage(argc, argv, params);
            exit(0);
        } else if (arg == "--random-prompt") {
            params.random_prompt = true;
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            gpt_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void gpt_print_usage(int /*argc*/, char ** argv, const gpt_params & params) {
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h, --help            show this help message and exit\n");
    fprintf(stderr, "  -i, --interactive     run in interactive mode\n");
    fprintf(stderr, "  -ins, --instruct      run in instruction mode (use with Alpaca models)\n");
    fprintf(stderr, "  -r PROMPT, --reverse-prompt PROMPT\n");
    fprintf(stderr, "                        in interactive mode, poll user input upon seeing PROMPT (can be\n");
    fprintf(stderr, "                        specified more than once for multiple prompts).\n");
    fprintf(stderr, "  --color               colorise output to distinguish prompt and user input from generations\n");
    fprintf(stderr, "  -s SEED, --seed SEED  RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N, --threads N     number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "  -p PROMPT, --prompt PROMPT\n");
    fprintf(stderr, "                        prompt to start generation with (default: empty)\n");
    fprintf(stderr, "  --random-prompt       start with a randomized prompt.\n");
    fprintf(stderr, "  -f FNAME, --file FNAME\n");
    fprintf(stderr, "                        prompt file to start generation.\n");
    fprintf(stderr, "  -n N, --n_predict N   number of tokens to predict (default: %d)\n", params.n_predict);
    fprintf(stderr, "  --top_k N             top-k sampling (default: %d)\n", params.top_k);
    fprintf(stderr, "  --top_p N             top-p sampling (default: %.1f)\n", params.top_p);
    fprintf(stderr, "  --repeat_last_n N     last n tokens to consider for penalize (default: %d)\n", params.repeat_last_n);
    fprintf(stderr, "  --repeat_penalty N    penalize repeat sequence of tokens (default: %.1f)\n", params.repeat_penalty);
    fprintf(stderr, "  -c N, --ctx_size N    size of the prompt context (default: %d)\n", params.n_ctx);
    fprintf(stderr, "  --ignore-eos          ignore end of stream token and continue generating\n");
    fprintf(stderr, "  --memory_f16          use f16 instead of f32 for memory key+value\n");
    fprintf(stderr, "  --temp N              temperature (default: %.1f)\n", params.temp);
    fprintf(stderr, "  -b N, --batch_size N  batch size for prompt processing (default: %d)\n", params.n_batch);
    fprintf(stderr, "  -m FNAME, --model FNAME\n");
    fprintf(stderr, "                        model path (default: %s)\n", params.model.c_str());
    fprintf(stderr, "\n");
}

std::string gpt_random_prompt(std::mt19937 & rng) {
    const int r = rng() % 10;
    switch (r) {
        case 0: return "So";
        case 1: return "Once upon a time";
        case 2: return "When";
        case 3: return "The";
        case 4: return "After";
        case 5: return "If";
        case 6: return "import";
        case 7: return "He";
        case 8: return "She";
        case 9: return "They";
        default: return "To";
    }

    return "The";
}

void replace(std::string & str, const std::string & needle, const std::string & replacement) {
    size_t pos = 0;
    while ((pos = str.find(needle, pos)) != std::string::npos) {
        str.replace(pos, needle.length(), replacement);
        pos += replacement.length();
    }
}

std::map<std::string, int32_t> json_parse(const std::string & fname) {
    std::map<std::string, int32_t> result;

    // read file into string
    std::string json;
    {
        std::ifstream ifs(fname);
        if (!ifs) {
            fprintf(stderr, "Failed to open %s\n", fname.c_str());
            exit(1);
        }

        json = std::string((std::istreambuf_iterator<char>(ifs)),
                (std::istreambuf_iterator<char>()));
    }

    if (json[0] != '{') {
        return result;
    }

    // parse json
    {
        bool has_key  = false;
        bool in_token = false;

        std::string str_key = "";
        std::string str_val = "";

        int n = json.size();
        for (int i = 1; i < n; ++i) {
            if (!in_token) {
                if (json[i] == ' ') continue;
                if (json[i] == '"') {
                    in_token = true;
                    continue;
                }
            } else {
                if (json[i] == '\\' && i+1 < n) {
                    if (has_key == false) {
                        str_key += json[i];
                    } else {
                        str_val += json[i];
                    }
                    ++i;
                } else if (json[i] == '"') {
                    if (has_key == false) {
                        has_key = true;
                        ++i;
                        while (json[i] == ' ') ++i;
                        ++i; // :
                        while (json[i] == ' ') ++i;
                        if (json[i] != '\"') {
                            while (json[i] != ',' && json[i] != '}') {
                                str_val += json[i++];
                            }
                            has_key = false;
                        } else {
                            in_token = true;
                            continue;
                        }
                    } else {
                        has_key = false;
                    }

                    ::replace(str_key, "\\u0120", " " ); // \u0120 -> space
                    ::replace(str_key, "\\u010a", "\n"); // \u010a -> new line
                    ::replace(str_key, "\\\"",    "\""); // \\\"   -> "

                    try {
                        result[str_key] = std::stoi(str_val);
                    } catch (...) {
                        //fprintf(stderr, "%s: ignoring key '%s' with value '%s'\n", fname.c_str(), str_key.c_str(), str_val.c_str());

                    }
                    str_key = "";
                    str_val = "";
                    in_token = false;
                    continue;
                }
                if (has_key == false) {
                    str_key += json[i];
                } else {
                    str_val += json[i];
                }
            }
        }
    }

    return result;
}

std::vector<gpt_vocab::id> gpt_tokenize(const gpt_vocab & vocab, const std::string & text) {
    std::vector<std::string> words;

    // first split the text into words
    {
        std::string str = text;
        std::string pat = R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\s[:alpha:][:digit:]]+|\s+(?!\S)|\s+)";

        std::regex re(pat);
        std::smatch m;

        while (std::regex_search(str, m, re)) {
            for (auto x : m) {
                words.push_back(x);
            }
            str = m.suffix();
        }
    }

    // find the longest tokens that form the words:
    std::vector<gpt_vocab::id> tokens;
    for (const auto & word : words) {
        if (word.size() == 0) continue;

        int i = 0;
        int n = word.size();
        while (i < n) {
            int j = n;
            while (j > i) {
                auto it = vocab.token_to_id.find(word.substr(i, j-i));
                if (it != vocab.token_to_id.end()) {
                    tokens.push_back(it->second);
                    i = j;
                    break;
                }
                --j;
            }
            if (i == n) {
                break;
            }
            if (j == i) {
                auto sub = word.substr(i, 1);
                if (vocab.token_to_id.find(sub) != vocab.token_to_id.end()) {
                    tokens.push_back(vocab.token_to_id.at(sub));
                } else {
                    fprintf(stderr, "%s: unknown token '%s'\n", __func__, sub.data());
                }
                ++i;
            }
        }
    }

    return tokens;
}

static size_t utf8_len(char src) {
    const size_t lookup[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4 };
    uint8_t highbits = static_cast<uint8_t>(src) >> 4;
    return lookup[highbits];
}

struct llama_sp_symbol {
    using index = int;
    index prev;
    index next;
    std::string_view text;
};

struct llama_sp_bigram {
    struct comparator {
        bool operator()(llama_sp_bigram & l, llama_sp_bigram & r) {
            return (l.score < r.score) || (l.score == r.score && l.left > r.left);
        }
    };
    using queue_storage = std::vector<llama_sp_bigram>;
    using queue = std::priority_queue<llama_sp_bigram, queue_storage, comparator>;
    llama_sp_symbol::index left;
    llama_sp_symbol::index right;
    float score;
    size_t size;
};

struct llama_tokenizer {
    llama_tokenizer(const gpt_vocab & vocab): vocab_(vocab) {}

    void tokenize(std::string_view text, std::vector<gpt_vocab::id> & output) {
        // split string into utf8 chars
        int index = 0;
        while (!text.empty()) {
            llama_sp_symbol sym;
            size_t char_len = std::min(text.size(), utf8_len(text.data()[0]));
            sym.text = std::string_view(text.data(), char_len);
            sym.prev = index - 1;
            text.remove_prefix(char_len);
            sym.next = text.empty() ? -1 : index + 1;
            index++;
            symbols_.emplace_back(std::move(sym));
        }

        // seed the work queue with all possible 2-character tokens.
        for (size_t i = 1; i < symbols_.size(); ++i) {
            try_add_bigram(i - 1, i);
        }

        // keep substituting the highest frequency pairs for as long as we can.
        while (!work_queue_.empty()) {
            auto bigram = work_queue_.top();
            work_queue_.pop();

            auto & left_sym = symbols_[bigram.left];
            auto & right_sym = symbols_[bigram.right];

            // if one of the symbols already got merged, skip it.
            if (left_sym.text.empty() || right_sym.text.empty() ||
                left_sym.text.size() + right_sym.text.size() != bigram.size) {
                continue;
            }

            // merge the right sym into the left one
            left_sym.text = std::string_view(left_sym.text.data(), left_sym.text.size() + right_sym.text.size());
            right_sym.text = std::string_view("");

            // remove the right sym from the chain
            left_sym.next = right_sym.next;
            if (right_sym.next >= 0) {
                symbols_[right_sym.next].prev = bigram.left;
            }

            // find more substitutions
            try_add_bigram(left_sym.prev, bigram.left);
            try_add_bigram(bigram.left, left_sym.next);
        }

        for (int i = 0; i != -1; i = symbols_[i].next) {
            auto& symbol = symbols_[i];
            auto token = vocab_.token_to_id.find(std::string(symbol.text));

            if (token == vocab_.token_to_id.end()) {
                // output any symbols that did not form tokens as bytes.
                for (int j = 0; j < symbol.text.size(); ++j) {
                    gpt_vocab::id token_id = static_cast<uint8_t>(symbol.text[j]) + 3;
                    output.push_back(token_id);
                }
            } else {
                output.push_back((*token).second);
            }
        }
    }

private:
    void try_add_bigram(int left, int right) {
        if (left == -1 || right == -1) {
            return;
        }

        std::string_view text(symbols_[left].text.data(), symbols_[left].text.size() + symbols_[right].text.size());
        auto token = vocab_.token_to_id.find(std::string(text));

        if (token == vocab_.token_to_id.end()) {
            return;
        }

        auto score = vocab_.score.find((*token).second);

        if (score == vocab_.score.end()) {
            return;
        }

        llama_sp_bigram bigram;
        bigram.left = left;
        bigram.right = right;
        bigram.score = (*score).second;
        bigram.size = text.size();
        work_queue_.push(bigram);
    }

    const gpt_vocab & vocab_;
    std::vector<llama_sp_symbol> symbols_;
    llama_sp_bigram::queue work_queue_;
};

std::vector<gpt_vocab::id> llama_tokenize(const gpt_vocab & vocab, std::string_view text, bool bos) {
    llama_tokenizer tokenizer(vocab);
    std::vector<gpt_vocab::id> output;

    if (text.size() == 0) {
        return output;
    }

    if (bos) {
        output.push_back(1);
    }

    tokenizer.tokenize(text, output);
    return output;
}

bool gpt_vocab_init(const std::string & fname, gpt_vocab & vocab) {
    printf("%s: loading vocab from '%s'\n", __func__, fname.c_str());

    vocab.token_to_id = ::json_parse(fname);

    for (const auto & kv : vocab.token_to_id) {
        vocab.id_to_token[kv.second] = kv.first;
    }

    printf("%s: vocab size = %d\n", __func__, (int) vocab.token_to_id.size());

    // print the vocabulary
    //for (auto kv : vocab.token_to_id) {
    //    printf("'%s' -> %d\n", kv.first.data(), kv.second);
    //}

    return true;
}


void sample_top_k(std::vector<std::pair<double, gpt_vocab::id>> & logits_id, int top_k) {
    // find the top K tokens
    std::partial_sort(
            logits_id.begin(),
            logits_id.begin() + top_k, logits_id.end(),
            [](const std::pair<double, gpt_vocab::id> & a, const std::pair<double, gpt_vocab::id> & b) {
        return a.first > b.first;
    });

    logits_id.resize(top_k);
}

gpt_vocab::id llama_sample_top_p_top_k(
        const gpt_vocab & vocab,
        const float * logits,
        std::vector<gpt_vocab::id> & last_n_tokens,
        double repeat_penalty,
        int top_k,
        double top_p,
        double temp,
        std::mt19937 & rng) {
    int n_logits = vocab.id_to_token.size();

    std::vector<std::pair<double, gpt_vocab::id>> logits_id;
    logits_id.reserve(n_logits);

    {
        const double scale = 1.0/temp;
        for (int i = 0; i < n_logits; ++i) {
            // repetition penalty from CTRL paper (https://arxiv.org/abs/1909.05858)
            // credit https://github.com/facebookresearch/llama/compare/main...shawwn:llama:main
            if (std::find(last_n_tokens.begin(), last_n_tokens.end(), i) != last_n_tokens.end()) {
                // if score < 0 then repetition penalty has to multiplied to reduce the previous token probability
                if (logits[i] < 0.0) {
                    logits_id.push_back(std::make_pair(logits[i]*scale*repeat_penalty, i));
                } else {
                    logits_id.push_back(std::make_pair(logits[i]*scale/repeat_penalty, i));
                }
            } else {
                logits_id.push_back(std::make_pair(logits[i]*scale, i));
            }
        }
    }

    sample_top_k(logits_id, top_k);

    double maxl = -INFINITY;
    for (const auto & kv : logits_id) {
        maxl = std::max(maxl, kv.first);
    }

    // compute probs for the top K tokens
    std::vector<double> probs;
    probs.reserve(logits_id.size());

    double sum = 0.0;
    for (const auto & kv : logits_id) {
        double p = exp(kv.first - maxl);
        probs.push_back(p);
        sum += p;
    }

    // normalize the probs
    for (auto & p : probs) {
        p /= sum;
    }

    if (top_p < 1.0f) {
        double cumsum = 0.0f;
        for (int i = 0; i < (int) probs.size(); i++) {
            cumsum += probs[i];
            if (cumsum >= top_p) {
                probs.resize(i + 1);
                logits_id.resize(i + 1);
                break;
            }
        }

        cumsum = 1.0/cumsum;
        for (int i = 0; i < (int) probs.size(); i++) {
            probs[i] *= cumsum;
        }
    }

    //printf("\n");
    //for (int i = 0; i < (int) 10; i++) {
    //    printf("%d: '%s' %f\n", i, vocab.id_to_token.at(logits_id[i].second).c_str(), probs[i]);
    //}
    //printf("\n\n");
    //exit(0);

    std::discrete_distribution<> dist(probs.begin(), probs.end());
    int idx = dist(rng);

    return logits_id[idx].second;
}


size_t ggml_quantize_q4_0(float * src, void * dst, int n, int k, int qk, int64_t * hist) {
    const int nb = k / qk;
    const size_t bs = (sizeof(float) + sizeof(uint8_t)*qk/2);
    const size_t row_size = nb*bs;

    assert(k % qk == 0);

    const size_t pp_size = qk / 2;
    uint8_t *pp = static_cast<uint8_t*>(alloca(pp_size));

    char * pdst = (char *) dst;

    for (int j = 0; j < n; j += k) {
        uint8_t * pd = (uint8_t *) (pdst + (j/k)*row_size + 0*bs);
        uint8_t * pb = (uint8_t *) (pdst + (j/k)*row_size + 0*bs + sizeof(float));

        for (int i = 0; i < nb; i++) {
            float amax = 0.0f; // absolute max

            {
                for (int l = 0; l < qk; l++) {
                    const float v = src[j + i*qk + l];
                    amax = std::max(amax, fabsf(v));
                }

                const float d = amax / ((1 << 3) - 1);
                const float id = d ? 1.0f/d : 0.0f;

                *(float *) pd = d;
                pd += bs;

                for (int l = 0; l < qk; l += 2) {
                    const float v0 = (src[j + i*qk + l + 0])*id;
                    const float v1 = (src[j + i*qk + l + 1])*id;

                    const uint8_t vi0 = ((int8_t) (round(v0))) + 8;
                    const uint8_t vi1 = ((int8_t) (round(v1))) + 8;

                    assert(vi0 >= 0 && vi0 < 16);
                    assert(vi1 >= 0 && vi1 < 16);

                    hist[vi0]++;
                    hist[vi1]++;

                    pp[l/2] = vi0 | (vi1 << 4);
                }

                memcpy(pb, pp, pp_size);
                pb += bs;
            }
        }
    }

    return (n/k)*row_size;
}

size_t ggml_quantize_q4_1(float * src, void * dst, int n, int k, int qk, int64_t * hist) {
    const int nb = k / qk;
    const size_t bs = (2*sizeof(float) + sizeof(uint8_t)*qk/2);
    const size_t row_size = nb*bs;

    assert(k % qk == 0);

    const size_t pp_size = qk / 2;
    uint8_t *pp = static_cast<uint8_t*>(alloca(pp_size));

    char * pdst = (char *) dst;

    for (int j = 0; j < n; j += k) { 
        uint8_t * pd = (uint8_t *) (pdst + (j/k)*row_size + 0*bs);
        uint8_t * pm = (uint8_t *) (pdst + (j/k)*row_size + 0*bs +   sizeof(float));
        uint8_t * pb = (uint8_t *) (pdst + (j/k)*row_size + 0*bs + 2*sizeof(float));

        //printf("n = %d, k = %d, nb = %d, row_size = %d, j = %d, pm = %p, pd = %p, pb = %p\n", n, k, nb, row_size, j, pm, pd, pb);

        for (int i = 0; i < nb; i++) {
            float min = std::numeric_limits<float>::max();
            float max = std::numeric_limits<float>::min();

            {
                for (int l = 0; l < qk; l++) {
                    const float v = src[j + i*qk + l];
                    if (v < min) min = v;
                    if (v > max) max = v;
                }

                const float d = (max - min) / ((1 << 4) - 1);
                const float id = d ? 1.0f/d : 0.0f;

                *(float *) pd = d;
                *(float *) pm = min;
                pd += bs; 
                pm += bs;

                for (int l = 0; l < qk; l += 2) {
                    const float v0 = (src[j + i*qk + l + 0] - min)*id;
                    const float v1 = (src[j + i*qk + l + 1] - min)*id;

                    const uint8_t vi0 = round(v0);
                    const uint8_t vi1 = round(v1);

                    assert(vi0 >= 0 && vi0 < 16);
                    assert(vi1 >= 0 && vi1 < 16);

                    hist[vi0]++;
                    hist[vi1]++;

                    pp[l/2] = vi0 | (vi1 << 4);
                }

                memcpy(pb, pp, pp_size);
                pb += bs;
            }
        }
    }

    return (n/k)*row_size;
}

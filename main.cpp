#include "sample.h"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <iostream>
#include <fstream>
#include <cerrno>
#include <exception>
#include <array>
#include <vector>
#include <cstring>

#include <unistd.h>

using namespace std;

class MetronomeException final : public exception
{
    public:
        MetronomeException(string msg = R"(Metronome exception happened!)") : message(msg) {}
        ~MetronomeException() throw() {}
        const char* what() const throw() override { return message.c_str(); }

    private:
        string message;
};

class Metronome
{
    public:
        Metronome(uint8_t bpm, uint8_t signature, string filepath = string()) : m_bpm(bpm), m_signature(signature), m_filepath(filepath) {
            if (!(m_pasimple = pa_simple_new(nullptr, R"(cmetronome)", PA_STREAM_PLAYBACK, nullptr,
                            R"(cmetronome playback)", &m_pasamplespec, nullptr, nullptr, &m_error)))
                throw MetronomeException(string(R"(pa_simple_new() failed: )") + pa_strerror(m_error));
        }
        ~Metronome() {
            if (m_pasimple)
                pa_simple_free(m_pasimple);
        }
        void play() {
            vector<char> vsample;
            if (m_filepath.empty()) vsample.insert(vsample.end(), sample.begin(), sample.end());
            else {
                ifstream ifs(m_filepath, ifstream::binary);
                if (!ifs) throw MetronomeException(string(R"(Could not open file: )") + m_filepath);
                array<char, 64> buf;
                while (ifs.read(buf.data(), buf.size()) || ifs.gcount())
                    vsample.insert(vsample.end(), buf.begin(), buf.begin()+ifs.gcount());
                if (ifs.bad()) throw MetronomeException(string(R"(read() failed: )") + strerror(errno));
                ifs.close();
            }
            vsample.resize(m_pasamplespec.rate*60/m_bpm*2);
            auto empty_sample_rate = (m_pasamplespec.rate-1)*60/m_bpm;
            while (true) {
                if (pa_simple_write(m_pasimple, vsample.data(), vsample.size(), &m_error) < 0)
                    throw MetronomeException(string(R"(pa_simple_write() failed: )") + pa_strerror(m_error));
            }
            if (pa_simple_drain(m_pasimple, &m_error) < 0)
                throw MetronomeException(string(R"(pa_simple_drain() failed: )") + pa_strerror(m_error));
        }
    private:
        const pa_sample_spec m_pasamplespec = {
            .format = PA_SAMPLE_S16LE,
            .rate = 44100,
            .channels = 1
        };
        pa_simple *m_pasimple{nullptr};
        int m_error;
        uint8_t m_bpm;
        uint8_t m_signature;
        string m_filepath;
};

void print_usage(int status)
{
    cerr << "Usage: cmetronome [-t bpm] [-s signature] [-f filepath] [-h] [-v]\n";
    exit(status);
}

int main(int argc, char **argv)
{
    int opt, signature = 4, bpm = 60;
    string filepath;
    while ((opt = getopt(argc, argv, "t:s:f:hv")) != -1) {
        switch (opt) {
        case 'f': filepath = optarg; break;
        case 't': bpm = stoi(optarg); break;
        case 's': signature = stoi(optarg); break;
        case 'h': print_usage(EXIT_SUCCESS); break;
        case 'v': cout << "cmetronome 0.1\n"; return 0;
        default: print_usage(EXIT_FAILURE); break;
        }
    }

    try {
        Metronome metronome(bpm, signature, filepath);
        metronome.play();
    } catch (MetronomeException &e) {
        cout << e.what() << endl;
    }
}

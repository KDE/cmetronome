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
        Metronome(uint8_t bpm = 60, uint8_t signature = 4, string begin_filepath = string(),  string filepath = string())
            : m_bpm(bpm), m_signature(signature), m_begin_filepath(begin_filepath), m_filepath(filepath) {
            if (!(m_pasimple = pa_simple_new(nullptr, R"(cmetronome)", PA_STREAM_PLAYBACK, nullptr,
                            R"(cmetronome playback)", &m_pasamplespec, nullptr, nullptr, &m_error)))
                throw MetronomeException(string(R"(pa_simple_new() failed: )") + pa_strerror(m_error));
        }
        ~Metronome() {
            if (m_pasimple)
                pa_simple_free(m_pasimple);
        }
        void play() {
            vector<char> vbsample;
            if (m_begin_filepath.empty()) vbsample.insert(vbsample.end(), bsample.begin(), bsample.end());
            else {
                ifstream ifs(m_begin_filepath, ifstream::binary);
                if (!ifs) throw MetronomeException(string(R"(Could not open file: )") + m_begin_filepath);
                array<char, 64> buf;
                while (ifs.read(buf.data(), buf.size()) || ifs.gcount())
                    vbsample.insert(vbsample.end(), buf.begin(), buf.begin()+ifs.gcount());
                if (ifs.bad()) throw MetronomeException(string(R"(read() failed: )") + strerror(errno));
                ifs.close();
            }
            size_t ssample = m_pasamplespec.rate*60/m_bpm*m_pasamplespec.channels*pa_sample_size(&m_pasamplespec);
            vbsample.resize(ssample);
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
            vsample.resize(ssample);
            uint8_t count = 0;
            while (true) {
                if (!count++) {
                    if (pa_simple_write(m_pasimple, vbsample.data(), vbsample.size(), &m_error) < 0)
                        throw MetronomeException(string(R"(pa_simple_write() failed: )") + pa_strerror(m_error));
                } else {
                    if (pa_simple_write(m_pasimple, vsample.data(), vsample.size(), &m_error) < 0)
                        throw MetronomeException(string(R"(pa_simple_write() failed: )") + pa_strerror(m_error));
                    if (count == m_signature) count = 0;
                }
            }
            if (pa_simple_drain(m_pasimple, &m_error) < 0)
                throw MetronomeException(string(R"(pa_simple_drain() failed: )") + pa_strerror(m_error));
        }
    private:
        const pa_sample_spec m_pasamplespec = {
            /*.format =*/ PA_SAMPLE_S16LE,
            /*.rate =*/ 44100,
            /*.channels =*/ 1
        };
        pa_simple *m_pasimple{nullptr};
        int m_error;
        uint8_t m_bpm;
        uint8_t m_signature;
        string m_begin_filepath;
        string m_filepath;
};

void print_usage(int status)
{
    cerr << R"(Usage: cmetronome [option parameter])" << endl << endl;
    cerr << "\t" << R"(-t The bpm that defines the tempo. It must be a positive number.)" << endl;
    cerr << "\t" << R"(-s The signature that defines the beginning tick, e.g. 3 or 4.)" << endl;
    cerr << "\t" << R"(-f The filepath of the beginning tick. The file specified should contain raw PCM S16LE 44.1 kHz stereo samples)" << endl;
    cerr << "\t" << R"(-g The filepath of the subsequent ticks. The file specified should contain raw PCM S16LE 44.1 kHz stereo samples)" << endl;
    cerr << "\t" << R"(-h Display this help.)" << endl;
    cerr << "\t" << R"(-v Display the version.)" << endl;
    exit(status);
}

int main(int argc, char **argv)
{
    int opt, signature = 4, bpm = 60;
    string begin_filepath, filepath;
    while ((opt = getopt(argc, argv, "t:s:f:hv")) != -1) {
        switch (opt) {
        case 'f': begin_filepath = optarg; break;
        case 'g': filepath = optarg; break;
        case 't': try {bpm = stoi(optarg); if (bpm < 1) print_usage(EXIT_FAILURE);} catch(exception &) {print_usage(EXIT_FAILURE);} break;
        case 's': try {signature = stoi(optarg);} catch(exception &) {print_usage(EXIT_FAILURE);} break;
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

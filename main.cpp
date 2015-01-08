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
        Metronome(uint8_t bpm, uint8_t signature) : m_bpm(bpm), m_signature(signature) {
            if (!(m_pasimple = pa_simple_new(nullptr, R"(cmetronome)", PA_STREAM_PLAYBACK, nullptr,
                            R"(cmetronome playback)", &m_pasamplespec, nullptr, nullptr, &m_error)))
                throw MetronomeException(string(R"(pa_simple_new() failed: )") + pa_strerror(m_error));
        }
        ~Metronome() {
            if (m_pasimple)
                pa_simple_free(m_pasimple);
        }
        void play() {
            std::ifstream ifs(sm_filename, ifstream::binary);
            if(!ifs)
                throw MetronomeException(string(R"(Please check the file's existance: )") + sm_filename);
            array<char, 64> buf;
            vector<char> sample;
            sample.resize(m_pasamplespec.rate*60/m_bpm*2);
            while (true) {
                ifs.read(buf.data(), buf.size());
                if (ifs.eof()) break;
                else if (!ifs.good())
                    throw MetronomeException(string(R"(read() failed: )") + strerror(errno));
                sample.insert(sample.end(), buf.begin(), buf.begin()+ifs.gcount());
            }
            ifs.close();
            auto empty_sample_rate = (m_pasamplespec.rate-1)*60/m_bpm;
            while (true) {
                if (pa_simple_write(m_pasimple, sample.data(), sample.size(), &m_error) < 0)
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
        static constexpr const char *sm_filename = "metronome1_stereo.wav";
        int m_error;
        uint8_t m_bpm;
        uint8_t m_signature;
};

void print_usage(int status)
{
    cerr << "Usage: cmetronome [-t bpm] [-s signature] [-h] [-v]\n";
    exit(status);
}

int main(int argc, char **argv)
{
    int opt, signature = 4, bpm = 60;
    while ((opt = getopt(argc, argv, "t:s:hv")) != -1) {
        switch (opt) {
        case 't':
            bpm = stoi(optarg);
            break;
        case 's':
            signature = stoi(optarg);
            break;
        case 'h':
            print_usage(EXIT_SUCCESS);
            break;
        case 'v':
            cout << "cmetronome 0.1\n";
            return 0;
        default:
            print_usage(EXIT_FAILURE);
            break;
        }
    }

    try {
        Metronome metronome(bpm, signature);
        metronome.play();
    } catch (MetronomeException &e) {
        cout << e.what() << endl;
    }
}

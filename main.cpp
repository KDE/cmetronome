#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <cerrno>
#include <exception>
#include <array>
#include <vector>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#define BUFSIZE 1024

using namespace std;

class PulseAudioException : public exception
{
    public:
        PulseAudioException(string msg = R"(PulseAudio exception happened!)") : message(msg) {}
        ~PulseAudioException() throw() {}
        const char* what() const throw() { return message.c_str(); }

    private:
        string message;
};

class PulseAudio
{
    public:
        PulseAudio() {
            if (!(m_pasimple = pa_simple_new(nullptr, R"(cmetronome)", PA_STREAM_PLAYBACK, nullptr,
                            R"(cmetronome playback)", &m_pasamplespec, nullptr, nullptr, &m_error)))
                throw PulseAudioException(string(R"(pa_simple_new() failed: )") + pa_strerror(m_error));
        }
        ~PulseAudio() {
            if (m_pasimple)
                pa_simple_free(m_pasimple);
        }
        void play() {
            std::ifstream ifs(sm_filename, ifstream::binary);
            if(!ifs)
                throw PulseAudioException(string(R"(Please check the file's existance: )") + sm_filename);
            array<char, 64> buf;
            vector<char> sample;
            while (true) {
                ifs.read(buf.data(), buf.size());
                if (ifs.eof()) break;
                else if (!ifs.good())
                    throw PulseAudioException(string(R"(read() failed: )") + strerror(errno));
                sample.insert(sample.end(), buf.begin(), buf.begin()+ifs.gcount());
            }
            ifs.close();
            while (true) {
                if (pa_simple_write(m_pasimple, sample.data(), sample.size(), &m_error) < 0)
                    throw PulseAudioException(string(R"(pa_simple_write() failed: )") + pa_strerror(m_error));
                for (int i = 0; i < m_pasamplespec.rate-1; ++i) {
                    if (pa_simple_write(m_pasimple, empty_buf.data(), empty_buf.size(), &m_error) < 0)
                        throw PulseAudioException(string(R"(pa_simple_write() failed (empty): )") + pa_strerror(m_error));
                }
            }
            if (pa_simple_drain(m_pasimple, &m_error) < 0)
                throw PulseAudioException(string(R"(pa_simple_drain() failed: )") + pa_strerror(m_error));
        }
    private:
        const pa_sample_spec m_pasamplespec = {
            .format = PA_SAMPLE_S16LE,
            .rate = 44100,
            .channels = 2
        };
        pa_simple *m_pasimple{nullptr};
        static constexpr const char *sm_filename = "metronome1.wav";
        int m_error;
        array<char, 4> empty_buf{{0, 0, 0, 0}};
};

int main(int argc, char **argv)
{
    try {
        PulseAudio pulseAudio;
        pulseAudio.play();
    } catch (PulseAudioException &e) {
        cout << e.what() << endl;
    }
}

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
            while (true) {
                std::basic_ifstream<uint8_t> ifs(sm_filename, ifstream::binary);
                if(!ifs)
                    throw PulseAudioException(string(R"(Please check the file's existance: )") + sm_filename);
                array<uint8_t, 64> buf;
                ifs.read(buf.data(), buf.size());
                if (ifs.eof()) break;
                else if (!ifs.good())
                    throw PulseAudioException(string(R"(read() failed: )") + strerror(errno));
                if (pa_simple_write(m_pasimple, buf.data(), ifs.gcount(), &m_error) < 0)
                    throw PulseAudioException(string(R"(pa_simple_write() failed: )") + pa_strerror(m_error));
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

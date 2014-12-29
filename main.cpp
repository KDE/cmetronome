#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <cerrno>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#define BUFSIZE 1024

using namespace std;

class PulseAudio
{
    public:
        PulseAudio()
        {
        }
        ~PulseAudio()
        {
            if (m_simple)
                pa_simple_free(m_simple);
        }
private:
    constexpr pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    pa_simple *m_simple{nullptr};
};

int main(int argc, char **argv) {
    int ret = 1;
    int error;
    /* Create a new playback stream */
    if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, argv[0], &ss, NULL, NULL, &error))) {
        cerr << __FILE__": pa_simple_new() failed: " << pa_strerror(error) << endl;
        goto finish;
    }
    while (true) {
        uint8_t buf[BUFSIZE];
        ssize_t r;
#if 0
        pa_usec_t latency;
        if ((latency = pa_simple_get_latency(s, &error)) == (pa_usec_t) -1) {
            fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        fprintf(stderr, "%0.0f usec \r", (float)latency);
#endif
        /* Read some data ... */
        if ((r = read(STDIN_FILENO, buf, sizeof(buf))) <= 0) {
            if (r == 0) /* EOF */
                break;
            cerr << __FILE__": read() failed: " << strerror(errno) << endl;
            goto finish;
        }
        /* ... and play it */
        if (pa_simple_write(s, buf, (size_t) r, &error) < 0) {
            cerr << __FILE__": pa_simple_write() failed: " << pa_strerror(error) << endl;
            goto finish;
        }
    }
    /* Make sure that every single sample was played */
    if (pa_simple_drain(s, &error) < 0) {
        cerr << __FILE__": pa_simple_drain() failed: " << pa_strerror(error) << endl;
        goto finish;
    }
    ret = 0;
finish:
    return ret;
}

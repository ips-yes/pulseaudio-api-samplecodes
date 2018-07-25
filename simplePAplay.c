/* Playback using raw audio data from a file using pulseaudio simple API */
/* To build this test application, utilize the following command :       */
/* gcc -o spa simplePAplay.c  -lpulse-simple -lpulse */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#define BUFSIZE 1024

pa_simple *s = NULL;

void cleanup()
{
    if (s)
        pa_simple_free(s);
}

int main(int argc, char*argv[]) 
{
    /* The Sample format to use */
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    int ret = 1;
    int error;
    /* replace STDIN with the specified file if needed */
    if (argc > 1) {
        int fd;
        if ((fd = open(argv[1], O_RDONLY)) < 0) 
	{
            fprintf(stderr, __FILE__": open() failed: %s\n", strerror(errno));
            cleanup();
        }
        if (dup2(fd, STDIN_FILENO) < 0) 
	{
            fprintf(stderr, __FILE__": dup2() failed: %s\n", strerror(errno));
            cleanup();
        }
        close(fd);
    }
    /* Create a new playback stream */
    if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) 
    {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        cleanup();
    }


    for (;;) 
    {
        uint8_t buf[BUFSIZE];
        ssize_t r;
        /* Read some data ... */
        if ((r = read(STDIN_FILENO, buf, sizeof(buf))) <= 0) 
	{
            if (r == 0) /* EOF */
                break;
            fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
            cleanup();
	    exit(1);
        }
        /* ... and play it */
        if (pa_simple_write(s, buf, (size_t) r, &error) < 0) 
	{
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
            cleanup();
        }
    }
    /* Make sure that every single sample was played */
    if (pa_simple_drain(s, &error) < 0) 
    {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        cleanup();
	exit(1);
    }
    ret = 0;

    return ret;
}


/* simple.c
 *    Simple libftdi usage example
 *       This program is distributed under the GPL, version 2
 *       */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ftdi.h>
int main(int argc, char* argv[])
{
    int ret;
    struct ftdi_context *ftdi;
    struct ftdi_version_info version;
    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }
    ftdi_init(ftdi);
    version = ftdi_get_library_version();
    printf("Initialized libftdi %s (major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
            version.version_str, version.major, version.minor, version.micro,
            version.snapshot_str);
    if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6010)) < 0)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }
    ftdi_usb_reset(ftdi);
    //ftdi_setflowctrl(ftdi, SIO_RTS_CTS_HS);

    unsigned char channel = 0;
    if (argc == 2)
    {
        channel = atoi(argv[1]);
    }
    ftdi_set_bitmode(ftdi, channel, BITMODE_RESET);
    ftdi_set_baudrate(ftdi, 1000000);

    // Try to print alternating bits
    unsigned char toWrite[8];
    for (int i = 0; i < 8; ++i)
    {
        toWrite[i] = 0xAA;
    }
    const int toggle = 0x08;
    for (int i = 0;i<3;++i)
    {
        //toWrite ^= toggle;
        ftdi_write_data(ftdi, toWrite, 8);
        sleep(1);
    }

    if ((ret = ftdi_usb_close(ftdi)) < 0)
    {
        fprintf(stderr,
                "unable to close ftdi device: %d (%s)\n",
                ret,
                ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }
    ftdi_free(ftdi);
    return EXIT_SUCCESS;
}

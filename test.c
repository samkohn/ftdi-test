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
    /*ftdi_init(ftdi);*/
    version = ftdi_get_library_version();
    printf("Initialized libftdi %s (major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
            version.version_str, version.major, version.minor, version.micro,
            version.snapshot_str);
    if ((ret = ftdi_usb_open(ftdi, 0x0403, 0x6001)) < 0)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }
    ret = ftdi_usb_reset(ftdi);
    ret |= ftdi_setflowctrl(ftdi, SIO_RTS_CTS_HS);
    ret |= ftdi_set_event_char(ftdi, 0, 0);
    ret |= ftdi_set_error_char(ftdi, 0, 0);
    ret |= ftdi_set_bitmode(ftdi, 0x0, BITMODE_RESET);
    ret |= ftdi_set_bitmode(ftdi, 0x0, BITMODE_MPSSE);
    if(ret != 0)
    {
        fprintf(stderr, "unable to setup bitmode: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    unsigned char buffer[8];
    buffer[0] = 0x84; // to enable loopback
    ret = ftdi_write_data(ftdi, buffer, 1);
    if(ret != 1)
    {
        fprintf(stderr, "unable to set loopback: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    buffer[0] = 0xAB; // Bad command
    ret = ftdi_write_data(ftdi, buffer, 1);
    if(ret != 1)
    {
        fprintf(stderr, "unable to set loopback: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    ret = ftdi_read_data(ftdi, buffer, 2);
    if(ret <= 0)
    {
        fprintf(stderr, "unable to read data: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }
    printf("%d bytes read: 0x%02X 0x%02X\n", ret, buffer[0], buffer[1]);
    if(!(buffer[0] == 0xFA && buffer[1] == 0xAB))
    {
        fprintf(stderr, "loopback and bad command failed; unable to synchronize");
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    buffer[0] = 0x85; // disable loopback
    ret = ftdi_write_data(ftdi, buffer, 1);
    if(ret != 1)
    {
        fprintf(stderr, "unable to unset loopback: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    // Set up clock
    buffer[0] = 0x8A; // use 60 MHz master clock
    buffer[1] = 0x97; // turn off adaptive clocking (TODO why?)
    buffer[2] = 0x8D; // Disable three-phase clocking (TODO why?)

    ret = ftdi_write_data(ftdi, buffer, 3);
    if(ret != 3)
    {
        fprintf(stderr, "unable to set master clock correctly: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    // Set clock rate
    unsigned int divisor = 0x0002;
    buffer[0] = '\x86'; // Prepare to set clock divisor
    buffer[1] = divisor & 0xFF;
    buffer[2] = (divisor >> 8) & 0xFF;
    ret = ftdi_write_data(ftdi, buffer, 3);
    if(ret != 3)
    {
        fprintf(stderr, "unable to set clock divisor: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    /* these are the "low-byte" pins
     * pin  type     in/out  initial state
     * AD0  CK       output  high
     * AD1  TDI/DO   output  low
     * AD2  TDO/DI   input
     * AD3  TMS/CS   output  high
     * AD4  GPIO1    output  low
     * AD5  GPIO2    output  low
     * AD6  GPIO3    output  high
     * AD7  GPIO4    output  high
     */
    buffer[0] = 0x80; // configure data bits low-byte of MPSSE port (TODO)
    buffer[1] = 0xC9; // initial state configuration
    buffer[2] = 0xFB; // in/out configuration
    ret = ftdi_write_data(ftdi, buffer, 3);
    if(ret != 3)
    {
        fprintf(stderr, "unable to configure low-byte AD pins: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    // Now configure the high-byte pins (All GPIO, input, initial state
    // low)
    buffer[0] = 0x82; // configure the high-byte port (TODO)
    buffer[1] = 0x00; // initial state
    buffer[2] = 0x00; // in/out
    buffer[0] = 0x80; // configure data bits low-byte of MPSSE port (TODO)
    buffer[1] = 0xC9; // initial state configuration
    buffer[2] = 0xFB; // in/out configuration
    ret = ftdi_write_data(ftdi, buffer, 3);
    if(ret != 3)
    {
        fprintf(stderr, "unable to configure high-byte AC pins: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }

    // Time for communication
    buffer[0] = 0x10; // Output on rising CK, no input, MSB first, clock bytes out
    buffer[1] = 0x05; // Length L
    buffer[2] = 0x00; // Length H  // Somehow length gets 1 added to it for a total of 2
    buffer[3] = 0xAB; // Data byte 1
    buffer[4] = 0xAB; // Data byte 2
    buffer[5] = 0xAB; // Data byte 3
    buffer[6] = 0xAB; // Data byte 4
    buffer[7] = 0xAB; // Data byte 5
    ret = ftdi_write_data(ftdi, buffer, 8);
    if(ret != 8)
    {
        fprintf(stderr, "transmission failed: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }


    /*
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
    */

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

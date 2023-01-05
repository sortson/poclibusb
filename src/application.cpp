#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <utility>
#include <iostream>
#include <libusb-1.0/libusb.h>

#define ERR_EXIT(errcode) do { perr("   %s\n", libusb_strerror((enum libusb_error)errcode)); return -1; } while (0)
#define CALL_CHECK(fcall) do { int _r=fcall; if (_r < 0) ERR_EXIT(_r); } while (0)
#define CALL_CHECK_CLOSE(fcall, hdl) do { int _r=fcall; if (_r < 0) { libusb_close(hdl); ERR_EXIT(_r); } } while (0)

// HID Class-Specific Requests values. See section 7.2 of the HID specifications
#define HID_GET_REPORT                0x01
#define HID_REPORT_TYPE_INPUT         0x01

struct ps3 {
    bool select;
    bool start;

    bool up;
    bool down;
    bool right;
    bool left;

    bool triangle;
    bool circle;
    bool square;
    bool cross;

    bool left_one;
    bool right_one;
    bool left_two;
    bool right_two;
    bool left_three;
    bool right_three;

    bool ps_button;

    int left_analog_x;
    int left_analog_y;
    int right_analog_x;
    int right_analog_y;

    int acceleration;

};

static inline void msleep(int msecs) {
#if defined(_WIN32)
    Sleep(msecs);
#else
    const struct timespec ts = { msecs / 1000, (msecs % 1000) * 1000000L };
    nanosleep(&ts, nullptr);
#endif
}

static void perr(char const *format, ...) {
    va_list args;

    va_start (args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

class UsbManager {
public:
    explicit UsbManager(std::function<void(ps3)> callback, int sleep_time = 0, bool print_mode = false, bool debug_mode = false) {
        this->callback = std::move(callback);
        this->sleep_time = sleep_time;
        this->print_mode = print_mode;
        this->debug_mode = debug_mode;
        running = true;
    }

    int start() {
        struct libusb_config_descriptor *conf_desc;
        libusb_device *dev;
        int r;
        int iface, nb_ifaces;

        if (int init_status = init() < 0) {
            return init_status;
        }

        libusb_device_handle *handle;
        uint16_t vid = 0x054C, pid = 0x0268;

        printf("Opening device %04X:%04X...\n", vid, pid);
        handle = libusb_open_device_with_vid_pid(nullptr, vid, pid);

        if (handle == nullptr) {
            perr("  Failed.\n");
            return -1;
        }

        dev = libusb_get_device(handle);
        CALL_CHECK_CLOSE(libusb_get_config_descriptor(dev, 0, &conf_desc), handle);
        nb_ifaces = conf_desc->bNumInterfaces;
        printf(" Interfaces: %d\n", nb_ifaces);
        libusb_set_auto_detach_kernel_driver(handle, 1);
        for (iface = 0; iface < nb_ifaces; iface++)
        {
            int ret = libusb_kernel_driver_active(handle, iface);
            printf("\nKernel driver attached for interface %d: %d\n", iface, ret);
            printf("\nClaiming interface %d...\n", iface);
            r = libusb_claim_interface(handle, iface);
            if (r != LIBUSB_SUCCESS) {
                perr("   Failed.\n");
            }
        }

        while (running) {
            process(handle);
            if (sleep_time > 0) msleep(sleep_time);
        }

        printf("\n");
        for (iface = 0; iface<nb_ifaces; iface++) {
            printf("Releasing interface %d...\n", iface);
            libusb_release_interface(handle, iface);
        }

        printf("Closing device...\n");
        libusb_close(handle);

        libusb_exit(nullptr);

        return 0;
    }

    void stop() {
        printf("Stopping...\n");
        running = false;
    }

private:
    std::function<void(ps3)> callback;
    bool running, print_mode, debug_mode;
    int sleep_time;

    [[nodiscard]] int init() const {
        static char debug_env_str[] = "LIBUSB_DEBUG=4";	// LIBUSB_LOG_LEVEL_DEBUG
        int init_status;

        if (debug_mode) {
            if (putenv(debug_env_str) != 0) {
                printf("Unable to set debug level\n");
            }
        } else {
            libusb_set_option(nullptr, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
        }

        init_status = libusb_init(nullptr);
        if (init_status < 0) { return init_status; }

        return 0;
    }

    int process(libusb_device_handle *handle) {
        ps3 data {};
        uint8_t input_report[49];

        // Get the status of the controller's buttons via its HID report
        if (print_mode) printf("\nReading PS3 Input Report...\n");
        CALL_CHECK(libusb_control_transfer(handle, LIBUSB_ENDPOINT_IN|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE,
                                           HID_GET_REPORT, (HID_REPORT_TYPE_INPUT<<8)|0x01, 0, input_report, sizeof(input_report), 1000));

        /** Direction pad plus start, select, and joystick buttons */



        if (input_report[2] == 0x01) {
            data.select = true;
            if (print_mode) printf("\tSELECT pressed\n");
        } else {
            data.select = false;
        }

        if (input_report[2] == 0x02) {
            data.left_three = true;
            if (print_mode) printf("\tLEFT 3 pressed\n");
        } else {
            data.left_three = false;
        }

        if (input_report[2] == 0x04) {
            data.right_three = true;
            if (print_mode) printf("\tRIGHT 3 pressed\n");
        } else {
            data.right_three = false;
        }

        if (input_report[2] == 0x08) {
            data.start = true;
            if (print_mode) printf("\tSTART pressed\n");
        } else {
            data.start = false;
        }

        if (input_report[2] == 0x10) {
            data.up = true;
            if (print_mode) printf("\tUP pressed\n");
        } else {
            data.up = false;
        }

        if (input_report[2] == 0x20) {
            data.right = true;
            if (print_mode) printf("\tRIGHT pressed\n");
        } else {
            data.right = false;
        }

        if (input_report[2] == 0x40) {
            data.down = true;
            if (print_mode) printf("\tDOWN pressed\n");
        } else {
            data.down = false;
        }

        if (input_report[2] == 0x80) {
            data.left = true;
            if (print_mode) printf("\tLEFT pressed\n");
        } else {
            data.left = false;
        }

        /** Shapes plus top right and left buttons */
        if (input_report[3] == 0x01) {
            data.left_two = true;
            if (print_mode) printf("\tLEFT 2 pressed\n");
        } else {
            data.left_two = false;
        }

        if (input_report[3] == 0x02) {
            data.right_two = true;
            if (print_mode) printf("\tRIGHT 2 pressed\n");
        } else {
            data.right_two = false;
        }

        if (input_report[3] == 0x04) {
            data.left_one = true;
            if (print_mode) printf("\tLEFT 1 pressed\n");
        } else {
            data.left_one = false;
        }

        if (input_report[3] == 0x08) {
            data.right_one = true;
            if (print_mode) printf("\tRIGHT 1 pressed\n");
        } else {
            data.right_one = false;
        }

        if (input_report[3] == 0x10) {
            data.triangle = true;
            if (print_mode) printf("\tTRIANGLE pressed\n");
        } else {
            data.triangle = false;
        }

        if (input_report[3] == 0x20) {
            data.circle = true;
            if (print_mode) printf("\tCIRCLE pressed\n");
        } else {
            data.circle = false;
        }

        if (input_report[3] == 0x40) {
            data.cross = true;
            if (print_mode) printf("\tCROSS pressed\n");
        } else {
            data.cross = false;
        }

        if (input_report[3] == 0x80) {
            data.square = true;
            if (print_mode) printf("\tSQUARE pressed\n");
        } else {
            data.square = false;
        }

        data.ps_button = input_report[4];
        if (print_mode) printf("\tPS button: %d\n", data.ps_button);

        data.left_analog_x = input_report[6];
        data.left_analog_y = input_report[7];
        if (print_mode) printf("\tLeft Analog (X,Y): (%d,%d)\n", data.left_analog_x, data.left_analog_y);

        data.right_analog_x = input_report[8];
        data.right_analog_y = input_report[9];
        if (print_mode) printf("\tRight Analog (X,Y): (%d,%d)\n", data.right_analog_x, data.right_analog_y);

        if (print_mode) printf("\tL2 Value: %d\tR2 Value: %d\n", input_report[18], input_report[19]);
        if (print_mode) printf("\tL1 Value: %d\tR1 Value: %d\n", input_report[20], input_report[21]);

        if (print_mode) printf("\tRoll (x axis): %d Yaw (y axis): %d Pitch (z axis) %d\n",
                    //(((input_report[42] + 128) % 256) - 128),
                               (int8_t)(input_report[42]),
                               (int8_t)(input_report[44]),
                               (int8_t)(input_report[46]));
        data.acceleration = input_report[48];
        if (print_mode) printf("\tAcceleration: %d\n\n", (int8_t)data.acceleration);


        callback(data);

        return 0;
    }

};

int main(int argc, char** argv) {
    printf("Main method\n");
    ps3 oldData {};
    UsbManager manager([&oldData, &manager](ps3 info) {
        if (info.ps_button) manager.stop();

        if (info.left && !oldData.left) {
            printf("\tLEFT!!!\n");
        }

        if (info.right  && !oldData.right) {
            printf("\tRIGHT!!!\n");
        }

        if (info.up && !oldData.up) {
            printf("\tUP!!!\n");
        }

        if (info.down && !oldData.down) {
            printf("\tDOWN!!!\n");
        }

        if (info.start && !oldData.start) {
            printf("\tSTART!!!\n");
        }

        if (info.select && !oldData.select) {
            printf("\tSELECT!!!\n");
        }

        if (info.left_one && !oldData.left_one) {
            printf("\tLEFT ONE!!!\n");
        }

        if (info.right_one && !oldData.right_one) {
            printf("\tRIGHT ONE!!!\n");
        }

        if (info.left_two && !oldData.left_two) {
            printf("\tLEFT TWO!!!\n");
        }

        if (info.right_two && !oldData.right_two) {
            printf("\tRIGHT TWO!!!\n");
        }

        if (info.left_three && !oldData.left_three) {
            printf("\tLEFT THREE!!!\n");
        }

        if (info.right_three && !oldData.right_three) {
            printf("\tRIGHT THREE!!!\n");
        }

        if (info.triangle && !oldData.triangle) {
            printf("\tTRIANGLE!!!\n");
        }

        if (info.circle && !oldData.circle) {
            printf("\tCIRCLE!!!\n");
        }

        if (info.cross && !oldData.cross) {
            printf("\tCROSS!!!\n");
        }

        if (info.square && !oldData.square) {
            printf("\tSQUARE!!!\n");
        }

        oldData = info;
    }, 0, true, true);
    manager.start();
    return 0;
}

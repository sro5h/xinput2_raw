#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

bool initXInput(Display*, int*);
void handleXIEvent(XGenericEventCookie*);

int main(void) {
        Display *d;
        Window w;
        XEvent e;
        int s, xiOpcode;

        d = XOpenDisplay(NULL);
        if (d == NULL) {
                fprintf(stderr, "Cannot open display\n");
                return EXIT_FAILURE;
        }

        s = DefaultScreen(d);

        if (!initXInput(d, &xiOpcode))
        {
                fprintf(stderr, "XInput not availlable\n");
                return EXIT_FAILURE;
        }

        w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, 100, 100, 1,
                        BlackPixel(d, s), WhitePixel(d, s));
        XSelectInput(d, w, ExposureMask | KeyPressMask | PointerMotionMask);

        // XInput2 events
        XIEventMask eventmask;
        unsigned char mask[XIMaskLen(XI_RawMotion)];
        std::memset(mask, 0, sizeof(mask));
        eventmask.deviceid = XIAllMasterDevices;
        eventmask.mask_len = sizeof(mask);
        eventmask.mask = mask;
        XISetMask(mask, XI_RawMotion);
        if (XISelectEvents(d, DefaultRootWindow(d), &eventmask, 1) != Success)
        {
                fprintf(stderr, "Could not set XInput2 event mask\n");
                return EXIT_FAILURE;
        }

        XMapWindow(d, w);

        while (1) {
                XNextEvent(d, &e);
                switch (e.type)
                {
                        case Expose:
                                printf("Exposed\n");
                                break;

                        case MotionNotify:
                                break;

                        case GenericEvent:
                                if (e.xcookie.extension == xiOpcode
                                                && XGetEventData(d, &e.xcookie))
                                {
                                        handleXIEvent(&e.xcookie);
                                }
                                break;
                }
        }

        XCloseDisplay(d);
        return EXIT_SUCCESS;
}

bool initXInput(Display* d, int* opcode)
{
        int event, error;
        if (XQueryExtension(d, "XInputExtension", opcode, &event, &error))
        {
                int major = 2, minor = 0;
                if (XIQueryVersion(d, &major, &minor) != BadRequest)
                {
                        printf("ver: %d.%d\n", major, minor);
                        return true;
                }
        }

        return false;
}

void handleXIEvent(XGenericEventCookie* cookie)
{
        if (cookie->evtype == XI_RawMotion)
        {
                const XIRawEvent* rawe;
                rawe = static_cast<const XIRawEvent*>(cookie->data);

                const int maxValues = 2; // Only two mouse axis
                int values[2] = { 0, 0 };
                int top = rawe->valuators.mask_len;
                if (top > maxValues)
                        top = maxValues;
                top *= 8;

                // Get raw values
                const double* input_values = rawe->raw_values;
                for (int i = 0; i < top && i < maxValues; ++i)
                {
                        if (XIMaskIsSet(rawe->valuators.mask, i))
                        {
                                values[i] = static_cast<int>(*input_values);
                                ++input_values;
                        }
                }

                printf("Raw motion: %d, %d\n", values[0], values[1]);
        }
}

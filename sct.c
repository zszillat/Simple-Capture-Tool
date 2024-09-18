#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <png.h>

//global variables

int help() {
    printf("-p, --path");
    printf("-n, --name");
    printf("-c, --clipboard");
}

void copyToClipboard(char *filename, char *filetype, char *filepath) {

    char command[256];
    snprintf(command, sizeof(command), "xclip -selection clipboard -t image/ppm -i %s", filepath);
    system(command);
}

int saveScreenshot(XRectangle hole, char *filename, char *filetype, char *filepath) {

    Display *display = XOpenDisplay(NULL);

    if (!display) {
        fprintf(stderr, "Unable to open X display\n");
        return EXIT_FAILURE;
    }

    Window root = DefaultRootWindow(display);
    XEvent event;
    XUngrabPointer(display, CurrentTime);
    XImage *image = XGetImage(display, root, hole.x, hole.y, hole.width, hole.height, AllPlanes, ZPixmap);

    if (!image) {
        fprintf(stderr, "Failed to capture image\n");
        return EXIT_FAILURE;
    }

    // Create the full filepath with filename and extension
    strcat(filename, ".png");
    strcat(filepath, "/");
    strcat(filepath, filename);

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open output file\n");
        XDestroyImage(image);
        return EXIT_FAILURE;
    }

    // Initialize libpng
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "Failed to create PNG write structure\n");
        fclose(f);
        XDestroyImage(image);
        return EXIT_FAILURE;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "Failed to create PNG info structure\n");
        png_destroy_write_struct(&png, NULL);
        fclose(f);
        XDestroyImage(image);
        return EXIT_FAILURE;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "Error during PNG creation\n");
        png_destroy_write_struct(&png, &info);
        fclose(f);
        XDestroyImage(image);
        return EXIT_FAILURE;
    }

    png_init_io(png, f);

    // Set PNG header info
    png_set_IHDR(png, info, hole.width, hole.height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    // Write image data
    png_bytep row = (png_bytep) malloc(3 * hole.width * sizeof(png_byte));
    for (int y = 0; y < hole.height; ++y) {
        for (int x = 0; x < hole.width; ++x) {
            unsigned long pixel = XGetPixel(image, x, y);
            row[x * 3]     = (pixel & image->red_mask) >> 16;  // Red
            row[x * 3 + 1] = (pixel & image->green_mask) >> 8; // Green
            row[x * 3 + 2] = (pixel & image->blue_mask);       // Blue
        }
        png_write_row(png, row);
    }

    // End write
    png_write_end(png, NULL);

    // Cleanup
    free(row);
    png_destroy_write_struct(&png, &info);
    fclose(f);
    XDestroyImage(image);
    XCloseDisplay(display);

    return EXIT_SUCCESS;
}


int main(int argc, char *argv[]) {

    const char* homeDir = getenv("HOME");

    //ARGS
    char filename[256];
    char filepath[256] = ""; // Default filepath
    strcpy(filepath, homeDir);
    char filetype[256] = "ppm";     // Default filetype

    // Get current date and time for default filename if -n is absent
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(filename, sizeof(filename), "%d-%02d-%02d_%02d:%02d:%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    // Loop through arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            strncpy(filename, argv[i + 1], sizeof(filename) - 1);
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(filepath, argv[i + 1], sizeof(filepath) - 1);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            strncpy(filetype, argv[i + 1], sizeof(filetype) - 1);
        }
    }

    Display *display;

    Window window;
    XSetWindowAttributes attributes;
    XVisualInfo vinfo;
    Colormap colormap;
    XEvent event;
    int screen;
    unsigned long white_pixel, black_pixel;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Unable to open X display\n");
        exit(1);
    }

    screen = DefaultScreen(display);
    white_pixel = WhitePixel(display, screen);
    black_pixel = BlackPixel(display, screen);

    int screen_width = DisplayWidth(display, screen);
    int screen_height = DisplayHeight(display, screen);

    // Choose a 32-bit visual for transparency
    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
        fprintf(stderr, "No matching visual found\n");
        exit(1);
    }

    colormap = XCreateColormap(display, RootWindow(display, screen), vinfo.visual, AllocNone);
    attributes.colormap = colormap;
    attributes.background_pixel = white_pixel;
    attributes.border_pixel = black_pixel;
    attributes.override_redirect = True;

    window = XCreateWindow(display, RootWindow(display, screen), 0, 0, screen_width, screen_height, 0,
                           vinfo.depth, InputOutput, vinfo.visual,
                           CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attributes);

    // Set window transparency using _NET_WM_WINDOW_OPACITY
    unsigned long opacity = (unsigned long)(0.5 * 0xFFFFFFFF); // 50% transparency
    Atom atom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(display, window, atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1);

    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

    XGrabPointer(display, window, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

    XMapWindow(display, window);

    int start_x; 
    int start_y; 
    int end_x; 
    int end_y;

    //wait for mouse press down
    while (1) {
        XNextEvent(display, &event);
        if (event.type == ButtonPress && event.xbutton.button == Button1) {
            start_x = event.xbutton.x_root;
            start_y = event.xbutton.y_root;
            break;
        }
    }

    //make initial rectangle
    XRectangle hole = {start_x, start_y, 0,0 };
    XShapeCombineRectangles(display, window, ShapeBounding, start_x, start_y, &hole, 1, ShapeSubtract, Unsorted);

    // // Wait for the mouse release
    while (1) {
        XNextEvent(display, &event);
        end_x = event.xbutton.x_root;
        end_y = event.xbutton.y_root;

        if (event.type == MotionNotify) {

            //fill in hole
            XShapeCombineRectangles(display, window, ShapeBounding, 0, 0, &hole, 1, ShapeUnion, Unsorted);
            
            if (end_x - start_x < 0 ) { 

                hole.x = end_x;
                hole.width = start_x - end_x;

            } else {

                hole.width = end_x - start_x;

            }
            if (end_y - start_y < 0 ) { 
                
                hole.y = end_y;
                hole.height = start_y - end_y;
                
            } else {

                hole.height = end_y - start_y;

            }

            XShapeCombineRectangles(display, window, ShapeBounding, 0, 0, &hole, 1, ShapeSubtract, Unsorted);

        }
        if (event.type == ButtonRelease && event.xbutton.button == Button1) {
            end_x = event.xbutton.x_root;
            end_y = event.xbutton.y_root;
            break;
        }

        
    }

    XCloseDisplay(display);

    sleep(1);

    saveScreenshot(hole, filename, filetype, filepath);

    sleep(1);

    //copyToClipboard(filename, filetype, filepath);
    
    return 0;
}

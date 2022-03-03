
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <X11/keysymdef.h>
#include <assert.h>
#include <malloc.h>
#include <solver.h>
#include <stdio.h>
#include <time.h>
#include <uiohook.h>
#include <unistd.h>

#include <chrono>
#include <color/color.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

int ReadFile(std::string fname, std::map<std::string, double> *m) {
  int count = 0;
  if (access(fname.c_str(), R_OK) < 0) return -errno;

  FILE *fp = fopen(fname.c_str(), "r");
  if (!fp) return -errno;

  m->clear();

  char *buf = 0;
  size_t buflen = 0;

  while (getline(&buf, &buflen, fp) > 0) {
    char *nl = strchr(buf, '\n');
    if (nl == NULL) continue;
    *nl = 0;

    char *sep = strchr(buf, '=');
    if (sep == NULL) continue;
    *sep = 0;
    sep++;

    std::string s1 = buf;
    std::string s2 = sep;

    (*m)[s1] = std::stod(sep);

    count++;
  }

  if (buf) free(buf);

  fclose(fp);
  return count;
}

std::pair<int, int> getMousePos() {
  int number_of_screens;
  int i;
  Bool result;
  Window *root_windows;
  Window window_returned;
  int root_x, root_y;
  int win_x, win_y;
  unsigned int mask_return;

  Display *display = XOpenDisplay(NULL);
  assert(display);
  // XSetErrorHandler(_XlibErrorHandler);
  number_of_screens = XScreenCount(display);
  root_windows = (Window *)malloc(sizeof(Window) * number_of_screens);
  for (i = 0; i < number_of_screens; i++) {
    root_windows[i] = XRootWindow(display, i);
  }
  for (i = 0; i < number_of_screens; i++) {
    result = XQueryPointer(display, root_windows[i], &window_returned,
                           &window_returned, &root_x, &root_y, &win_x, &win_y,
                           &mask_return);
    if (result == True) {
      break;
    }
  }
  if (result != True) {
    fprintf(stderr, "No mouse found.\n");
    ;
  }
  free(root_windows);
  XCloseDisplay(display);
  return std::pair<int, int>(root_x, root_y);
}

bool logger_proc(unsigned int level, const char *format, ...) {
  bool status = false;

  va_list args;
  switch (level) {
    case LOG_LEVEL_INFO:
      va_start(args, format);
      status = vfprintf(stdout, format, args) >= 0;
      va_end(args);
      break;

    case LOG_LEVEL_WARN:
    case LOG_LEVEL_ERROR:
      va_start(args, format);
      status = vfprintf(stderr, format, args) >= 0;
      va_end(args);
      break;
  }

  return status;
}
bool corner1Defined;
std::pair<int, int> corner1;
std::pair<int, int> corner2;

struct Pixel {
  int x;
  int y;
  int r;
  int g;
  int b;
};

Pixel color_grey = Pixel{0, 0, 162, 162, 162};
Pixel color_yellow = Pixel{0, 0, 198, 233, 1};
Pixel color_green = Pixel{0, 0, 106, 170, 100};
Pixel color_white = Pixel{0, 0, 255, 255, 255};

auto colors =
    std::vector<Pixel>{color_grey, color_yellow, color_green, color_white};
auto colorsWithoutWhite =
    std::vector<Pixel>{color_grey, color_yellow, color_green};

bool colorEquals(Pixel p1, Pixel p2) {
  return p1.r == p2.r && p1.g == p2.g && p1.b == p2.b;
}

void microsleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void moveMouse(int x, int y) {
  Display *d = XOpenDisplay((char *)NULL);
  XWarpPointer(d, None, DefaultRootWindow(d), 0, 0, 0, 0, x, y);
  XSync(d, false);
  XCloseDisplay(d);
}

void get_pixel_color(Pixel *p) {
  Display *d = XOpenDisplay((char *)NULL);
  XColor c;

  // std::cout << "x: " << p->x << " y: " << p->y << " / ";

  XImage *image;
  image = XGetImage(d, XRootWindow(d, XDefaultScreen(d)), p->x, p->y, 1, 1,
                    AllPlanes, XYPixmap);
  c.pixel = XGetPixel(image, 0, 0);
  XFree(image);
  XQueryColor(d, XDefaultColormap(d, XDefaultScreen(d)), &c);
  p->r = c.red / 256;
  p->g = c.green / 256;
  p->b = c.blue / 256;
  XCloseDisplay(d);
  moveMouse(p->x, p->y);
  microsleep(200);
}

double distance(Pixel pixel1, Pixel pixel2) {
  color::rgb<double> rgb1{pixel1.r / 255.0, pixel1.g / 255.0, pixel1.b / 255.0};
  color::rgb<double> rgb2{pixel2.r / 255.0, pixel2.g / 255.0, pixel2.b / 255.0};

  return color::operation::distance<
      color::constant::distance::CIEDE2000_entity>(rgb1, rgb2);
}

Pixel getClosest(Pixel p, bool withWhite) {
  std::vector<Pixel> used;
  if (withWhite)
    used = colors;
  else
    used = colorsWithoutWhite;

  auto minElement = used[0];
  auto min = distance(p, used[0]);

  for (auto color : used) {
    auto temp = distance(p, color);
    if (min > temp) {
      min = temp;
      minElement = color;
    }
  }
  return minElement;
}

ColorWordle getLine(int line) {
  ColorWordle wordle;
  auto columnSize = (corner2.first - corner1.first) / 4;
  auto lineSize = (corner2.second - corner1.second) / 5;

  auto current = corner1.first - columnSize;
  auto linePos = corner1.second + lineSize * line;

  for (int i = 0; i < 5; i++) {
    current = columnSize + current;
    Pixel p{current, linePos, 0, 0, 0};
    get_pixel_color(&p);

    if (colorEquals(getClosest(p, true), color_white)) {
      auto position = current;
      int i = 0;
      do {
        p.x += 5;
        get_pixel_color(&p);
        i++;
        if (i > 10) {
          return ColorWordle{};
        }
      } while (colorEquals(getClosest(p, true), color_white));
    }
    auto closest = getClosest(p, false);
    // std::cout << p.r << " " << p.g << " " << p.b << std::endl;
    if (colorEquals(closest, color_grey)) wordle.push_back(Color::grey);

    if (colorEquals(closest, color_yellow)) wordle.push_back(Color::yellow);

    if (colorEquals(closest, color_green)) wordle.push_back(Color::green);
  }
  return wordle;
}

void dispatch_proc(uiohook_event *const event) {
  // std::cout << "aid=" << event->type << " ";

  switch (event->type) {
    case EVENT_KEY_PRESSED: {
      if (event->data.keyboard.keycode == 3665) {
        if (!corner1Defined) {
          corner1 = getMousePos();
          corner1Defined = true;
          std::cout << corner1.first << " " << corner1.second << std::endl;
        } else {
          corner2 = getMousePos();
          hook_stop();
          std::cout << corner2.first << " " << corner2.second << std::endl;
        }
      }

      break;
    }
  }
}

void toggleKey(unsigned int key, bool pressed) {
  Display *d = XOpenDisplay((char *)NULL);
  const auto is_press = pressed ? True : False;
  XTestFakeKeyEvent(d, key, is_press, CurrentTime);
  XSync(d, false);
  XCloseDisplay(d);
}

unsigned int getKeyCodeFromChar(char c) {
  Display *d = XOpenDisplay((char *)NULL);

  char buf[2];
  buf[0] = c;
  buf[1] = '\0';

  auto code = XKeysymToKeycode(d, XStringToKeysym(buf));
  XCloseDisplay(d);
  return code;
}

void pressKey(unsigned int code) {
  std::cout << "Key press: " << (int)code << std::endl;
  toggleKey(code, true);
  microsleep(20.0);
  toggleKey(code, false);
  microsleep(100.0);
}

void pressKey(char c) { pressKey(getKeyCodeFromChar(c)); }

void pressKeys(std::string str) {
  for (char c : str) pressKey(c);
  pressKey((unsigned int)36);
}
void mouseClick(int button) {
  Display *display = XOpenDisplay(NULL);

  XEvent event;

  if (display == NULL) {
    std::cout << "clicking error 0" << std::endl;
    exit(EXIT_FAILURE);
  }

  memset(&event, 0x00, sizeof(event));

  event.type = ButtonPress;
  event.xbutton.button = button;
  event.xbutton.same_screen = True;

  XQueryPointer(display, RootWindow(display, DefaultScreen(display)),
                &event.xbutton.root, &event.xbutton.window,
                &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x,
                &event.xbutton.y, &event.xbutton.state);

  event.xbutton.subwindow = event.xbutton.window;

  while (event.xbutton.subwindow) {
    event.xbutton.window = event.xbutton.subwindow;
    XQueryPointer(display, event.xbutton.window, &event.xbutton.root,
                  &event.xbutton.subwindow, &event.xbutton.x_root,
                  &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
                  &event.xbutton.state);
  }

  if (XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0)
    std::cout << "clicking error 1" << std::endl;

  XFlush(display);

  event.type = ButtonRelease;
  event.xbutton.state = 0x100;

  if (XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0)
    std::cout << "clicking error 2" << std::endl;

  XFlush(display);

  XCloseDisplay(display);
}

int main() {
  hook_set_logger_proc(&logger_proc);
  hook_set_dispatch_proc(dispatch_proc);

  std::cout
      << "Welcome in WordleReader press right control twice while putting "
         "your mouse in diagonal corners of the wordle"
      << std::endl;

  hook_run();
  mouseClick(1);
  microsleep(500);

  std::vector<std::string> v;
  std::string line;
  std::ifstream fin("words.txt");
  while (getline(fin, line)) {
    v.push_back(line);
  }

  auto values = v;

  std::map<std::string, double> averages;
  if (ReadFile("result.txt", &averages) == -errno)
    averages = calculateAllProbas(v);
  int l = 0;
  while (1) {
    auto best = getBestValue(averages);
    std::cout << "Best input: " << best << std::endl;
    pressKeys(best);

    sleep(1);
    auto wordle = getLine(l);
    std::cout << "Wordle: " << wordle[0] << wordle[1] << wordle[2] << wordle[3]
              << wordle[4] << std::endl;

    values = filterValues(values, best, wordle);
    if (values.size() == 1) {
      std::cout << "The solution is: " << values[0] << std::endl;

      pressKeys(values[0]);
      exit(0);
    }
    if (values.size() == 0) {
      std::cout << "there is no solution" << std::endl;
      exit(0);
    }

    averages = calculateAllProbas(values);

    std::cout << "Possible words: ";
    for (std::string str : values) {
      std::cout << str << " ";
    }
    std::cout << std::endl;
    l++;
  }
}


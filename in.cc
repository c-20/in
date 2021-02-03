#if 0
gcc=g++
src=in.cc
out=in
echo "Compiling in..."
$gcc -o $out $src
echo "Compiling test..."
./test.gcc # recompile test
exit 0
#else
#include <iostream>
using namespace std;
#define KEYBOARD   "./test 9"
#define EXTBOARD   "./test 15"
#define KEYCOLUMN  12
#define KEYCSTATE  5
#define SHIFTKEY   50


void handlekey(int key, char state) {
  if (key == SHIFTKEY) {
    if (state == 'p') { cout << "SHIFT KEY PRESSED\n"; }
    if (state == 'r') { cout << "SHIFT KEY RELEASED\n"; }
  } else {
    string status = (state == 'p') ? "PRESSED" :
                    (state == 'r') ? "RELEASED" : "UNKNOWN";
    cout << "KEY " << key << " " << status << "\n";
  }
}

int main(int argc, char **argv) {
  // TODO: multiplex keyboard and extboard
  system("stty -echo");
  FILE *kbd = popen(KEYBOARD, "r");
  if (!kbd) { cout << "NO KEYBOARD\n"; }
  // todo: nonblocking?????
  int log = (argc > 1 && argv[1][0] == '-') ? 1 : 0;
  int col = 0;
  int key = 0;
  char keystate = 'x';
  while (1) {
    int inch = fgetc(kbd);
    if (inch == EOF) { break; }
    char ch = (char)inch;
    if (ch == '\n') {
      if (log) { cout << '\n'; }
      handlekey(key, keystate);
      col = 0; key = 0; keystate = 'x';
    } else if (++col == KEYCSTATE) {
      keystate = ch;
    } else if (col > KEYCOLUMN) {
      if (log) { cout << "(" << (int)ch << ")"; }
      if (ch == ' ') { } // expect a space after the number
      else if (ch < '0' || ch > '9')
        { cout << "UNEXPECTED KEYCH: " << ch << "\n"; }
      else { key = (key * 10) + (ch - '0'); }
    } else if (col <= KEYCOLUMN) { }
    else { cout << "UNEXPECTED CH: " << ch << "\n"; }
  }
  fclose(kbd);
  system("stty echo");
  return 0;
}
#endif


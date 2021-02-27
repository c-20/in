#if 0
name=keyboard
gcc=g++
src=$name.cc
out=$name
echo "Compiling $name..."
$gcc -o $out $src
echo "Note: requires test."
exit 0
#else

#include <iostream>
#include <stdio.h>
using namespace std;
#define FAILEXIT    exit(EXIT_FAILURE)
#define MOVEUP(n)   printf("\033[%dA", n)
#undef MOVEUP
#define MOVEUP(n)
// blank MOVEUP for debug output
#define FPUT2C(a, b, dest)     fputc(a, dest); fputc(b, dest)
#define FPUT3C(a, b, c, dest)  fputc(a, dest); fputc(b, dest); fputc(c, dest)

//#include "initshift.h" // not with popen mode/yet

//#undef INPUTPATH // .h defines for in/ subdir
#define TESTPATH   "./test"
#define TESTKBD1   "9"
#define TESTKBD2   "15"
// todo: track which is which TODO TODO TODO TODO TODO

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

struct termios originalterm;
void disablerawmode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalterm);
}
void enablerawmode() {
  tcgetattr(STDIN_FILENO, &originalterm);
  atexit(disablerawmode);
  struct termios raw = originalterm;
  raw.c_lflag &= ~(ECHO | ICANON); // (ECHO | ICANON);
//  raw.c_lflag &= ~(ECHO | ICANON);
//  raw.c_cc[VMIN] = 0;  // min chars to stop waiting
//  raw.c_cc[VTIME] = 1; // a tenth of a second
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
// WHY DOES THIS SEEM TO MAKE CHILD THREADS NOT RESPOND
// WHY DOES THIS BREAK PIPE RETURN
// TERMINAL CAN ONLY HAVE ONE MODE????
}

#define KEYBOARDLEN    256
#define KBMAXKEYSDOWN  10    // seems to be 6 for numbers, less for letters
#define KBACTIVEKEY    0
#define KBTOGGLEKEY    1
#define KBMODIFIERKEY  2


typedef struct _Keyboard {
  char  feature[KEYBOARDLEN];
  char  pressed[KEYBOARDLEN];
  int  modifier;
  char keys[KBMAXKEYSDOWN];
  int keysdown;
  void *dnroutine[KEYBOARDLEN];
  void *uproutine[KEYBOARDLEN];
  void *sdnroutine[KEYBOARDLEN];
  void *suproutine[KEYBOARDLEN];
  void *cdnroutine[KEYBOARDLEN];
  void *cuproutine[KEYBOARDLEN];
  void *adnroutine[KEYBOARDLEN];
  void *auproutine[KEYBOARDLEN];
  void *csdnroutine[KEYBOARDLEN];
  void *csuproutine[KEYBOARDLEN];
  void *asdnroutine[KEYBOARDLEN];
  void *asuproutine[KEYBOARDLEN];
  void *cadnroutine[KEYBOARDLEN];
  void *cauproutine[KEYBOARDLEN];
  void *casdnroutine[KEYBOARDLEN];
  void *casuproutine[KEYBOARDLEN];
  struct _Keyboard *next;
} Keyboard;
Keyboard keyboard;


void clearkeyboard(Keyboard *kb) {
  kb->modifier = 0;
  kb->keysdown = 0;
 // consider kb->keys list ?
  int ix = -1;
  while (++ix < KEYBOARDLEN) {
    kb->feature[ix] = 0;
    kb->pressed[ix] = ' ';
    kb->dnroutine[ix] = NULL;
    kb->uproutine[ix] = NULL;
    kb->sdnroutine[ix] = NULL;
    kb->suproutine[ix] = NULL;
    kb->cdnroutine[ix] = NULL;
    kb->cuproutine[ix] = NULL;
    kb->adnroutine[ix] = NULL;
    kb->auproutine[ix] = NULL;
    kb->csdnroutine[ix] = NULL;
    kb->csuproutine[ix] = NULL;
    kb->asdnroutine[ix] = NULL;
    kb->asuproutine[ix] = NULL;
    kb->cadnroutine[ix] = NULL;
    kb->cauproutine[ix] = NULL;
    kb->casdnroutine[ix] = NULL;
    kb->casuproutine[ix] = NULL;
  }
}

int haskeyfeature(Keyboard *kb, int keyindex, int feature) {
  return ((kb->feature[keyindex] & (1 << feature)) ? 1 : 0);
}

void enablekeyfeature(Keyboard *kb, int keyindex, int feature) {
  kb->feature[keyindex] |= (1 << feature);
}

void enablemodifierkey(Keyboard *kb, int keyindex) {
  enablekeyfeature(kb, keyindex, KBACTIVEKEY);
  enablekeyfeature(kb, keyindex, KBMODIFIERKEY);
}

void enablekeyrange(Keyboard *kb, int firstindex, int lastindex) {
  int keyindex = firstindex - 1;
  while (++keyindex <= lastindex) {
    enablekeyfeature(kb, keyindex, KBACTIVEKEY);
  }
}

#define KBSHIFTMODIFIER  1
#define KBCTRLMODIFIER   2
#define KBALTMODIFIER    3
#define KBWINMODIFIER    4
#define KBLSHIFTKEY      50
#define KBRSHIFTKEY      62
#define KBLCTRLKEY       37
//#define KBRCTRLKEY       37
#define KBLALTKEY        64
#define KBRALTKEY        108
#define KBLWINKEY        133
//#define KBRWINKEY        133
//#define fn doesn't map to a key
#define KBNUMBER1S  10
#define KBNUMBER1T  19
#define KBLETTER1S  24
#define KBLETTER1T  33
#define KBLETTER2S  36
#define KBLETTER2T  46
#define KBLETTER3S  52
#define KBLETTER3T  60

void initstandardkeyboard(Keyboard *kb) {
  enablemodifierkey(kb, KBLSHIFTKEY);
  enablemodifierkey(kb, KBRSHIFTKEY);
  enablemodifierkey(kb, KBLCTRLKEY);
  enablemodifierkey(kb, KBLWINKEY);
  enablemodifierkey(kb, KBLALTKEY);
  enablemodifierkey(kb, KBRALTKEY);
  enablekeyrange(kb, KBNUMBER1S, KBNUMBER1T);
  enablekeyrange(kb, KBLETTER1S, KBLETTER1T);
  enablekeyrange(kb, KBLETTER2S, KBLETTER2T);
  enablekeyrange(kb, KBLETTER3S, KBLETTER3T);
// todo: enable symbol keys
// todo: unrecognised key function
}

void seekeyboard(Keyboard *kb) {
  int ix = -1;
  int inactive = 0;
  while (++ix < KEYBOARDLEN) {
    if (haskeyfeature(kb, ix, KBACTIVEKEY)) {
      char kbch = (char)(kb->pressed[ix]);
      if (haskeyfeature(kb, ix, KBMODIFIERKEY)) {
        kbch = (kbch != ' ') ? 'M' : 'm';
      } else if (inactive == 0) { // range start
        cout << ix << "[";
        inactive = 1; // else it is 1 already
      }
      cout << kbch; // visible active key
    } else {
      if (inactive == 1) { // range stop
        cout << "]" << (ix - 1);
        inactive = 0; // else it is not 1
      }
    }
  }
}

void modifierdn(Keyboard *kb) { // should only become pressed if active
// hold both alt keys for delete-key modifier
  int newmodifier = (0 + ((kb->pressed[KBLSHIFTKEY] != ' ') || (kb->pressed[KBRSHIFTKEY] != ' ')) ? (1 << KBSHIFTMODIFIER) : 0) +
                    (0 +  (kb->pressed[KBLCTRLKEY]  != ' ')                                       ? (1 << KBCTRLMODIFIER)  : 0) +
                    (0 + ((kb->pressed[KBLALTKEY]   != ' ') || (kb->pressed[KBRALTKEY]   != ' ')) ? (1 << KBALTMODIFIER)   : 0) +
                    (0 +  (kb->pressed[KBLWINKEY]   != ' ')                                       ? (1 << KBWINMODIFIER)   : 0);
  kb->modifier = newmodifier;
}
void modifierup(Keyboard *kb) {
  modifierdn(kb); // recalculate modifier -- currently same for dn and up
}
int ismodifierdown(Keyboard *kb, int modifier) {
  return ((kb->modifier & (1 << modifier)) ? 1 : 0);
}

void presskey(Keyboard *kb, int keyindex) { // aka keydn()
  int ix = keyindex; // key remap / cipher table can go here
  if (!haskeyfeature(kb, ix, KBACTIVEKEY)) {
    enablekeyfeature(kb, ix, KBACTIVEKEY);
  } // activate key if inactive
  kb->pressed[ix % KEYBOARDLEN] = '*';
  if (!haskeyfeature(kb, ix, KBMODIFIERKEY)) {
    kb->keys[kb->keysdown] = ix;
    if (kb->keysdown++ >= KBMAXKEYSDOWN)
      { cout << "too many keys\n"; kb->keysdown--; }
  } // exclude modifiers from key list
  if (haskeyfeature(kb, ix, KBMODIFIERKEY))
    { modifierdn(kb); } // modifier triggers modify key triggers
  void *triggerdn = kb->dnroutine[ix];
  if (ismodifierdown(kb, KBSHIFTMODIFIER)) {
    if (ismodifierdown(kb, KBCTRLMODIFIER)) {
      if (ismodifierdown(kb, KBALTMODIFIER)) {
        triggerdn = kb->casdnroutine[ix];
      } else { triggerdn = kb->csdnroutine[ix]; }
    } else if (ismodifierdown(kb, KBALTMODIFIER)) {
      triggerdn = kb->asdnroutine[ix];
    } else { triggerdn = kb->sdnroutine[ix]; }
  } else if (ismodifierdown(kb, KBCTRLMODIFIER)) {
    if (ismodifierdown(kb, KBALTMODIFIER)) {
      triggerdn = kb->cadnroutine[ix];
    } else { triggerdn = kb->cdnroutine[ix]; }
  } else if (ismodifierdown(kb, KBALTMODIFIER)) {
    triggerdn = kb->adnroutine[ix];
  } // else { triggerdn = kb->dnroutine[ix]; }
  if (triggerdn) {
    ((void (*)())triggerdn)();
//    triggerdn(); // no inputs !
  }
}

void clearkey(Keyboard *kb, int keyindex) { // aka keyup()
  // if modifier is DOUBLE-ALT, disable the key .... DOUBLE-ALT-SHIFT to re-enable ? 
  int ix = keyindex; // key remap / cipher table can go here
  kb->pressed[ix % KEYBOARDLEN] = ' ';
  int ki = -1;
  while (++ki < kb->keysdown) {
    if (kb->keys[ki] == ix) { break; }
  } // match keyindex and remove it
  if (ki < kb->keysdown) {
    while (++ki < kb->keysdown) {
      kb->keys[ki - 1] = kb->keys[ki];
    }
    kb->keysdown--;
  }
  if (haskeyfeature(kb, ix, KBMODIFIERKEY))
    { modifierup(kb); } // modifier triggers modify key triggers
  void *triggerup = kb->uproutine[ix];
  if (ismodifierdown(kb, KBSHIFTMODIFIER)) {
    if (ismodifierdown(kb, KBCTRLMODIFIER)) {
      if (ismodifierdown(kb, KBALTMODIFIER)) {
        triggerup = kb->casuproutine[ix];
      } else { triggerup = kb->csuproutine[ix]; }
    } else if (ismodifierdown(kb, KBALTMODIFIER)) {
      triggerup = kb->asuproutine[ix];
    } else { triggerup = kb->suproutine[ix]; }
  } else if (ismodifierdown(kb, KBCTRLMODIFIER)) {
    if (ismodifierdown(kb, KBALTMODIFIER)) {
      triggerup = kb->cauproutine[ix];
    } else { triggerup = kb->cuproutine[ix]; }
  } else if (ismodifierdown(kb, KBALTMODIFIER)) {
    triggerup = kb->auproutine[ix];
  } // else { triggerup = kb->uproutine[ix]; }
  if (triggerup) {
    ((void (*)())triggerup)();
//    triggerup(); // no inputs !
  }
} // mod for safety

string keyboardstatus(Keyboard *kb) {
  int modified = 0;
  string status = "";
  if (ismodifierdown(kb, KBCTRLMODIFIER))
    { modified = 1; status.append("CTRL "); }
  if (ismodifierdown(kb, KBALTMODIFIER))
    { modified = 1; status.append("ALT "); }
  if (ismodifierdown(kb, KBSHIFTMODIFIER))
    { modified = 1; status.append("SHIFT "); }
  if (modified) { status.append("+ "); }
  int ki = -1;
  while (++ki < kb->keysdown) {
    status.append(1, kb->keys[ki]);
  }
  return status;
}

int main(void) {
  pid_t pid1, pid2;
  int ai[2], ao[2];
  int bi[2], bo[2];
  pipe(ai);
  pipe(ao);
  pid1 = fork();
  if (pid1 < 0) { cout << "fork1 fail\n"; FAILEXIT; }
  else if (pid1 == 0) {
    FILE *wstream = fdopen(ao[1], "w");
    if (!wstream) { cout << "wstream1fail\n"; FAILEXIT; }
    close(ai[1]);
    close(ao[0]);
    FILE *stream = fdopen(ai[0], "r");
    if (stream == NULL) { cout << "rstream1fail\n"; FAILEXIT; }
    fputs("[1|INIT]", wstream);
    fflush(wstream);
    int c;
    while (1) {
      c = getc(stream);
      if (c == EOF) {
        cout << "EOF1IN\n";
        break; // so we have stdout but it's different to the pipe back ...
      } else if (c == '*') {
        fputc('#', wstream); // reply #
//        fflush(wstream);
//        cout << "--KEY1MODE--\n"; // can anything be sent to stdout? it does display
        break;
      } else {
        if (c == ' ') { FPUT3C(' ', c, ' ', wstream); }
        else          { FPUT3C('[', c, ']', wstream); }
      }
    }
//    fputs("\n", wstream);
    fflush(wstream);
    FILE *k1 = popen(TESTPATH " " TESTKBD1, "r");
    while (1) {
      int kinch = fgetc(k1);
      if (kinch == EOF) { break; }
      else {
        char kch = (char)kinch;
        fputc(kch, wstream);
        fflush(wstream);
      }
    }
    exit(0); // this is only reached if k1 sends EOF
  } else {
    pipe(bi);
    pipe(bo);
    pid2 = fork();
    if (pid2 < 0) { cout << "fork2 fail\n"; FAILEXIT; }
    else if (pid2 == 0) {
      close(ai[1]);
      close(ai[0]);
      close(ao[1]);
      close(ao[0]);
      FILE *wstream = fdopen(bo[1], "w");
      if (wstream == NULL) { cout << "wstream2fail\n"; FAILEXIT; }
      close(bi[1]);
      close(bo[0]);
      FILE *stream = fdopen(bi[0], "r");
      if (stream == NULL) { cout << "rstream2fail\n"; FAILEXIT; }
      fputs("[2|INIT]", wstream);
//      FPUT3C('{', '2', '}', wstream);
      fflush(wstream);
      int c;
      while (1) {
        c = getc(stream);
        if (c == EOF) {
          cout << "EOF2IN\n";
          break; // so we have stdout but it's different to the pipe back ...
        } else if (c == '*') {
          fputc('#', wstream); // reply #
//          fflush(wstream);
//          cout << "--KEY2MODE--\n";
          break;
        } else {
          if (c == ' ') { FPUT3C(' ', c, ' ', wstream); }
          else          { FPUT3C('{', c, '}', wstream); }
          fflush(wstream);
        }
      }
//      fputs("\n", wstream);
      fflush(wstream);
      FILE *k2 = popen(TESTPATH " " TESTKBD2, "r");
      while (1) {
        int kinch = fgetc(k2);
        if (kinch == EOF) { break; }
        else {
          char kch = (char)kinch;
          fputc(kch, wstream);
          fflush(wstream);
        }
      }
      exit(0);
    }
  }
  close(ai[0]);
  close(bi[0]);
  close(ao[1]);
  close(bo[1]);
  cout << "PREPARING STREAMS...\n";
  FILE *stream1 = fdopen(ai[1], "w");
  if (stream1 == NULL) { cout << "stream1 fail\n"; FAILEXIT; }
  FILE *stream2 = fdopen(bi[1], "w");
  if (stream2 == NULL) { cout << "stream2 fail\n"; FAILEXIT; }
  int flags1 = fcntl(ao[0], F_GETFL, 0);
  if (flags1 == -1) { cout << "flags1 fail\n"; FAILEXIT; }
  else { fcntl(ao[0], F_SETFL, (flags1 |= O_NONBLOCK)); }
  int flags2 = fcntl(bo[0], F_GETFL, 0);
  if (flags2 == -1) { cout << "flags2 fail\n"; FAILEXIT; }
  else { fcntl(bo[0], F_SETFL, (flags2 |= O_NONBLOCK)); }
  FILE *rstream1 = fdopen(ao[0], "r");
  if (!rstream1) { cout << "rstream1 fail\n"; FAILEXIT; }
  FILE *rstream2 = fdopen(bo[0], "r");
  if (!rstream2) { cout << "rstream2 fail\n"; FAILEXIT; }

  cout << "READING STANDARD INPUT...\n";
  enablerawmode();
  fd_set fdselect;
  FD_ZERO(&fdselect);
  int standardkeys = 0;
  int extendedkeys = 0;
  int *keyswitch = &standardkeys;
//  char *keyarray = NULL;
  FILE *instream = NULL;
  Keyboard standardkey;
  Keyboard extendedkey;
  Keyboard *kboard = NULL;
  clearkeyboard(&standardkey);
  clearkeyboard(&extendedkey);
  initstandardkeyboard(&standardkey);
  const char *keyboardbootscript = "{}";
  const char *keyboardbootstate = &keyboardbootscript[0];
  // can only boot a keyboard once, after that it will ignore
  // key input unless the popen is closed and aloop created
  while (1) {
    if (keyswitch == &standardkeys) {
      instream = rstream1;
      kboard = &standardkey;
    } else if (keyswitch == &extendedkeys) {
      instream = rstream2;
      kboard = &extendedkey;
    } else { instream = NULL; } // third pass = neither, stdin
    if (instream) {
      int pinch = fgetc(instream);
      if (pinch == EOF) {
        // an empty buffer -- no input waiting
        cout << "|\n"; // does not erase the rest of the line
        fflush(stdout);
//        usleep(100000); // no delays
      } else {
        if (*keyswitch == 0) { // keyboard is not connected
          int pcount = 128;
          cout << " <";
          while (--pcount >= 0) {
            if (pinch == EOF)  { break; }
            else if (pinch == '#') {
              *keyswitch = 1; // this keyboard is enabled
              cout << " (enabled)";
            } else { cout << (char)pinch; }
            pinch = fgetc(instream);
          }
          cout << ">          \n";
          fflush(stdout);
        } else {
          cout << " [<";
          while (pinch != EOF) {
            if (pinch != 'k') { cout << "nonkeyrelated input\n"; break; }
            if (fgetc(instream) != 'e') { cout << "uncontiguous input\n"; break; }
            if (fgetc(instream) != 'y') { cout << "uncontiguous input\n"; break; }
            if (fgetc(instream) != ' ') { cout << "uncontiguous input\n"; break; }
            int keystate = fgetc(instream);
            if (keystate != 'p' && keystate != 'r') { cout << "unrecognised input\n"; break; }
            else if (keystate == 'p') {
              if (fgetc(instream) != 'r') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 'e') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 's') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 's') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != ' ') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != ' ') { cout << "uncontiguous input\n"; break; }
            } else if (keystate == 'r') {
              if (fgetc(instream) != 'e') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 'l') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 'e') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 'a') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 's') { cout << "uncontiguous input\n"; break; }
              if (fgetc(instream) != 'e') { cout << "uncontiguous input\n"; break; }
            }
            if (fgetc(instream) != ' ') { cout << "uncontrolled input\n"; break; }
            int keyindex = 0;
            int keydigits = 0;
            int digit1 = fgetc(instream);
            if (digit1 == ' ') { cout << "blank input\n"; break; }
            else if (digit1 >= '0' && digit1 <= '9') {
              keyindex = (keyindex * 10) + (digit1 - '0'); // was 0
            } else { cout << "nonnumeric input\n"; break; }
            int digit2 = fgetc(instream);
            if (digit2 == ' ') { keydigits = 1; } // end of input, expect linebreak then more keys maybe
            else if (digit2 >= '0' && digit2 <= '9') {
              keyindex = (keyindex * 10) + (digit2 - '0');
            } else { cout << "nonnumeric second digit\n"; break; }
            if (!keydigits) {
              int digit3 = fgetc(instream);
              if (digit3 == ' ') { keydigits = 2; }
              else if (digit3 >= '0' && digit3 <= '9') {
                keyindex = (keyindex * 10) + (digit3 - '0');
              } else { cout << "nonnumeric third digit\n"; break; }
              if (!keydigits) {
                int digit4 = fgetc(instream);
                if (digit4 == ' ') { keydigits = 3; }
                else if (digit4 >= '0' && digit4 <= '9') {
                  cout << "only 3 digit keypresses are supported\n";
                } else { cout << "unexpected fourth digit\n"; break; }
              }
            }
            if (keydigits) {
              if (keystate == 'r') {
                clearkey(kboard, keyindex);
              } else { presskey(kboard, keyindex); }
              // keydown / keyup event would be processed here
//              if (keystate == 'r') { cout << " key " << keyindex; }
//              else                 { cout << " KEY " << keyindex; }
//              cout << "detected " << ((keystate == 'r') ? "RELEASE" : "PRESS");
//              cout << " of key " << keyindex << "(" << keydigits << " digits)\n";
            }
            if (fgetc(instream) != '\n') { cout << "unlinebroken input\n"; break; }
            pinch = fgetc(instream); // expect EOF or another k
          }
          // see keyboard unconditionally
          seekeyboard(kboard);
          cout << ">]      \n";
//          cout << keyboardstatus(kboard) << "\n";
          fflush(stdout);
        }
      }
    } else { // else instream is NULL so this is stdin's iteration
      int inch = EOF;
      struct timeval refresh = { 1, 0 };
      FD_SET(STDIN_FILENO, &fdselect); // flag yes for checking
      int action = select(1, &fdselect, NULL, NULL, &refresh);
      if (action == -1) { cout << "SELECTFAIL\n"; FAILEXIT; }
      else if (action == 0) {
        if (FD_ISSET(STDIN_FILENO, &fdselect)) {
          cout << "THEREISACHARBUTACTIONWILLRETURN1\n";
          inch = getchar(); // so this wont be called....
        } else { errno = EINTR; } // EOF INTR if timeout reached
      } else { cout << "-"; fflush(stdout); inch = getchar(); } // else at least one char is waiting
      if (inch == EOF && errno == EINTR && *keyboardbootstate)
        { inch = *keyboardbootstate; keyboardbootstate++; } // first timeout EOFs boot the two devices
      if (inch == EOF) { // O_NONBLOCK seemed to cause pipe/child popen issues, hence select()
        if (errno == EINTR) { // || errno == EWOULDBLOCK || errno == EAGAIN) {
          cout << "|\n";
          MOVEUP(3);
          fflush(stdout);
//          continue; // that'll prevent the iterating
        } else { cout << "UNEXPECTEDSTDINEOF\n"; break; }
      } else {
        char ch = (char)inch;
        if (ch == '{') {
          cout << "                  >> {1}\n";
          MOVEUP(3);
          fflush(stdout);
          fprintf(stream1, "KEYBOARD 1 STANDARD*");
          fflush(stream1);
        } else if (ch == '}') {
          cout << "          >> {2}\n"; // sendcmd readkeystate
          MOVEUP(3);
          fflush(stdout);
          fprintf(stream2, "KEYBOARD 1 EXTENDED*");
          fflush(stream2);
        } else if (ch == 3) {
          cout << "Ctrl + C   \n";
          break;
        } else if (ch == 4) {
          cout << "Ctrl + D   \n";
          break;
        } else if (ch == 10) {
          cout << " << \\n   \n";
          MOVEUP(3);
          fflush(stdout);
        } else {
          cout << "<< " << ch << "   \n";
          MOVEUP(3);
          fflush(stdout);
        }
      }
    }
    if (keyswitch == &standardkeys) {
      keyswitch = &extendedkeys;
    } else if (keyswitch == &extendedkeys) {
      keyswitch = NULL;
    } else { keyswitch = &standardkeys; }
  }
  fclose(stream1);
  fclose(rstream1);
  fclose(stream2);
  fclose(rstream2);
  printf("Waiting for PID exits...\n");
  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
  printf("All done!\n");
  return 0;
}









#endif


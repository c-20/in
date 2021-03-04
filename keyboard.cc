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
#include <signal.h>
using namespace std;
#define FAILEXIT      exit(EXIT_FAILURE)
#define OKEXIT        exit(0)
#define MOVEUP(n)     if (n > 0) { printf("\033[%dA", n); }
#define MOVEDOWN(n)   if (n > 0) { printf("\033[%dB", n); }
#define EOLERASE      printf("\033[K")
#define GOTOSOL       printf("\033[1G")
#define EOLSOL        EOLERASE; GOTOSOL // ; // printf("\n"); // GOTOSOL
#define BLANKTOEOL    "\033[K"
#define MOVEUPALINE   "\033[1A"
#define MOVEUP2LINES  "\033[2A"
#define MOVEUP3LINES  "\033[3A"
#define MOVEUP4LINES  "\033[4A"
#define MOVEUP5LINES  "\033[5A"
#define TIMEVAL1S     { 1, 0 }
#define TIMEVAL100MS  { 0, 100000 }
#define TIMEVAL10MS   { 1, 10000 }
#define TIMEVAL1MS    { 0, 1000 }
#define CLOSE2(a, b)        close(a); close(b)
#define CLOSE4(a, b, c, d)  CLOSE2(a, b); CLOSE2(c, d)
#define PIPE2(a, b)         pipe(a); pipe(b)
#define FCLOSE2(a, b)       fclose(a); fclose(b)
#define FCLOSE4(a, b, c, d) FCLOSE2(a, b); FCLOSE2(c, d)
//#define (filep = FDOPEN(filen, fmode) || errmsg)     \
//        ^     ^^^            ^^      ^^^^      ^ SYMBOLS
//                 ^^^^^^ NAME
// SEMICOLON IS NOT A MATCHING SYMBOL AND MAYBE SHOULD BE REQUIRED FOR (); defines
// FILE *var = FDOPEN(desc, "r") || errmsg;
#define FDOPEN(filep, filen, fmode, errmsg)       \
  FILE *filep = fdopen(filen, fmode);             \
  if (!filep) { cout << errmsg << "\n"; FAILEXIT; }
#define FDUNBLOCK(flagvar, fdvar, errmsg)                  \
  int flagvar = fcntl(fdvar, F_GETFL, 0);                  \
  if (flagvar == -1) { cout << errmsg << "\n"; FAILEXIT; } \
  else { fcntl(fdvar, F_SETFL, (flagvar |= O_NONBLOCK)); }
#define FPUT2C(a, b, dest)          fputc(a, dest); fputc(b, dest)
#define FPUT3C(a, b, c, dest)       fputc(a, dest); fputc(b, dest); fputc(c, dest)
#define FPRINTS(target, charstr)    fputs(charstr, target); fflush(target)
#define FPRINTC(target, a)          fputc(a, target); fflush(target)
#define FPRINT2C(target, a, b)      FPUT2C(a, b, target); fflush(target)
#define FPRINT3C(target, a, b, c)   FPUT3C(a, b, c, target); fflush(target)
#define FFS     fflush(stdout)
#define FF(f)   fflush(f)
#define DO      while (1) {
#define DOK     }
#define DFS     }; FFS
#define DFF(f)  }; FF(f)

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
  raw.c_iflag &= ~(IXON); // disable ctrl+s and ctrl+q (pause/resume)
//  raw.c_lflag &= ~(ECHO | ICANON);
//  raw.c_cc[VMIN] = 0;  // min chars to stop waiting
//  raw.c_cc[VTIME] = 1; // a tenth of a second
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
// WHY DOES THIS SEEM TO MAKE CHILD THREADS NOT RESPOND
// WHY DOES THIS BREAK PIPE RETURN
// TERMINAL CAN ONLY HAVE ONE MODE????
}

#define KEYBOARDLEN    256
#define KEYBOARDROWLEN 64    // timebuffer will have 4 rows
#define KBMAXKEYSDOWN  10    // seems to be 6 for numbers, less for letters
#define KBACTIVEKEY    0
#define KBTOGGLEKEY    1
#define KBMODIFIERKEY  2

char keyboardplainkeys[] = {
  '`', '`', '`', '`', '`', '`', '`', '`', '`', '`',
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '`', '`', '`', '`',
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '`', '`', '`', '`',
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '`', '`', '`', '`', '`',
  'z', 'x', 'c', 'v', 'b', 'n', 'm', '`', '`', '`', '`', '`',
  '`', '`', '`', '`', '`', '`', '`', '`', '`', '`', '`', '`'
};
char keyboardshiftkeys[] = { };

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
  void *wdnroutine[KEYBOARDLEN];
  void *wuproutine[KEYBOARDLEN];
  void *swdnroutine[KEYBOARDLEN];
  void *swuproutine[KEYBOARDLEN];
  void *cwdnroutine[KEYBOARDLEN];
  void *cwuproutine[KEYBOARDLEN];
  void *awdnroutine[KEYBOARDLEN];
  void *awuproutine[KEYBOARDLEN];
  void *cswdnroutine[KEYBOARDLEN];
  void *cswuproutine[KEYBOARDLEN];
  void *aswdnroutine[KEYBOARDLEN];
  void *aswuproutine[KEYBOARDLEN];
  void *cawdnroutine[KEYBOARDLEN];
  void *cawuproutine[KEYBOARDLEN];
  void *caswdnroutine[KEYBOARDLEN];
  void *caswuproutine[KEYBOARDLEN];
  struct _Keyboard *next;
} Keyboard;

typedef struct _TimeKeyboard {
  char pressed[KEYBOARDLEN];
  char timeleft[KEYBOARDLEN];
  int winbuffermode;
  struct _TimeKeyboard *next;
} TimeKeyboard;
TimeKeyboard timekeyboard;
struct timeval standardinputtimedelay = TIMEVAL100MS;
// declared here to sync stdin with the timekeyboard

void cleartimekeyboard(TimeKeyboard *tkb) {
  int ix = -1;
  while (++ix < KEYBOARDLEN) {
    tkb->pressed[ix] = ' ';
    tkb->timeleft[ix] = 0;
  }
}

void iteratetimekeyboard(TimeKeyboard *tkb) {
  int ix = -1;
  while (++ix < KEYBOARDLEN) {
    if (tkb->timeleft[ix] > 0) {
      if (--(tkb->timeleft[ix]) <= 0)
        { tkb->pressed[ix] = ' '; }
    }
  }
}

// todo: map mouse left/right clicks to scancode 1/2 (mouse movement in extrabuffers)
// or put mouseclicks in extrabuffers too? both 1 (allmouseclicks) /and/ extrabuffer mousedevice1 leftclick
// TODO: PRESS BUTTON 1 AND 2 AND 4 AND 5 AND 6 FROM MOUSE INPUT AND ADD A MOUSEMOVEDX AND A MOUSEMOVEDY AND A MOUSEMOVED IN KEYBOARD 2

void presstimekey(TimeKeyboard *tkb, int keyindex, struct timeval delay, float presstime) {
  tkb->pressed[keyindex] = '*';
  float delaytime = (float)delay.tv_sec + (((float)delay.tv_usec) / 1000000.0f);
  int delayiterations = (int)(presstime / delaytime);
  tkb->timeleft[keyindex] = delayiterations;
}

void releasetimekey(TimeKeyboard *tkb, int keyindex, struct timeval delay, float releasetime) {
  // if timeleft is more than 0, the press didn't finish. can measure timeleft
  tkb->pressed[keyindex] = '-';
  float delaytime = (float)delay.tv_sec + (((float)delay.tv_usec) / 1000000.0f);
  int delayiterations = (int)(releasetime / delaytime);
  tkb->timeleft[keyindex] = delayiterations;
}

#define KBSHIFTMODIFIER  1
#define KBCTRLMODIFIER   2
#define KBALTMODIFIER    3
#define KBWINMODIFIER    4
int iskbmodifierdown(int kbmodifier, int modifier) {
  return ((kbmodifier & (1 << modifier)) ? 1 : 0);
}
int kbmodifierdownbit(int modifier) {
  return (1 << modifier);
}
// requires kbmodifierdownbit and iskbmodifierdown
#include "timekeymap.h"

void pressscancode(int scancode, int modifier) {
  Scancode scode = { scancode, modifier };
  int hastimekey = scancodetopresstimekey(scode); // triggers presstimekey
}

void releasescancode(int scancode, int modifier) {
  Scancode scode = { scancode, modifier };
  int hastimekey = scancodetoreleasetimekey(scode); // triggers presstimekey
}

void pressasciicode(int asciicode) {
  int hastimekey = asciicodetopresstimekey(asciicode);
}

void releaseasciicode(int asciicode) {
  int hastimekey = asciicodetoreleasetimekey(asciicode);
}
// consider a key that can be both pressed at released at the same time ????

void clearkeyboard(Keyboard *kb) {
  kb->modifier = 0;
  kb->keysdown = 0;
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
    kb->wdnroutine[ix] = NULL;
    kb->wuproutine[ix] = NULL;
    kb->swdnroutine[ix] = NULL;
    kb->swuproutine[ix] = NULL;
    kb->cwdnroutine[ix] = NULL;
    kb->cwuproutine[ix] = NULL;
    kb->awdnroutine[ix] = NULL;
    kb->awuproutine[ix] = NULL;
    kb->cswdnroutine[ix] = NULL;
    kb->cswuproutine[ix] = NULL;
    kb->aswdnroutine[ix] = NULL;
    kb->aswuproutine[ix] = NULL;
    kb->cawdnroutine[ix] = NULL;
    kb->cawuproutine[ix] = NULL;
    kb->caswdnroutine[ix] = NULL;
    kb->caswuproutine[ix] = NULL;
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

// modifier indices needed earlier by timekeyboard
#define KBLSHIFTKEY      LEFTSHIFTSCANCODE  // 50
#define KBRSHIFTKEY      RIGHTSHIFTSCANCODE // 62
#define KBLCTRLKEY       LEFTCTRLSCANCODE   // 37
#define KBRCTRLKEY       RIGHTCTRLSCANCODE  // 105
#define KBLALTKEY        LEFTALTSCANCODE    // 64
#define KBRALTKEY        RIGHTALTSCANCODE   // 108
#define KBLWINKEY        LEFTWINSCANCODE    // 133
#define KBRWINKEY        RIGHTWINSCANCODE   // 134

//#define fn doesn't map to a key
#define KBNUMBER1S  NUMBER1SCANCODE
#define KBNUMBER1T  NUMBER0SCANCODE
#define KBLETTER1S  LETTERQSCANCODE
#define KBLETTER1T  LETTERPSCANCODE
#define KBLETTER2S  LETTERASCANCODE
#define KBLETTER2T  LETTERLSCANCODE
#define KBLETTER3S  LETTERZSCANCODE
#define KBLETTER3T  LETTERMSCANCODE

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

void seekeyboard2(Keyboard *kb1, Keyboard *kb2) {
  int ix = -1;
  while (++ix < KEYBOARDLEN) {
    char kbch = '~';    // inactive keys have ~ for now
    int isactive = 0;
    if (haskeyfeature(kb1, ix, KBACTIVEKEY)) {
      kbch = (char)(kb1->pressed[ix]);
      isactive++;
    }
    if (haskeyfeature(kb2, ix, KBACTIVEKEY)) {
      kbch = (char)(kb2->pressed[ix]);
      isactive++;
    }
    if (isactive > 1) { kbch = '!'; } // double-active!
    cout << kbch; // visible active key
  }
}

void modifierdn(Keyboard *kb) { // should only become pressed if active
// todo: hold both alt keys for delete-key modifier
  int shiftbit = (0 + ((kb->pressed[KBLSHIFTKEY] != ' ') || (kb->pressed[KBRSHIFTKEY] != ' ')) ? (1 << KBSHIFTMODIFIER) : 0);
  int ctrlbit =  (0 +  (kb->pressed[KBLCTRLKEY]  != ' ')                                       ? (1 << KBCTRLMODIFIER)  : 0);
  int altbit =   (0 + ((kb->pressed[KBLALTKEY]   != ' ') || (kb->pressed[KBRALTKEY]   != ' ')) ? (1 << KBALTMODIFIER)   : 0);
  int winbit =   (0 +  (kb->pressed[KBLWINKEY]   != ' ')                                       ? (1 << KBWINMODIFIER)   : 0);
  int newmodifier = shiftbit + ctrlbit + altbit + winbit;
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
  } // activate key if inactive !!!!!!!!!!!!!
  kb->pressed[ix % KEYBOARDLEN] = '*';
  pressscancode(ix, kb->modifier);
//  presstimekey(&timekeyboard, ix, standardinputtimedelay, 1.0f);
  if (!haskeyfeature(kb, ix, KBMODIFIERKEY)) {
    int thiskey = keyboardplainkeys[ix]; // todo: SHIFT
    if (thiskey != '`') {
      kb->keys[kb->keysdown] = thiskey; // ix;
      if (kb->keysdown++ >= KBMAXKEYSDOWN)
        { cout << "too many keys\n"; kb->keysdown--; }
    } // exclude modifiers from key list
  }
  if (haskeyfeature(kb, ix, KBMODIFIERKEY))
    { modifierdn(kb); } // modifier triggers modify key triggers
  void *triggerdn = kb->dnroutine[ix];
  if (ismodifierdown(kb, KBSHIFTMODIFIER)) {
    if (ismodifierdown(kb, KBCTRLMODIFIER)) {
      if (ismodifierdown(kb, KBALTMODIFIER)) {
        if (ismodifierdown(kb, KBWINMODIFIER)) {
          triggerdn = kb->caswdnroutine[ix];            // casw
        } else { triggerdn = kb->casdnroutine[ix]; }    // cas
      } else if (ismodifierdown(kb, KBWINMODIFIER)) {
        triggerdn = kb->cswdnroutine[ix];               // csw
      } else { triggerdn = kb->csdnroutine[ix]; }       // cs
    } else if (ismodifierdown(kb, KBALTMODIFIER)) {
      if (ismodifierdown(kb, KBWINMODIFIER)) {
        triggerdn = kb->aswdnroutine[ix];               // asw
      } else { triggerdn = kb->asdnroutine[ix]; }       // as
    } else if (ismodifierdown(kb, KBWINMODIFIER)) {
      triggerdn = kb->swdnroutine[ix];                  // sw
    } else { triggerdn = kb->sdnroutine[ix]; }          // s
  } else if (ismodifierdown(kb, KBCTRLMODIFIER)) {
    if (ismodifierdown(kb, KBALTMODIFIER)) {
      if (ismodifierdown(kb, KBWINMODIFIER)) {
        triggerdn = kb->cawdnroutine[ix];               // caw
      } else { triggerdn = kb->cadnroutine[ix]; }       // ca
    } else if (ismodifierdown(kb, KBWINMODIFIER)) {
      triggerdn = kb->cwdnroutine[ix];                  // cw
    } else { triggerdn = kb->cdnroutine[ix]; }          // c
  } else if (ismodifierdown(kb, KBALTMODIFIER)) {
    if (ismodifierdown(kb, KBWINMODIFIER)) {
      triggerdn = kb->awdnroutine[ix];                  // aw
    } else { triggerdn = kb->adnroutine[ix]; }          // a
  } else if (ismodifierdown(kb, KBWINMODIFIER)) {
    triggerdn = kb->wdnroutine[ix];                     // w
  } // else { triggerdn = kb->dnroutine[ix]; }
  if (triggerdn) {
    ((void (*)())triggerdn)();
//    triggerdn(); // no inputs !
  }
}

void clearkey(Keyboard *kb, int keyindex) { // aka keyup()
  // if modifier is DOUBLE-ALT, disable the key .... DOUBLE-ALT-SHIFT to re-enable ? 
  int ix = keyindex; // key remap / cipher table can go here
  kb->pressed[ix % KEYBOARDLEN] = ' '; // loopback overwrite for safety
  // currently no releasescancode() function ..
//  releasetimekey(&timekeyboard, ix, standardinputtimedelay, 1.0f);
  releasescancode(ix, kb->modifier);
  int ki = -1;
  int thiskey = keyboardplainkeys[ix];
  if (thiskey != '`') {
    while (++ki < kb->keysdown) {
      if (kb->keys[ki] == thiskey) { break; }
    } // match keyindex and remove it
    if (ki < kb->keysdown) {
      while (++ki < kb->keysdown) {
        kb->keys[ki - 1] = kb->keys[ki];
      }
      kb->keysdown--;
    }
  }
  if (haskeyfeature(kb, ix, KBMODIFIERKEY))
    { modifierup(kb); } // modifier triggers modify key triggers
  void *triggerup = kb->uproutine[ix];
  if (ismodifierdown(kb, KBSHIFTMODIFIER)) {
    if (ismodifierdown(kb, KBCTRLMODIFIER)) {
      if (ismodifierdown(kb, KBALTMODIFIER)) {
        if (ismodifierdown(kb, KBWINMODIFIER)) {
          triggerup = kb->caswuproutine[ix];            // casw
        } else { triggerup = kb->casuproutine[ix]; }    // cas
      } else if (ismodifierdown(kb, KBWINMODIFIER)) {
        triggerup = kb->cswuproutine[ix];               // csw
      } else { triggerup = kb->csuproutine[ix]; }       // cs
    } else if (ismodifierdown(kb, KBALTMODIFIER)) {
      if (ismodifierdown(kb, KBWINMODIFIER)) {
        triggerup = kb->aswuproutine[ix];               // asw
      } else { triggerup = kb->asuproutine[ix]; }       // as
    } else if (ismodifierdown(kb, KBWINMODIFIER)) {
      triggerup = kb->swuproutine[ix];                  // sw
    } else { triggerup = kb->suproutine[ix]; }          // s
  } else if (ismodifierdown(kb, KBCTRLMODIFIER)) {
    if (ismodifierdown(kb, KBALTMODIFIER)) {
      if (ismodifierdown(kb, KBWINMODIFIER)) {
        triggerup = kb->cawuproutine[ix];               // caw
      } else { triggerup = kb->cauproutine[ix]; }       // ca
    } else if (ismodifierdown(kb, KBWINMODIFIER)) {
      triggerup = kb->cwuproutine[ix];                  // cw
    } else { triggerup = kb->cuproutine[ix]; }          // c
  } else if (ismodifierdown(kb, KBALTMODIFIER)) {
    if (ismodifierdown(kb, KBWINMODIFIER)) {
      triggerup = kb->awuproutine[ix];                  // aw
    } else { triggerup = kb->auproutine[ix]; }          // a
  } else if (ismodifierdown(kb, KBWINMODIFIER)) {
    triggerup = kb->wuproutine[ix];                     // w
  } // else { triggerup = kb->uproutine[ix]; }
  if (triggerup) {
    ((void (*)())triggerup)();
//    triggerup(); // no inputs !
  }
} // mod for safety

string keyboard1status(Keyboard *kb) {
  int modified = 0;
  string status = "";
  if (ismodifierdown(kb, KBCTRLMODIFIER))
    { modified = 1; status.append("CTRL "); }
  if (ismodifierdown(kb, KBALTMODIFIER))
    { modified = 1; status.append("ALT "); }
  if (ismodifierdown(kb, KBSHIFTMODIFIER))
    { modified = 1; status.append("SHIFT "); }
  if (ismodifierdown(kb, KBWINMODIFIER))
    { modified = 1; status.append("WIN "); }
  if (modified && kb->keysdown) { status.append("+ "); }
  int ki = -1;
  while (++ki < kb->keysdown) {
    status.append(1, kb->keys[ki]);
  }
  return status;
}

string keyboard2status(Keyboard *kb) {
  string keys = "";
  string meaning = "";
  int ki = -1;
  while (++ki < KEYBOARDLEN) {
    if (kb->pressed[ki] == '*') {
      if (keys.length())    { keys.append(", ");    }
      if (meaning.length()) { meaning.append(", "); }
      // will have excess commas only after first match
      keys.append(to_string(ki));
      if      (ki == 121) { meaning.append("VOLUME MUTE TOGGLE"); }
      else if (ki == 122) { meaning.append("VOLUME DOWN");        }
      else if (ki == 123) { meaning.append("VOLUME UP");          }
      else if (ki == 173) { meaning.append("PREVIOUS TRACK");     }
      else if (ki == 172) { meaning.append("PLAY / PAUSE");       }
      else if (ki == 171) { meaning.append("NEXT TRACK");         }
      else { meaning.append("?"); }
    }
  }
  if (meaning.length()) { keys.append(" (").append(meaning).append(")"); }
  return keys;
}

string timekeyboardstatus(TimeKeyboard *tkb) {
  string pressed = "";
  string released = "";
  int tki = -1;
  while (++tki < KEYBOARDLEN) {
    if (tkb->pressed[tki] == '*') {
      if (pressed.length()) { pressed.append(1, ' '); }
      pressed.append(to_string(tki));
    } else if (tkb->pressed[tki] == '-') {
      if (released.length()) { released.append(1, ' '); }
      released.append(to_string(tki));
    }
  }
  return pressed.append("][").append(released);
}

string timekeyboardbuffer(TimeKeyboard *tkb) {
  string status = "";
  int tki = -1;
  while (++tki < KEYBOARDLEN) {
    if      (tkb->pressed[tki] == '*')   { status.append("*");  }
    else if (tkb->pressed[tki] == '-')   { status.append("-");  }
    else                                 { status.append(" ");  }
    if ((tki + 1) % KEYBOARDROWLEN == 0) { status.append("\n"); }
  }
  return status;
}

string keyboardstatus2(Keyboard *kb1, Keyboard *kb2) {
  string kb1status = keyboard1status(kb1);
  string kb2status = keyboard2status(kb2);
  string kbstatus = "";
  if (kb2status.length()) {
    if (kb1status.length()) {
      kbstatus.append(" [").append(kb1status);
      kbstatus.append("] [");
      kbstatus.append(kb2status).append("]");
    } else { kbstatus.append(" [").append(kb2status).append("]"); }
  } else { kbstatus.append(" [").append(kb1status).append("]"); }
  string timekbstatus = timekeyboardstatus(&timekeyboard);
  if (timekbstatus.length())
    { kbstatus.append(" [").append(timekbstatus).append("]"); }
  return kbstatus;
}

string keyboardstatus3(Keyboard *kb1, Keyboard *kb2, int inch, string keyname) {
  string kbstatus = "";
  if (inch == -1) { // catch EOF
    cout << "!!KBEOF"; FFS; // not expected .....??? or maybe it is........TODO
    return ""; // blank status for EOF ! no message ! no FFS !
  } else { // stdin keys get added to timebuffer upon status check
    char ch = (char)inch; // stdin keys are time-pressed but not time-released !
    pressasciicode(ch); // stdin keys are not currently released
    if (timekeyboard.winbuffermode) {
      kbstatus.append(keyboardstatus2(kb1, kb2)).append(BLANKTOEOL).append("\n");
      string winkbstatus = timekeyboardbuffer(&timekeyboard);
      kbstatus.append(">>").append(winkbstatus).append("<<").append(MOVEUP5LINES);
    } else { kbstatus.append(keyboardstatus2(kb1, kb2)); }
  }
  return kbstatus;
}

#define ALTWINSYNC    199
void altwinsyncdn() {
  presstimekey(&timekeyboard, ALTWINSYNC, standardinputtimedelay, 1.0f);
// consider howto: add parameters to ((*)()triggerdn)();
// if hasfeature TIMEKEY ((*)(TimeKeyboard *, struct timeval)triggerdn)(&timekeyboard, standardinputtimedelay);
}
void altwinsyncup() {
  releasetimekey(&timekeyboard, ALTWINSYNC, standardinputtimedelay, 1.0f);
}

//#define SIMULATETILDEKEY    system("xdotool key shift+49")
//#define SIMULATEGRAVEKEY    system("xdotool key 49")
// shift+49 leaves SHIFT ENABLED AFTER THE PROGRAM EXITS
//#define SIMULATETILDEKEY    system("xdotool key asciitilde")
//#define SIMULATEGRAVEKEY    system("xdotool key grave")
// so does this ...

int main(void) {
  signal(SIGTSTP, SIG_IGN); // disable Ctrl+Z
  signal(SIGINT, SIG_IGN);  // disable Ctrl+C
  int ai[2], ao[2], bi[2], bo[2]; // bi/bo defined before second fork
  PIPE2(ai, ao); // ai/ao pipe in both threads, thread 2 closes it
  pid_t pid2 = -1, pid1 = fork(); // maybe without, the same descriptors could be repeated
  if (pid1 < 0) { cout << "fork1 fail\n"; FAILEXIT; }
  else if (pid1 == 0) {
    FDOPEN(wstream, ao[1], "w", "wstream1fail");
    CLOSE2(ai[1], ao[0]);
    FDOPEN(rstream, ai[0], "r", "rstream1fail");
    FPRINTS(wstream, "[1|INIT]");
    DO int c = fgetc(rstream);
       if      (c == EOF) { cout << "EOF1IN\n";  break;   } // not expected because not non-blocking
       else if (c == '*') { fputc('#', wstream); break;   } // << *, >> # enables keyboard 1
       else if (c == ' ') { FPUT3C(' ', c, ' ', wstream); } // spaces are spaced
       else               { FPUT3C('[', c, ']', wstream); } DFF(wstream);
    FILE *k1 = popen(TESTPATH " " TESTKBD1, "r");
    DO int kinch = fgetc(k1);
       if (kinch == EOF) { break; }
       else {
         char kch = (char)kinch;
         fputc(kch, wstream);
         fflush(wstream);
       } DOK;
    OKEXIT; // this is only reached if k1 sends EOF
  } else {
    PIPE2(bi, bo);
    pid2 = fork();
    if (pid2 < 0) { cout << "fork2 fail\n"; FAILEXIT; }
    else if (pid2 == 0) {
      CLOSE4(ai[1], ai[0], ao[1], ao[0]);
      FDOPEN(wstream, bo[1], "w", "wstream2fail");
      CLOSE2(bi[1], bo[0]);
      FDOPEN(rstream, bi[0], "r", "rstream2fail");
      FPRINTS(wstream, "[2|INIT]");
      DO int c = fgetc(rstream);
         if      (c == EOF) { cout << "EOF2IN\n";  break;   } // not expected because not non-blocking
         else if (c == '*') { fputc('#', wstream); break;   } // << *, >> # enables keyboard 1
         else if (c == ' ') { FPUT3C(' ', c, ' ', wstream); } // spaces are spaced
         else               { FPUT3C('{', c, '}', wstream); } DFF(wstream);
      FILE *k2 = popen(TESTPATH " " TESTKBD2, "r");
      DO int kinch = fgetc(k2);
         if (kinch == EOF) { break; }
         else {
           char kch = (char)kinch;
           fputc(kch, wstream);
           fflush(wstream);
         } DOK;
      OKEXIT;
    }
  }
  CLOSE4(ai[0], bi[0], ao[1], bo[1]);
  FDOPEN(mwstream1, ai[1], "w", "mwstream1 fail");
  FDOPEN(mwstream2, bi[1], "w", "mwstream2 fail");
  FDUNBLOCK(flags1, ao[0], "mrstream1 unblockfail");
  FDUNBLOCK(flags2, bo[0], "mrstream2 unblockfail");
  FDOPEN(mrstream1, ao[0], "r", "mrstream1 fail");
  FDOPEN(mrstream2, bo[0], "r", "mrstream2 fail");
  enablerawmode();
  fd_set fdselect;
  FD_ZERO(&fdselect);
  int keyboardlogs = 0;
  int standardkeys = 0;
  int extendedkeys = 0;
  int *keyswitch = &standardkeys;
  FILE *instream = NULL;
  Keyboard standardkey;
  Keyboard extendedkey;
  Keyboard *kboard = NULL;
  clearkeyboard(&standardkey);
  clearkeyboard(&extendedkey);
  cleartimekeyboard(&timekeyboard); // todo: set winbuffermode to 0 !!!!
  timekeyboard.winbuffermode = 0; // toggled in script
  initstandardkeyboard(&standardkey);
  standardkey.awdnroutine[ALTWINSYNC] = (void *)altwinsyncdn;
  standardkey.awuproutine[ALTWINSYNC] = (void *)altwinsyncup;
  const char *keyboardbootscript = "|{}"; // enable winbuffer, boot kb1 kb2
  const char *keyboardbootstate = &keyboardbootscript[0];
  // can only boot a keyboard once, after that it will ignore
  // key input unless the popen is closed and a loop created
  if (keyboardlogs) { cout << "\n" << "\n"; } FFS;
  int uplines = 0;
  while (1) {
    if (keyswitch == &standardkeys)
      { instream = mrstream1; uplines = 2; kboard = &standardkey; }
    else if (keyswitch == &extendedkeys)
      { instream = mrstream2; uplines = 1; kboard = &extendedkey; }
    else { instream = NULL; } // third pass = neither, stdin
    if (instream) {
      int pinch = fgetc(instream);
      if (pinch == EOF) { // no input waiting
        if (keyboardlogs) { // doesn't happen because of select() trigger now
          MOVEUP(uplines); // move to buffer line
          cout << "|\n"; // does not erase the rest of the line
          MOVEDOWN(uplines - 1); // move to buffer line
          fflush(stdout);
        }
      } else {
        if (keyboardlogs) { MOVEUP(uplines); } // move to buffer line
        if (*keyswitch == 0) { // keyboard is not connected
          int pcount = 128;
          if (keyboardlogs) { cout << " <"; }
          while (--pcount >= 0) {
            if (pinch == EOF)  { break; }
            else if (pinch == '#') {
              *keyswitch = 1; // this keyboard is enabled
              if (keyboardlogs) { cout << " (enabled)"; }
            } else if (keyboardlogs) { cout << (char)pinch; }
            pinch = fgetc(instream);
            int maxeofs = 50;
            while (pinch == EOF) { // erroneous EOF still displays!
              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (--maxeofs <= 0) { cout << "1EOF"; break; } // sometimes there is no input waiting
                if (keyboardlogs) { cout << "."; } FFS;
                pinch = fgetc(instream);
              } else { cout << "!!1EOF"; break; }
            }
          }
          if (keyboardlogs) { cout << ">"; EOLSOL; fflush(stdout); }
        } else {
          if (keyboardlogs) { cout << " [<"; }
          char linebuffer[128];
          int linebufferi = 0;
          linebuffer[linebufferi] = '\0';
          while (pinch != '\n') { // first pinch can't be EOF or it would have flagged as empty buffer
            linebuffer[linebufferi] = pinch;
            linebuffer[++linebufferi] = '\0';
            pinch = fgetc(instream);
            while (pinch == EOF) { // no maxeofs to prevent half packets in delay situations
              if (errno == EAGAIN || errno == EWOULDBLOCK) { // only non-blocking-EOF is ok
//                if (--maxeofs <= 0) { cout << "DATAEOF"; break; } // not expected, input should be waiting
                if (keyboardlogs) { cout << "."; } FFS; // does FFS cause select() in main thread to be ready with an EOF?
                pinch = fgetc(instream);
              } else { cout << "[!!EOF]"; break; }
            }
          }
          if (linebuffer[0]  != 'k')                         { cout << "k-match fail\n"; }
          if (linebuffer[4]  != 'p' && linebuffer[4] != 'r') { cout << "pr-match fail\n"; }
          if (linebuffer[11] != ' ')                         { cout << "space-match fail\n"; }
          int keyindex = 0;
          int keydigits = 0;
          char keystate = linebuffer[4]; // 'p' or 'r'
          int digit1 = linebuffer[12]; // fgetc(instream);
          if (digit1 == ' ') { cout << "blank input\n"; break; }
          else if (digit1 >= '0' && digit1 <= '9') {
            keyindex = (keyindex * 10) + (digit1 - '0'); // was 0
          } else { cout << "nonnumeric input\n"; break; }
          int digit2 = linebuffer[13]; // fgetc(instream);
          if (digit2 == ' ') { keydigits = 1; } // end of input, expect linebreak then more keys maybe
          else if (digit2 >= '0' && digit2 <= '9') {
            keyindex = (keyindex * 10) + (digit2 - '0');
          } else { cout << "nonnumeric second digit\n"; break; }
          if (!keydigits) {
            int digit3 = linebuffer[14]; // fgetc(instream);
            if (digit3 == ' ') { keydigits = 2; }
            else if (digit3 >= '0' && digit3 <= '9') {
              keyindex = (keyindex * 10) + (digit3 - '0');
            } else { cout << "nonnumeric third digit\n"; break; }
            if (!keydigits) {
              int digit4 = linebuffer[15]; // fgetc(instream);
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
          }
          if (keyboardlogs) { seekeyboard(kboard); }
          if (keyboardlogs) { cout << ">]"; EOLSOL; FFS; }
        }
        if (keyboardlogs) { MOVEDOWN(uplines); } FFS; // move to buffer line
      }
    } // else instream is NULL (no input buffer reading to do)
    if (1) { // actually, read stdin after each buffer read, not both buffer reads
      int inch = EOF; // show keystatus after reading keybuffer
      if (keyswitch) { inch = EOF; errno = EINTR; keyswitch = NULL; }
      else {
        struct timeval refresh = standardinputtimedelay;
        FD_SET(STDIN_FILENO, &fdselect); // flag yes for checking
        FD_SET(ao[0], &fdselect); // also watch standard keyboard
        FD_SET(bo[0], &fdselect); // also watch extended keyboard
        int pipemax = (ao[0] > bo[0]) ? ao[0] : bo[0];
        int fdmax = ((STDIN_FILENO > pipemax) ? STDIN_FILENO : pipemax) + 1;
        int action = select(fdmax, &fdselect, NULL, NULL, &refresh);
        int stdinpending = FD_ISSET(STDIN_FILENO, &fdselect);
        if (action == -1) { cout << "SELECTFAIL\n"; FAILEXIT; }
        else if (action > 0 && !stdinpending) {
          if (FD_ISSET(ao[0], &fdselect)) {
            inch = EOF; errno = EACCES; keyswitch = &standardkeys;
            if (keyboardlogs) { cout << "1-"; } FFS; continue;
          } else if (FD_ISSET(bo[0], &fdselect)) {
            inch = EOF; errno = EACCES; keyswitch = &extendedkeys;
            if (keyboardlogs) { cout << "2-"; } FFS; continue;
          } else { cout << "INVALIDFILEDESCRIPTOR\n"; FAILEXIT; }
        } else if (action == 0) { errno = EINTR; } // no action
        else if (action > 0 && stdinpending) {
          if (keyboardlogs) { cout << "0-"; fflush(stdout); }
          inch = getchar();
          errno = 0; // make sure it is reset (it probably is) in case of script | check
        } else { cout << "SELECTFAIL\n"; FAILEXIT; }
        if (inch == EOF && errno == EINTR && *keyboardbootstate)
          { inch = *keyboardbootstate; keyboardbootstate++; } // first timeout EOFs boot the two devices
      }
      if (inch == EOF) { // O_NONBLOCK seemed to cause pipe/child popen issues, hence select()
        if (errno == EINTR) { // || errno == EWOULDBLOCK || errno == EAGAIN) {
          iteratetimekeyboard(&timekeyboard);
          cout << keyboardstatus3(&standardkey, &extendedkey, 0, "++T");
          FFS; // continue; did prevent buffer switching until fds were added to select()
        } else if (errno == EACCES) { cout << "CONTINUE\n"; continue; }
        else { cout << "UNEXPECTEDSTDINEOF\n"; break; }
      } else {
        char ch = (char)inch;
        if        (ch == '{') { // { is not sent as a stdin key !!!!!
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, "}"); FFS;
          if (!standardkeys) { FPRINTS(mwstream1, "KEYBOARD 1 STANDARD*"); }
        } else if (ch == '}') {
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, "}"); FFS;
          if (!extendedkeys) { FPRINTS(mwstream2, "KEYBOARD 1 EXTENDED*"); }
        } else if (ch == '|') { // ctrl+| or | in keyboard boot script for winbuffermode toggle
          int inkeyboardbootstate = (errno == EINTR) ? 1 : 0;
          if (ismodifierdown(&standardkey, KBCTRLMODIFIER) || inkeyboardbootstate)
            { timekeyboard.winbuffermode = (timekeyboard.winbuffermode) ? 0 : 1; }
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, "|"); FFS;
        } else if (ch == 27) {
          cout << "ESC \n"; EOLSOL; FFS;
          break;
        } else if (ch == 3 || ch == 26) { // also 11 (VTAB) and 12 (FFEED) suppressed
          cout << "Ctrl + C and Ctrl + Z do not generate a ctrlchar code after signal is disabled\n";
          break; // should not occur unless signals are re-enabled !
        } else if (ch == 10 || ch == 13) { // LINEBREAK (^J) or CARRIAGE RETURN (^M)
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, "ENTER"); FFS;
        } else if (ch < 32) { // matches 11 (VTAB) and 12 (FFEED)
          char ctrlchar = (char)(ch - 1) + 'A'; // why offset by -1??
          string ctrlname = "CTRL ";
          ctrlname.append(1, ctrlchar);
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, ctrlname); FFS;
        } else if (ch == '~') {
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, "TILDE"); FFS;
        } else if (ch == '`') {
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, "GRAVE"); FFS;
        } else {
          string keyname = "";
          keyname.append(1, ch);
          cout << keyboardstatus3(&standardkey, &extendedkey, ch, keyname); FFS;
        }
      }
    } // keyswitch was iterated here but now select() checks pipe fds
  }
  FCLOSE4(mwstream1, mrstream1, mwstream2, mrstream2);
  CLOSE4(ai[1], bi[1], ao[0], bo[0]);
  kill(pid1, SIGHUP);
  kill(pid2, SIGHUP);
  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
  return 0;
}
#endif


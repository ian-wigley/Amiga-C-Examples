// C conversion of AmigaVikke copperbars example

#include "support/gcc8_c_support.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <exec/execbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

struct ExecBase *SysBase;
volatile struct Custom *hw;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

//backup
static UWORD SystemInts;
static UWORD SystemDMA;
static UWORD SystemADKCON;
volatile static APTR VBR = 0;
static APTR SystemIrq;

struct View *ActiView;

void UpdateCopper(int frame);

INCBIN(amigavikke, "data/amigavikke.raw")

UWORD sin255_60[256] = { 30,31,31,32,33,34,34,35,36,37,37,38,39,39,40,41,42,42,43,44,44,45,45,46,47,47,48,49,49,50,50,51,51,52,52,53,53,54,54,55,55,55,56,56,57,57,57,57,58,58,58,59,59,59,59,59,59,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,59,59,59,59,59,58,58,58,58,57,57,57,56,56,56,55,55,54,54,53,53,53,52,52,51,50,50,49,49,48,48,47,46,46,45,45,44,43,43,42,41,40,40,39,38,38,37,36,36,35,34,33,33,32,31,30,30,29,28,27,27,26,25,24,24,23,22,22,21,20,20,19,18,17,17,16,15,15,14,14,13,12,12,11,11,10,10,9,8,8,7,7,7,6,6,5,5,4,4,4,3,3,3,2,2,2,2,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,3,3,3,3,4,4,5,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,13,13,14,15,15,16,16,17,18,18,19,20,21,21,22,23,23,24,25,26,26,27,28,29,29,30 };
UWORD cos255[256] = { 255,255,255,255,254,254,254,253,253,252,251,250,249,249,248,246,245,244,243,241,240,238,237,235,233,232,230,228,226,224,222,220,218,215,213,211,208,206,203,201,198,196,193,190,187,185,182,179,176,173,170,167,164,161,158,155,152,149,146,143,140,137,133,130,127,124,121,118,115,112,109,105,102,99,96,93,90,87,84,81,78,76,73,70,67,65,62,59,57,54,51,49,47,44,42,40,37,35,33,31,29,27,25,23,22,20,18,17,15,14,13,11,10,9,8,7,6,5,4,4,3,3,2,2,1,1,1,1,1,1,1,1,2,2,3,3,4,4,5,6,7,8,9,10,11,13,14,15,17,18,20,22,23,25,27,29,31,33,35,37,40,42,44,47,49,51,54,57,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,109,112,115,118,121,124,127,130,133,137,140,143,146,149,152,155,158,161,164,167,170,173,176,179,182,185,187,190,193,196,198,201,203,206,208,211,213,215,218,220,222,224,226,228,230,232,233,235,237,238,240,241,243,244,245,246,248,249,249,250,251,252,253,253,254,254,254,255,255,255,255 };
UWORD cbars[] = {
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,// Black
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0, 0,
        0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0x900, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00,//; red - 32w
        0xf00, 0xe00, 0xd00, 0xc00, 0xb00, 0xa00, 0x900, 0x800, 0x700, 0x600, 0x500, 0x400, 0x300, 0x200, 0x100, 0, 0,
        0x010, 0x020, 0x030, 0x040, 0x050, 0x060, 0x070, 0x080, 0x090, 0x0a0, 0x0b0, 0x0c0, 0x0d0, 0x0e0, 0x0f0,//; grn - 32w
        0x0f0, 0x0e0, 0x0d0, 0x0c0, 0x0b0, 0x0a0, 0x090, 0x080, 0x070, 0x060, 0x050, 0x040, 0x030, 0x020, 0x010, 0, 0,
        0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x00a, 0x00b, 0x00c, 0x00d, 0x00e, 0x00f,//; blu - 32w
        0x00f, 0x00e, 0x00d, 0x00c, 0x00b, 0x00a, 0x009, 0x008, 0x007, 0x006, 0x005, 0x004, 0x003, 0x002, 0x001, 0, 0,
};

volatile UWORD cbar_y[] = { 0, 0, 0, 0 };      // y - position
volatile UWORD cbar_z[] = { 0, 1, 2, 0 };      // z - position(depth, for calculating order of display)
volatile UWORD cbar_a[] = { 0, 85, 170, 0 };   // angle - 0, 85, 170 = evenly distributed on 255 bitgrad
volatile int copper;

volatile UWORD *a0 = 0;
volatile UWORD *a1 = 0;
volatile UWORD *a2 = 0;
volatile UWORD *a3 = 0;
volatile UWORD *a4 = 0;
volatile UWORD* a6 = 0;

volatile UWORD d0 = 0;
volatile UWORD d1 = 0;
volatile UWORD d2 = 0;
volatile UWORD d3 = 0;
volatile UWORD d4 = 0;
volatile UWORD d5 = 0;
volatile UWORD d6 = 0;
volatile UWORD d7 = 0;

volatile static __attribute__((section("bitmap.MEMF_CHIP"))) UWORD bitmap [6000] = {};
volatile static __attribute__((section("bitmap.MEMF_CHIP"))) UWORD bpl0[4000] = {};
volatile static __attribute__((section("bitmap.MEMF_CHIP"))) UWORD bpl1[4000] = {};
volatile static __attribute__((section("copper.MEMF_CHIP"))) UWORD copper1[1024] = {};
volatile static __attribute__((section("copper.MEMF_CHIP"))) UWORD copper2[1024] = {};
volatile static __attribute__((section("copper.MEMF_CHIP"))) UWORD copperlines1[1000] = {};

volatile UWORD cbar_mode = 0;                  //  0 = normal, 1 = additive
volatile UWORD anglespeed = 2;
volatile UWORD cbar_start_default = 60;        //  startline default
volatile UWORD cbar_start = 0;                 //  calculated startline

static APTR GetVBR(void)
{
    APTR vbr = 0;
    UWORD getvbr[] = {0x4e7a, 0x0801, 0x4e73}; // MOVEC.L VBR,D0 RTE

    if (SysBase->AttnFlags & AFF_68010)
        vbr = (APTR)Supervisor((void *)getvbr);

    return vbr;
}

void SetInterruptHandler(APTR interrupt)
{
    *(volatile APTR *)(((UBYTE *)VBR) + 0x6c) = interrupt;
}

APTR GetInterruptHandler()
{
    return *(volatile APTR *)(((UBYTE *)VBR) + 0x6c);
}

//vblank begins at vpos 312 hpos 1 and ends at vpos 25 hpos 1
//vsync begins at line 2 hpos 132 and ends at vpos 5 hpos 18
void WaitVbl()
{
    while (1)
    {
        volatile ULONG vpos = *(volatile ULONG *)0xDFF004;
        vpos &= 0x1ff00;
        if (vpos != (311 << 8))
            break;
    }
    while (1)
    {
        volatile ULONG vpos = *(volatile ULONG *)0xDFF004;
        vpos &= 0x1ff00;
        if (vpos == (311 << 8))
            break;
    }
}

inline void WaitBlt()
{
    UWORD tst = *(volatile UWORD *)&hw->dmaconr; //for compatiblity a1000
    (void)tst;
    while (*(volatile UWORD *)&hw->dmaconr & (1 << 14))
    {
    } //blitter busy wait
}

void TakeSystem()
{
    ActiView = GfxBase->ActiView; //store current view
    OwnBlitter();
    WaitBlit();
    Disable();

    //Save current interrupts and DMA settings so we can restore them upon exit.
    SystemADKCON = hw->adkconr;
    SystemInts = hw->intenar;
    SystemDMA = hw->dmaconr;
    hw->intena = 0x7fff; //disable all interrupts
    hw->intreq = 0x7fff; //Clear any interrupts that were pending

    WaitVbl();
    WaitVbl();
    hw->dmacon = 0x7fff; //Clear all DMA channels

    // enable selected DMA
    hw->dmacon =  0x87E0;
    hw->dmacon =  0x20;

    hw->bpl1mod = 0x0000;
    hw->bpl2mod = 0x0000;
    hw->diwstrt = 0x2c81;
    hw->diwstop = 0xf4d1;
    hw->ddfstrt = 0x0038;
    hw->ddfstop = 0x00d0;

    LoadView(0);
    WaitTOF();
    WaitTOF();

    WaitVbl();
    WaitVbl();

    VBR = GetVBR();
    SystemIrq = GetInterruptHandler(); //store interrupt register
}

void FreeSystem()
{
    WaitVbl();
    WaitBlt();
    hw->intena = 0x7fff; //disable all interrupts
    hw->intreq = 0x7fff; //Clear any interrupts that were pending
    hw->dmacon = 0x7fff; //Clear all DMA channels

    //restore interrupts
    SetInterruptHandler(SystemIrq);

    /*Restore system copper list(s). */
    hw->cop1lc = (ULONG)GfxBase->copinit;
    hw->cop2lc = (ULONG)GfxBase->LOFlist;
    hw->copjmp1 = 0x7fff; //start coppper

    /*Restore all interrupts and DMA settings. */
    hw->intena = SystemInts | 0x8000;
    hw->dmacon = SystemDMA | 0x8000;
    hw->adkcon = SystemADKCON | 0x8000;

    LoadView(ActiView);
    WaitTOF();
    WaitTOF();
    WaitBlit();
    DisownBlitter();
    Enable();
}

inline short MouseLeft() { return !((*(volatile UBYTE *)0xbfe001) & 64); }
inline short MouseRight() { return !((*(volatile UWORD *)0xdff016) & (1 << 10)); }

volatile UWORD bgcolor = 0;

static __attribute__((interrupt)) void interruptHandler()
{
    hw->intreq = (1 << INTB_VERTB);
    hw->intreq = (1 << INTB_VERTB); //reset vbl req. twice for a4000 bug.
}

int main()
{
    memcpy(bitmap, amigavikke, 5000);

    // Copy bitmap data into the bitplanes
    UWORD i = 320/8*65/4;
    int count = 0;
    i *= 2;
    while (i > 0) {
        bpl0[count] = bitmap[count];
        bpl1[count] = bitmap[count + 1280];
        count += 1;
        i -= 1;
    }

    SysBase = *((struct ExecBase **)4UL);
    hw = (struct Custom *)0xdff000;

    // We will use the graphics library only to locate and restore the system copper list once we are through.
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library",0);
    if (!GfxBase)
        Exit(0);

    // used for printing
    DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
    if (!DOSBase)
        Exit(0);

    KPrintF("Hello debugger from Amiga!\n");
    Write(Output(), "Hello console!\n", 15);
    Delay(50);

    warpmode(1);
    // TODO: precalc stuff here
    warpmode(0);

    TakeSystem();
    WaitVbl();

    // DEMO
    SetInterruptHandler((APTR)interruptHandler);
    hw->intena = (1 << INTB_SETCLR) | (1 << INTB_INTEN) | (1 << INTB_VERTB);
    hw->intreq = 1 << INTB_VERTB; //reset vbl req

    volatile UWORD frame = 1;
    while (!MouseLeft())
    {
        UpdateCopper(frame);
        WaitVbl();
        frame+=1;
    }

    // END
    FreeSystem();

    CloseLibrary((struct Library *)DOSBase);
    CloseLibrary((struct Library *)GfxBase);
}

void UpdateCopper(int frame) {

    hw->bplcon0 = 0x2200;  // 2 Bitplanes

    //; doubble buffering of copperlists, defined at copper1 and copper2, chosen by LSB in framecounter
    //; copper (and a6) will hold the address to the copperlist we will write to (not the one currently in use)
    d0 = frame;

    //; change effect settings according to framecounter,  normal or "additive" bars
    d1 = d0;                        // move.l d0,d1
    d1 &= 0xff;                     // and.l #$ff,d1 ; 255/50 ~ 5sec => every 5 sec change between modes

    if (d1 == 0) {                  // bne .10
        // Either 0 or 1
        cbar_mode = 0 ? !cbar_mode : 0; // eori.b #1,cbar_mode
    }
    //  .10:
    //; anglespeed
    d1 = d0;                        // move.l d0,d1
    d1 &= 0x1ff;                    // and.l #$1ff,d1; 511/50 ~ 10sec => every 10 sec speed changes +1, in interval [2,5]

    // bne .20
    //d2 = anglespeed;
    if (d1 == 0) {
        d2 = anglespeed;            // move.b anglespeed,d2
        d2 ++;                      // add.b #1,d2
        if (d2 == 6) {              // cmp.b #6,d2
            d2 = 2;
        }
        else {
            anglespeed = d2;
        }
    }
     // bne .21 ; Branch Not Equal to zero 
    else
    {
        //.20:
        //; startline
        d1 = d0;                    // move.l d0,d1
        d1 &= 0xff;                 // and.l #$ff,d1            ; change angle each frame, but only with 8 LSB!
        a0 = &sin255_60[0];         // move.l #sin255_60,a0
        d2 = *(a0 + d1);            // move.b (a0,d1),d2
        d1 = cbar_start_default;    // move.w cbar_start_default,d1
        d1 = d1 + d2;               // add.w d2,d1
        cbar_start = d1;            // move.w d1,cbar_start
    }

    d0 &= 0x1;                      // and.l #1,d0
    if (d0 == 0) // usecopper1:
    {
        a6 = &copper1[0];
        copper = (int)copper1;
    }
    else { // usecopper2:
        a6 = &copper2[0];
        copper = (int)copper2;
    }

    // bitplane 0
    int d00 = (int)bpl0;            // move.l #bpl0,d0
    *(a6++) = 0x00e2;               // move.w #$00e2,(a6)+  ; LO-bits of start of bitplane
    *(a6++) = d00 & 0xffff;         // move.w d0,(a6)+      ; go into $dff0e2
                                    // swap d0
    *(a6++) = 0x00e0;               // move.w #$00e0,(a6)+  ; HI-bits of start of bitplane
    *(a6++) = d00 >> 16;            // move.w d0,(a6)+      ; go into $dff0e0
    // bitplane 1
    d00 = (int)bpl1;                // move.l #bpl1,d0
    *(a6++) = 0x00e6;               // move.w #$00e6,(a6)+  ; LO-bits of start of bitplane
    *(a6++) = d00 & 0xffff;         // move.w d0,(a6)+      ; go into $dff0e6
                                    // swap d0
    *(a6++) = 0x00e4;               // move.w #$00e4,(a6)+  ; HI-bits of start of bitplane
    *(a6++) = d00 >> 16;            // move.w d0,(a6)+      ; go into $dff0e4

    // colors
    *(a6++) = 0x0180;
    *(a6++) = 0x0000;
    *(a6++) = 0x0182;
    *(a6++) = 0x0000;
    *(a6++) = 0x0184;
    *(a6++) = 0x0f00;
    *(a6++) = 0x0186;
    *(a6++) = 0x0fff;

    //; horizontal scroll
    *(a6++) = 0x0102;               // move.l #$01020000,(a6)+  ; 0 for both odd and even numbered bpl (rightmost 2 zeros)
    *(a6++) = 0x0000;

    //; change angles according to anglespeed
    d1 = anglespeed;                // move.b anglespeed, d1
    a0 = &sin255_60[0];             // move.l #sin255_60, a0
    a1 = &cos255[0];                // move.l #cos255, a1
    a2 = &cbar_a[0];                // move.l #cbar_a, a2
    a3 = &cbar_y[0];                // move.l #cbar_y, a3
    a4 = &cbar_z[0];                // move.l #cbar_z, a4
    d2 = 4;                         // move.b #4, d2
    while (d2 > 0) { // loop_copperbars :
        d3 = *(a2);                 // move.b(a2), d3     ; angle
		// &0xff to keep within range 0..255
        d3 = (d3 + d1)&0xff;        // add.b d1, d3       ; angle + anglespeed
        d4 = *(a0 + d3);            // move.b(a0, d3), d4 ; y from sintable
        d5 = *(a1 + d3);            // move.b(a1, d3), d5 ; z from costable
        *(a2++) = d3;               // move.b d3, (a2)+   ; angle
        *(a3++) = d4;               // move.b d4, (a3)+   ; y
        *(a4++) = d5;               // move.b d5, (a4)+   ; z
        d2 -= 1;                    // subq.b #1, d2
    }//bne loop_copperbars

    // get z - value for all bars
     a2 = &cbar_z[0];               // move.l #cbar_z, a2
    d1 = *(a2++);                   // move.b(a2)+ , d1
    d2 = *(a2++);                   // move.b(a2)+ , d2
    d3 = *(a2++);                   // move.b(a2)+ , d3
    d4 = 0;                         // moveq #0, d4; z red
    d5 = 0;                         // moveq #0, d5; z grn
    d6 = 0;                         // moveq #0, d6; z blu
    // compare z - values
    if (d2 <= d1) {                 // cmp.b d1, d2
                                    // bcc z1
        d5++;                       // addq #1, d5
                                    // bra z1e
    }
    else { //z1 :
        d4++;                       // addq #1, d4
    }
    //z1e :
    if (d3 <= d1) {                 // cmp.b d1, d3
                                    // bcc z2
        d6++;                       // addq #1, d6
                                    // bra z2e
    }
    else { //z2 :
        d4 += 1;                    // addq #1, d4
    }
    //z2e :
    if (d3 <= d2) {                 // cmp.b d2, d3
                                    // bcc z3
        d6++;                       // addq #1, d6
                                    // bra z3e
    }
    else { //z3 :
        d5++;                       // addq #1, d5
    }
    //z3e :
    a2 = &cbar_z[0];                // move.l #cbar_z, a2
    *(a2++) = d4;                   // move.b d4, (a2)+; red
    *(a2++) = d5;                   // move.b d5, (a2)+; grn
    *(a2++) = d6;                   // move.b d6, (a2)+; blu

    //; empty copperline data - needed because of "additive" - mode!
    d3 = 0;                         // moveq #0, d3
    a3 = &copperlines1[0];          // move.l #copperlines1, a3
    d2 = 90;                        // move.l #90, d2; max height of copperbars : 60 + 30
    while (d2 > 0) {                // loop_empty_copperlines1 :
         *(a3++) = d3;              // move.w d3, (a3)+
         d2--;                      // subq #1, d2
    }                               // bne loop_empty_copperlines1

    d0 = 0;                         // move.l #0,d0; d0 is the loop index
    while (d0 < 3)  //loop_rasterlines:
    {
        d3 = 0;                     // move.l #0,d3; no bar
        if (d0 == d6) {             // cmp.b d0,d6
                                    // bne zz1
            d3 = 3;                 // move.l #3,d3; blu
        }
        //zz1:
        if (d0 == d5) {             // cmp.b d0,d5
                                    // bne zz2
            d3 = 2;                 // move.l #2,d3; grn
        }
        //zz2:
        if (d0 == d4) {             // cmp.b d0,d4
                                    // bne zz3
            d3 = 1;                 // move.l #1,d3; red
        }
        //zz3:
        if (d3 != 0)                // cmp.b #0,d3
        {
            // beq copperline_nothing
            //; now d3 contains the number of the copperbar to be drawn(0 = dummy bar)
            a2 = &cbar_y[0];        // move.l #cbar_y, a2
            d1 = 0;                 // move.l #0, d1
            d1 = *(a2 + d3);        // move.b(a2, d3), d1; y start
            d1 = d1 << 1;           // lsl.b #1, d1//; *2 to get from b to w addressing in copperlines1
            a3 = &copperlines1[0];  // move.l #copperlines1, a3; copperlines1 is just an array for storing color info for each line
            a3 = (UWORD*)((int)a3 + d1); // add.l d1, a3
            d3 = d3 << 6;           // lsl.b #6, d3//; *64 to get from b to w addressing in cbars and each bar is 32 words long(*64 = *2 * 32)
            a2 = &cbars[0];         // move.l #cbars, a2
            a2 = (UWORD*)((int)a2 + d3); // add.l d3, a2

            d7 = 30;                // move.l #30, d7; d7 is the loop index, each bar is 30 lines high(+2 lines that aren't drawn, to get 32 = 2^5)
            while (d7 > 0) {
                //  loop_copperline_render:
                if (cbar_mode == 0) {   // cmp.b #0, cbar_mode
                                        // bne cbar_additive
                    *(a3++) = *(a2++);  // move.w(a2)+ , (a3)+; if normal mode : just write over old data
                                        // bra cbar_normal
                }
                else { //cbar_additive :
                    d1 = *(a2++);       // move.w(a2)+, d1; if additive mode : old OR new
                     *(a3++) = *(a3) | d1; // ori.w d1, (a3)+; this isn't the same as old + new, but it works well enough in binary
                }
                // cbar_normal:
                d7--;               // subq #1, d7
            }                       // bne loop_copperline_render
        } //copperline_nothing:
            d0++;                   // addq #1,d0
                                    // cmp #3,d0
    };                              // bcs loop_rasterlines

    //; let's finally "draw" the copperbars into the copperlist, so that the copper can "draw" them onto the display
    a2 = &copperlines1[0];          // move.l #copperlines1, a2
    d3 = cbar_start;                // move.w cbar_start, d3; startline
    d7 = 0;                         // move.l #0,d7; d7 is the loop index, going from 0 to 89 because the WAIT instructions of the copper have to be in the right order to work!
                                    // loop_rasterlines_copper:
    while (d7 != 90) {
        d4 = d3;                    // move.w d3,d4
        d4 = d4 + d7;               // add.w d7,d4
        if (d4 == 256) {            // cmp.w #256,d4; PAL needs a trick to to handle more than 256 lines in overscan 
                                    // bne no_PAL_fix
            *(a6++) = 0xffdf;       // move.l #$ffdffffe,(a6)+; we wait for the last beamposition on line 255 that is possible for the copper to handle, after that line 0 = line 256 etc
            *(a6++) = 0xfffe;
        }
        else {
            // no_PAL_fix:
            //; copper WAIT-instruction generation
            d4 = d7;                // move.w d7,d4             ; d4=d7
            d4 = d4 + d3;           // add.w d3,d4              ; d4=d7+d3 (d7+startline)
            d4 = d4 << 8;           // lsl.w #8,d4              ; d4=(d7+startline)*256
            d4 = d4 + 7;            // add.w #$07,d4            ; d4=(d7+startline)<<256+07
            *(a6++) = d4;           // move.w d4,(a6)+          ; Wait - first line, ex: $6407
            *(a6++) = 0xfffe;       // move.w #$fffe,(a6)+      ; Mask
            //; copper MOVE-instruction generation
            *(a6++) = 0x0180;       // move.w #$0180,(a6)+      ; Color0: $0180
            *(a6++) = *(a2++);      // move.w (a2)+,(a6)+       ; Colordata
        }
        d7++;                       // addq #1,d7
                                    // cmp #90,d7
                                    // bcs loop_rasterlines_copper
    }

    //; set colors to default on next line - for a "clean" setup on next screen
    //; d7 = 90 at this point, so copying the same code as above will put the next WAIT and MOVE instructions for the next line
    d4 = d7;                        // move.w d7,d4             ; d4=d7
    d4 = d4 + d3;                   // add.w d3,d4              ; d4=d7+d3 (d7+startline)
    d4 = d4 << 8;                   // lsl.w #8,d4              ; d4=(d7+startline)<<8
    d4 = d4 + 7;                    // add.w #$07,d4            ; d4=(d7+startline)<<8+07
    *(a6++) = d4;                   // move.w d4,(a6)+          ; Wait - first line ex: $6407
    *(a6++) = 0xfffe;               // move.w #$fffe,(a6)+      ; Mask
    *(a6++) = 0x0180;               // move.l #$01800000,(a6)+  ; color 0
    *(a6++) = 0x0000;

    //; end of copperlist(copperlist ALWAYS ends with WAIT $fffffffe)
    *(a6++) = 0xffff;
    *(a6++) = 0xfffe;

    // use next copperlist - as we are using doubblebuffering on copperlists we now take the new one into use
    hw->cop1lc = (ULONG)copper;
}
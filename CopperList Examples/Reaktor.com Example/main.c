// C conversion of https://www.reaktor.com/blog/crash-course-to-amiga-assembly-programming/

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

static int frame = 0;

void UpdateCopper(UWORD high, UWORD low, UWORD high1, UWORD low1, UWORD high2, UWORD low2);

INCBIN(masters, "data/masters3.bin")
static __attribute__((section("bitmap.MEMF_CHIP")))UWORD bitmap[202400];
static __attribute__((section("copper.MEMF_CHIP")))UWORD copper[158] = {};

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

    hw->cop1lc = (ULONG)copper;

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
    memcpy(bitmap, masters, 202400);
    // Pointer to the start of the array
    int bm = (int)bitmap;
    UWORD high = bm >> 16;
    UWORD low = bm & 0xffff;

    bm += 40;
    UWORD high1 = bm >> 16;
    UWORD low1 = bm & 0xffff;

    bm += 40;
    UWORD high2 = bm >> 16;
    UWORD low2 = bm & 0xffff;

    SysBase = *((struct ExecBase **)4UL);
    hw = (struct Custom *)0xdff000;

    // We will use the graphics library only to locate and restore the system copper list once we are through.
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
    if (!GfxBase)
        Exit(0);

    // used for printing
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
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

    while (!MouseLeft())
    {
        frame += 1;
        UpdateCopper(high, low, high1, low1, high2, low2);
        WaitVbl();
    }

    // END
    FreeSystem();

    CloseLibrary((struct Library *)DOSBase);
    CloseLibrary((struct Library *)GfxBase);
}

void UpdateCopper(UWORD high, UWORD low, UWORD high1, UWORD low1, UWORD high2, UWORD low2) {

    hw->bplcon0 = 0x3200;
    hw->bpl1mod = 0x0050;
    hw->bpl2mod = 0x0050;
    hw->diwstrt = 0x2c81;
    hw->diwstop = 0xc8d1;
    hw->ddfstrt = 0x0038;
    hw->ddfstop = 0x00d0;

    UWORD bitplanes[12] =
    {
        0x00e0, high,
        0x00e2, low,
        0x00e4, high1,
        0x00e6, low1,
        0x00e8, high2,
        0x00ea, low2,
    };
    UWORD colours[16] =
    {
        0x0180, 0x0fd3,
        0x0182, 0x0832,
        0x0184, 0x036b,
        0x0186, 0x0667,
        0x0188, 0x0f53,
        0x018a, 0x07ad,
        0x018c, 0x0000,
        0x018e, 0x0cef,
    };
    UWORD sin32_15[32] = { 8,9,10,12,13,14,14,15,15,15,14,14,13,12,10,9,8,6,5,3,2,1,1,0,0,0,1,1,2,3,5,6 };
    UWORD a6[130];
    UWORD i = 0;
    
    UWORD d0 = 32;              // move.l #32, d0; Number of iterations
    UWORD d1 = 7;               // move.l #$07, d1; Current row wait
    UWORD* a0 = &sin32_15[0];   // move.l #sin32_15, a0; Sine base
    UWORD d2 = frame + 1;       // move.l frame, d2; Current sine
    // scrollrows :
    while(d0 != 0) {
        // ; Wait for correct offset row
        a6[i + 0] = d1;         // move.w d1, (a6)+
        a6[i + 1] = 0xfffe;     // move.w #$fffe, (a6)+
        // ; Fetch sine from table
        UWORD d3 = d2;          // move.l d2, d3
        d3 = d3 & 0x1f;         // and.l #$1f, d3
        UWORD d4 = *(a0 + d3);  // move.b(a0, d3), d4
        // ; Transform sine to horizontal offset value
        UWORD d5 = d4;          // move.l d4, d5
        d4 = d4 << 4;           // lsl.l #4, d4
        d5 += d4;               // add.l d4, d5
        // ; Add horizontal offset to copperlist
        a6[i + 2] = 0x0102;     // move.w #$0102, (a6)+
        a6[i + 3] = d5;         // move.w d5, (a6)+
        // ; Proceed to next row that we want to offset
        d1 += 0x0500;           // add.l #$500, d1
        // ; Move to next sine position for next offset row
        d2 += 1;                // addq.w #1, d2
        d0 -= 1;                // subq.w #1, d0
        i += 4;
        // bne scrollrows
    }
    
    a6[i + 2] = 0xffff;
    a6[i + 3] = 0xfffe;

    UWORD count = 0;
    for (UWORD i = 0; i < 12; i++) {
        copper[count++] = bitplanes[i];
    }
    for (UWORD i = 0; i < 16; i++) {
        copper[count++] = colours[i];
    }
    for (UWORD i = 0; i < 130; i++) {
        copper[count++] = a6[i];
    }
}

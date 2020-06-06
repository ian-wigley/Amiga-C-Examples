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

static __attribute__((section("copper.MEMF_CHIP"))) UWORD coplist_pal[] = {

	0x0100, 0x0200, // don't need any Bitplanes
	0x0180, 0x0000, // Color0 = Black
	0x500F, 0xFFFE, // Wait VPos 0x50

	// Colors
	0x0180, 0x0F02, 0x0180, 0x0F04, 0x0180, 0x0F06, 0x0180, 0x0F08,
	0x0180, 0x0F0A, 0x0180, 0x0F0C, 0x0180, 0x0F0E, 0x0180, 0x0F0F,
	0x0180, 0x0E0F, 0x0180, 0x0C0F, 0x0180, 0x0A0F, 0x0180, 0x080F,
	0x0180, 0x060F, 0x0180, 0x040F, 0x0180, 0x020F, 0x0180, 0x000F,
	0x0180, 0x002F, 0x0180, 0x004F, 0x0180, 0x006F, 0x0180, 0x008F,
	0x0180, 0x00AF, 0x0180, 0x00CF, 0x0180, 0x00EF, 0x0180, 0x00FF,
	0x0180, 0x00FE, 0x0180, 0x00FC, 0x0180, 0x00FA, 0x0180, 0x00F8,
	0x0180, 0x00F6, 0x0180, 0x00F4, 0x0180, 0x00F2, 0x0180, 0x00F0,
	0x0180, 0x02F0, 0x0180, 0x04F0, 0x0180, 0x06F0, 0x0180, 0x08F0,
	0x0180, 0x0AF0, 0x0180, 0x0CF0, 0x0180, 0x0EF0, 0x0180, 0x0FF0,
	0x0180, 0x0FE0, 0x0180, 0x0FC0, 0x0180, 0x0FA0, 0x0180, 0x0F80,
	0x0180, 0x0F60, 0x0180, 0x0F40, 0x0180, 0x0F20, 0x0180, 0x0F00,
	0x0180, 0x0F02, 0x0180, 0x0F04, 0x0180, 0x0F06, 0x0180, 0x0F08,
	0x0180, 0x0F0A, 0x0180, 0x0F0C, 0x0180, 0x0000,

	0x0180, 0x0000, // Colour0 = Black
	0xffff, 0xfffe  // End of the copper list
};

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

	//set all colors black
	for (int a = 0; a < 32; a++)
		hw->color[a] = 0;

	hw->cop1lc = (ULONG)coplist_pal;

	// enable selected DMA
	hw->dmacon = 0x8280; // No Blitter or sprites, only copper

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

	// Pointer into the first colour in the copperlist
	UWORD pointer = 7;

	while (!MouseLeft())
	{
		WaitVbl();
		//hw->color[0] = bgcolor;

		// Take a copy of the 1st colour value
		UWORD firstColour = coplist_pal[pointer];

		for (UWORD i = 0; i < 54 * 2; i += 2)
		{
			// Copy the second colour value onto the first
			coplist_pal[pointer + i] = coplist_pal[pointer + 2 + i];
		}

		// Copy the value stored in "first" into the "last" value
		coplist_pal[pointer + (54 * 2) - 2] = firstColour;
	}

	// END
	FreeSystem();

	CloseLibrary((struct Library *)DOSBase);
	CloseLibrary((struct Library *)GfxBase);
}

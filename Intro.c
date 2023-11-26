#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "custom_defines.h"



#include "System.h"

#include "Math.h"


#include "Renderer.h"

#include "c2p_6b.h"


#include "clist.h"

UBYTE numstr[32] = "                              \n";
ULONG fps = 0;


void setColor(WORD index, UWORD hicolor, UWORD locolor) {
    UWORD offset = (index<<1)+(index>31?2:0);
    cl[79+offset] = hicolor;
    cl[211+offset] = locolor;
}

int main()
{
    
    struct ColorMap *cm;
    UBYTE *chunky;
    UBYTE *ptr;
    UBYTE *ptr10 = NULL, *ptr11, *ptr12, *ptr13, *ptr14, *ptr15;
    UBYTE *ptr20 = NULL, *ptr21, *ptr22, *ptr23, *ptr24, *ptr25;
    ULONG *ptrl;


    UWORD coplistSize;

    WORD i, j, l;
    UWORD k;
    
    Object3D *object1, *object2;
    

    k = 0;
    for (i = 0; i < HEIGHT; i++) {
        rowchunky[i] = k;
        k += 320;
    }


    
    // Open libs
    initMathLibs();
    IntuitionBase = (struct IntuitionBase*)OldOpenLibrary("intuition.library");

   	// open gfx lib and save original copperlist
  	GfxBase = (struct GfxBase*)OldOpenLibrary("graphics.library");
   	
    oldCopinit = GfxBase->copinit;
   	oldview = GfxBase->ActiView;
    oldViewPort = &IntuitionBase->ActiveScreen->ViewPort;
    cm = oldViewPort->ColorMap;
    
    // Alloc memory

    chunky = (UBYTE*)AllocMem(57600, MEMF_ANY | MEMF_CLEAR);

    cop1 = (UWORD*)AllocMem(sizeof(coplist), MEMF_CHIP | MEMF_CLEAR);
    cop2 = (UWORD*)AllocMem(sizeof(coplist), MEMF_CHIP | MEMF_CLEAR);
    
    CopyMem(coplist, cop1, sizeof(coplist));
    CopyMem(coplist, cop2, sizeof(coplist));
    
    
    ptr10 = AllocMem(((WIDTH * HEIGHT) >> 3) * 6, MEMF_CHIP | MEMF_CLEAR);
    ptr11 = ptr10 + 7200;
    ptr12 = ptr11 + 7200;
    ptr13 = ptr12 + 7200;
    ptr14 = ptr13 + 7200;
    ptr15 = ptr14 + 7200;
    
    ptr20 = AllocMem(((WIDTH * HEIGHT) >> 3) * 6, MEMF_CHIP | MEMF_CLEAR);
    ptr21 = ptr20 + 7200;
    ptr22 = ptr21 + 7200;
    ptr23 = ptr22 + 7200;
    ptr24 = ptr23 + 7200;
    ptr25 = ptr24 + 7200;

    if (chunky        != NULL &&
        cop1          != NULL &&
        cop2          != NULL && 
        ptr10         != NULL && 
        ptr20         != NULL) {
        
        generateSineCosTable();

        coplistSize = (sizeof(coplist)>>1)-4;

        palflag = SysBase->PowerSupplyFrequency < 59;
    
        vbr = (SysBase->AttnFlags & AFF_68010) ?
                (void *)Supervisor((void *)GetVBR) : NULL;

        object1 = loadObject3DWavefront("monkey.obj",0.12f);
        object2 = loadObject3DWavefront("icosphere.obj",0.08f);

        object1->pZ = 1000;
        object2->pZ = 1000;
        object2->pX = -300;
        object2->pY = -150;

        LoadView(NULL);
        WaitTOF();
        WaitTOF();

        // Install input handler
        installInputHandler();
        

        Forbid();

        // Save interrupts and DMA
        oldInt = custom->intenar;
        oldDMA = custom->dmaconr;
        
        // disable all interrupts and DMA
        custom->intena = 0x7fff;
        custom->intreq = 0x7fff; 
        custom->intreq = 0x7fff;
        custom->dmacon = 0x7fff;
        
        custom->dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER  | DMAF_COPPER;
        
        bplptr[0] = ptr20;
        bplptr[1] = ptr10;

        clist[0] = (ULONG)cop1;
        clist[1] = (ULONG)cop2;


        cop1[BPL0]     = (ULONG)ptr10 >> 16;
        cop1[BPL0 + 2] = (ULONG)ptr10 & 0xffff;
        cop1[BPL1]     = (ULONG)ptr11 >> 16;
        cop1[BPL1 + 2] = (ULONG)ptr11 & 0xffff;
        cop1[BPL2]     = (ULONG)ptr12 >> 16;
        cop1[BPL2 + 2] = (ULONG)ptr12 & 0xffff;
        cop1[BPL3]     = (ULONG)ptr13 >> 16;
        cop1[BPL3 + 2] = (ULONG)ptr13 & 0xffff;
        cop1[BPL4]     = (ULONG)ptr14 >> 16;
        cop1[BPL4 + 2] = (ULONG)ptr14 & 0xffff;
        cop1[BPL5]     = (ULONG)ptr15 >> 16;
        cop1[BPL5 + 2] = (ULONG)ptr15 & 0xffff;
        
        cop2[BPL0]     = (ULONG)ptr20 >> 16;
        cop2[BPL0 + 2] = (ULONG)ptr20 & 0xffff;
        cop2[BPL1]     = (ULONG)ptr21 >> 16;
        cop2[BPL1 + 2] = (ULONG)ptr21 & 0xffff;
        cop2[BPL2]     = (ULONG)ptr22 >> 16;
        cop2[BPL2 + 2] = (ULONG)ptr22 & 0xffff;
        cop2[BPL3]     = (ULONG)ptr23 >> 16;
        cop2[BPL3 + 2] = (ULONG)ptr23 & 0xffff;
        cop2[BPL4]     = (ULONG)ptr24 >> 16;
        cop2[BPL4 + 2] = (ULONG)ptr24 & 0xffff;
        cop2[BPL5]     = (ULONG)ptr25 >> 16;
        cop2[BPL5 + 2] = (ULONG)ptr25 & 0xffff;
		

        c2p1x1_6_c5_030_init(320, 180, 0, 0, 40, 7200, 0);


        custom->cop1lc = (ULONG)cop1;
        ptr = ptr20;
        
        
        scene3d.objects[0] = object1;
        scene3d.objects[1] = object2;
        scene3d.nobjects = 2;

        initRenderer();

        loadObject3DPalette(object1);

        fps = 0;
        scene = 0;
        dbcount = 2;

      
        installVBInterrupt();
        while ((custom->potinp&0x400) && (cia->ciapra & CIAF_GAMEPORT0)) { 
            cl = (UWORD*)clist[frame^1];
            switch (scene) {
                case 0:
                    if (dbcount > 0) { 
                        dbcount --;
                    }

                    light[0] = -FP_UNIT;
                    light[1] = -FP_UNIT;
                    light[2] = -FP_UNIT;

                    nVectorsPaint = 0;
                    
                    minZ =  32767;
                    maxZ = -32768;
                    
                    matrixIdentity(mt);
                    matrixXYZRotation(mt, (scnvb<<1)&1023,(scnvb)&1023,(scnvb>>1)&1023);
                    matrixMultiply31(light, mt);
                    matrixTranspose(mt);
             
                    update(object1, &object2d, mt);
                    update(object2, &object2d, mt);
                    render(nVectorsPaint, &scene3d, &object2d, chunky);


                    break;
            }

            c2p1x1_6_c5_030(chunky, ptr);

            // Wait VBlank and swap buffers
            waitVBlank();
            frame ^= 1;
            custom->cop1lc = clist[frame];
            ptr = bplptr[frame];
            
            fps++;
                
            // Counters for sync intro scenes
            cvb = vbcounter;
            cvbinc = vbinc;
            vbinc = 0;
            scnvb += cvbinc;
            
            // Clear chunky screen

            ptrl = ((ULONG*)chunky);
            for (i = 0; i < 14400; i++) {
                *ptrl++ = clearColor;
            }

           
        }

        removeVBInterrupt();

    
        // restore DMA
        custom->dmacon = 0x7fff;
        custom->dmacon = oldDMA | DMAF_SETCLR | DMAF_MASTER;


        // restore original copper
        custom->cop1lc = (ULONG) oldCopinit;

        // restore interrupts
        custom->intena = oldInt | 0xc000;
        
        Permit();
        
        // Remove input handler
        removeInputHandler();
        
        LoadView(oldview);
        WaitTOF();
        WaitTOF();

        freeObject3D(object1);
        freeObject3D(object2);
    }
    else
        printf("Error: Not enough memory\n");
  

    FreeMem(ptr10, ((WIDTH * HEIGHT) >> 3) * 6);
    FreeMem(ptr20, ((WIDTH * HEIGHT) >> 3) * 6);
    FreeMem(chunky, 57600);
  
    
    FreeMem(cop1, sizeof(coplist));
    FreeMem(cop2, sizeof(coplist));
    
        
    closeMathLibs();
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);


   return 0;
}



#include <exec/execbase.h>
#include <exec/memory.h>  

#define FIXED_POINT 1

#define FP 10
#define FP_UNIT 1024
#define FP_2D 6
#define FP_2D_UNIT 64

//#define IEEESP 1

#ifdef IEEESP

#define float FLOAT
#define int LONG
#include <libraries/mathieeesp.h>
#include <clib/mathieeesingbas_protos.h>
#include <clib/mathieeesingtrans_protos.h>
#define fix IEEESPFix
#define flt IEEESPFlt

#else

#include <libraries/mathffp.h>
#include <clib/mathffp_protos.h> 
#include <clib/mathtrans_protos.h>
#define fix SPFix
#define flt SPFlt

#endif

#define k_pi            3.14159265358979f
#define k_pi_inverse    0.31830988618379f
#define k_pi_half       1.57079632679f


LONG fpsin[1024];
LONG fpcos[1024];


void initMathLibs(void) {
#ifdef IEEESP
    if (MathIeeeSingBasBase = OpenLibrary("mathieeesingbas.library",34)) 
    {
    }
    
    if (MathIeeeSingTransBase = OpenLibrary("mathieeesingtrans.library",34))
    {
    }
#else
    if (MathBase = OpenLibrary("mathffp.library", 0))
    {
    }
    
    if (MathTransBase = OpenLibrary("mathtrans.library",0))
    {
    }
#endif
}

void closeMathLibs(void) {
    #ifdef IEEESP
    if (MathIeeeSingBasBase != NULL)
        CloseLibrary(MathIeeeSingBasBase);
    if (MathIeeeSingTransBase != NULL) 
        CloseLibrary(MathIeeeSingTransBase);
#else
    if (MathBase != NULL)
        CloseLibrary(MathBase);
    if (MathTransBase != NULL) 
        CloseLibrary(MathTransBase);
#endif
}


void generateSineCosTable(void) {
#ifdef FIXED_POINT
    WORD k = 0;
    FLOAT prc, fsi;
    for (k = 0; k < 1024; k++) {
        prc = 2.0*k_pi*(k/1024.0);
        fsi = sin(prc);
        fpsin[k] = fsi*FP_UNIT;
        fsi = cos(prc);
        fpcos[k] = fsi*FP_UNIT;
    }
#endif
}

void matrixIdentity(LONG m[4][4]) {
    
    m[0][0] = FP_UNIT; m[1][0] = 0;       m[2][0] = 0;       m[3][0] = 0;
    m[0][1] = 0;       m[1][1] = FP_UNIT; m[2][1] = 0;       m[3][1] = 0;
    m[0][2] = 0;       m[1][2] = 0;       m[2][2] = FP_UNIT; m[3][2] = 0;
    m[0][3] = 0;       m[1][3] = 0;       m[2][3] = 0;       m[3][3] = FP_UNIT;
}

void matrixMultiply31(LONG m1[3], LONG m2[4][4]) {
    BYTE i, k;
    LONG result[3];
    for(i = 0; i < 3; i++) {
        result[i] = 0;
        for(k = 0; k < 3; k++) {
            result[i] += (m1[k]*m2[k][i])>>FP;
        }
    }
    CopyMem(result, m1, 12);
}

void matrixMultiply41(LONG m1[4], LONG m2[4][4]) {
    BYTE i, k;
    LONG result[4];
    for(i = 0; i < 4; i++) {
        result[i] = 0;
        for(k = 0; k < 4; k++) {
            result[i] += (m1[k]*m2[k][i])>>FP;
        }
    }
    CopyMem(result, m1, 16);
}

void matrixTranslate(LONG m[4], LONG tx, LONG ty, LONG tz) {
    m[0] += tx; 
    m[1] += ty;
    m[2] += tz;
}

void matrixTranspose(LONG m[4][4]) {
    LONG i, j, result[4][4];
    for (i = 0; i < 4; i++) {
       for (j = 0; j < 4; j++) {
          result[i][j] = m[j][i];
       }
    }
    CopyMem(result, m, 64);
}

void matrixTranslation(LONG m[4][4], LONG tx, LONG ty, LONG tz) {
    m[0][3]=tx;
    m[1][3]=ty;
    m[2][3]=tz;
}

void matrixScale(LONG m[4][4], LONG sx, LONG sy, LONG sz) {
    m[0][0] = sx; m[1][0] = 0;  m[2][0] = 0;  m[3][0] = 0;
    m[0][1] = 0;  m[1][1] = sy; m[2][1] = 0;  m[3][1] = 0;
    m[0][2] = 0;  m[1][2] = 0;  m[2][2] = sz; m[3][2] = 0;
    m[0][3] = 0;  m[1][3] = 0;  m[2][3] = 0;  m[3][3] = FP_UNIT;
}

void matrixXRotation(LONG m[4][4], LONG angle) {
    LONG sin,cos;

    sin=fpsin[angle];
    cos=fpcos[angle];

    m[1][1]=cos;
    m[2][1]=-sin;
    m[1][2]=sin;
    m[2][2]=cos;
}

void matrixYRotation(LONG m[4][4], LONG angle) {
    LONG sin,cos;

    sin=fpsin[angle];
    cos=fpcos[angle];
    
    m[0][0]=cos;
    m[2][0]=sin;
    m[0][2]=-sin;
    m[2][2]=cos;
}

void matrixZRotation(LONG m[4][4], LONG angle) {
    LONG sin,cos;

    sin=fpsin[angle];
    cos=fpcos[angle];
    
    m[0][0]=cos;
    m[1][0]=-sin;
    m[0][1]=sin;
    m[1][1]=cos;
}

void matrixXYZRotation(LONG m[4][4], LONG x, LONG y, LONG z) {
    m[0][0] = (fpcos[y]*fpcos[z])>>FP;
    m[1][0] = (fpcos[y]*fpsin[z])>>FP;
    m[2][0] = -fpsin[y];
    m[0][1] = ((((fpsin[x]*fpsin[y])>>FP)*fpcos[z])>>FP)-((fpcos[x]*fpsin[z])>>FP);
    m[1][1] = ((((fpsin[x]*fpsin[y])>>FP)*fpsin[z])>>FP)+((fpcos[x]*fpcos[z])>>FP);
    m[2][1] = (fpsin[x]*fpcos[y])>>FP;
    m[0][2] = ((((fpcos[x]*fpsin[y])>>FP)*fpcos[z])>>FP)+((fpsin[x]*fpsin[z])>>FP);
    m[1][2] = ((((fpcos[x]*fpsin[y])>>FP)*fpsin[z])>>FP)-((fpsin[x]*fpcos[z])>>FP);
    m[2][2] = ((fpcos[x]*fpcos[y])>>FP);
}

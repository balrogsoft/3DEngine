
#define MAX_OBJECTS             8
#define MAX_MATERIALS           4
#define ZBUFFER_LEN             1200
#define BUFFER2D_LEN            1600

#define WIDTH_FP                20416
#define HEIGHT_FP               11520


typedef struct {
    UBYTE red;
    UBYTE green;
    UBYTE blue;
} Material;

typedef struct  {
    UWORD npoints;
    UWORD nvectors;
    UBYTE nmaterials;
    WORD *points;
    WORD *vectors;
    WORD *normals;
    UWORD *rowoffset;
    Material *material[MAX_MATERIALS];

    WORD pX;
    WORD pY;
    WORD pZ;
    WORD rX;
    WORD rY;
    WORD rZ;
    WORD tX;
    WORD tY;
    WORD tZ;

//    WORD dirty[4];

    WORD npoints_size;
    WORD nvectors_size;
} Object3D;

typedef struct {
    WORD points[MAX_OBJECTS][BUFFER2D_LEN];
    WORD *vectors[ZBUFFER_LEN];
} Object2D;

typedef struct {
    UBYTE nobjects;
    Object3D *objects[MAX_OBJECTS];
} Scene3D;

Scene3D scene3d;

Object2D object2d;

ULONG rowchunky[HEIGHT];

LONG wpoints[1600];
ULONG medlight[800];
UBYTE cpoint[800];


UWORD nVectorsPaint;

LONG mt[4][4];

LONG light[3];

LONG minZ = 0, maxZ = 0;

UBYTE objectId = 0;

UBYTE stringStartsWith(char *str1, char *str2) {
    WORD l = 0;
    while (str1[l] != 0) {
        if (str1[l] != str2[l])
            return 0;
        l++;
    }
    return 1;
}

void initRenderer(void) {
    UWORD i, j;
    for (i = 0; i < scene3d.nobjects; i++) {
        scene3d.objects[i]->npoints_size = scene3d.objects[i]->npoints*3;
        scene3d.objects[i]->nvectors_size = scene3d.objects[i]->nvectors*5;
        for (j = 0; j < BUFFER2D_LEN; j++) {
            object2d.points[i][j] = 0;
        }
    }
    for (i = 0; i < ZBUFFER_LEN; i++) {
        object2d.vectors[i] = NULL;
    }
}
void freeMaterial(Material *mat) {
    if (mat)
        FreeMem(mat, sizeof(Material));
}

void freeObject3D(Object3D *obj) {
    if (obj) {

        if (obj->rowoffset)
            FreeMem(obj->rowoffset, obj->npoints*2);

        if (obj->normals)
            FreeMem(obj->normals, obj->nvectors*3*2);
        if (obj->nmaterials > 0 && obj->material) {
            UBYTE i;
            for (i = 0; i < obj->nmaterials; i++)
                freeMaterial(obj->material[i]);
        }
        FreeMem(obj, sizeof(Object3D));
    }
}


void loadMaterial(char *name, Object3D *object) {
    UBYTE *buffer;
    UBYTE str[64];
    BPTR file_ptr;
    UBYTE mat_idx = 0;
    Material *mat = NULL;
    if (file_ptr = Open(name, MODE_OLDFILE))
    {
        while (buffer = FGets(file_ptr, str, 64L)) {
            if (stringStartsWith("Kd ",str) == 1) {
                const char s[2] = " ";
                char *value;
                BYTE idx = 0;
                value = strtok(str, s);
                mat = AllocMem(sizeof(Material), MEMF_ANY|MEMF_CLEAR);
                while(value != NULL) {
                    if (idx > 0) {
                        FLOAT f = afp(value);
                        switch (idx) {
                            case 1:
                                mat->red = 255*f;
                                break;
                            case 2:
                                mat->green = 255*f;
                                break;
                            case 3:
                                mat->blue = 255*f;
                                break;
                        }
                    }
                    value = strtok(NULL, s);
                    idx++;
                }
                if (mat_idx < MAX_MATERIALS)
                    object->material[mat_idx++] = mat;
                else
                    printf("Error: Material overflow. Can't handle more than 4 materials\n");
            }
        }
        object->nmaterials = mat_idx;
        Close(file_ptr);
    }
}


Object3D* loadObject3DWavefront(char *name, FLOAT scale) {

    UBYTE *buffer;
    UBYTE str[64];
    BPTR file_ptr;
    WORD *wpptr = NULL;
    WORD *wvptr = NULL;
    WORD *wnptr = NULL;
    Object3D *obj = NULL;
    WORD  wv = 0;
    UBYTE cmat = 0;

    if (file_ptr = Open(name, MODE_OLDFILE))
    {
        obj = AllocMem(sizeof(Object3D), MEMF_ANY|MEMF_CLEAR);
        obj->points = NULL;
        obj->vectors = NULL;
        while (buffer = FGets(file_ptr, str, 64L)) {
            if (stringStartsWith("mtllib",str) == 1) {
                Material *mat;
                char res[32];
                strncpy(res, &str[7], strlen(str) - 8);
                res[strlen(str)-8] = 0;
                loadMaterial(res, obj);
            }
            else if (stringStartsWith("usemtl",str) == 1) {
                const char s[2] = ".";
                char *value;
                BYTE idx = 0;
                value = strtok(str, s);
                while(value != NULL) {
                    if (idx == 1)
                        cmat = atoi(value)-1;
                    value = strtok(NULL, s);
                    idx++;
                }

            }
            else  if (stringStartsWith("d ",str) == 1) {
                const char s[2] = " ";
                char *value;
                BYTE idx = 0;
                value = strtok(str, s);
                while(value != NULL) {
                    LONG i = atoi(value);
                    if (idx == 1) {
                        obj->npoints = i;
                        obj->points = (WORD*)AllocMem(i*3*2, MEMF_ANY);
                        obj->rowoffset = (UWORD*)AllocMem(obj->npoints*2, MEMF_ANY);

                        obj->npoints_size = obj->npoints*3;
                        wpptr = obj->points;
                    }
                    else if (idx == 2) {
                        obj->nvectors = i;
                        obj->vectors = (WORD*)AllocMem(i*5*2, MEMF_ANY);

                        obj->nvectors_size = obj->nvectors*5;

                        obj->normals = (WORD*)AllocMem(i*3*2, MEMF_ANY);
                        wvptr = obj->vectors;
                        wnptr = obj->normals;
                    }
                    value = strtok(NULL, s);
                    idx++;
                }
            }

            else if (stringStartsWith("v ",str) == 1) {
                const char s[2] = " ";
                char *value;
                value = strtok(&str[2], s);
                while(value != NULL) {
                    FLOAT f = afp(value);
                    WORD w = f*FP_UNIT*scale;
                    *wpptr++ = w;
                    value = strtok(NULL, s);
                }
            }
            else if (stringStartsWith("vn ",str) == 1) {
                const char s[2] = " ";
                char *value;
                value = strtok(&str[2], s);
                while(value != NULL) {
                    FLOAT f = afp(value);
                    WORD w = f*FP_UNIT;

                    *wnptr++ = w;
                    value = strtok(NULL, s);
                }
            }
            else if (stringStartsWith("f ",str) == 1) {
                char s[2] = " ";
                char b[2] = "/";
                char *value;
                BYTE idx = 0;
                WORD w;
                value = strtok(&str[2], s);
                while(value != NULL) {
                    char res[5] = "    ";
                    strncpy(res, value, strchr(value , b[0])-value);
                    if (idx >= 0 && idx < 3) {
                        w = atoi(res)-1;

                        *wvptr++ = w;
                    }
                    value = strtok(NULL, s);
                    idx++;
                }
                *wvptr++ = 0x00;
                w = (objectId<<12) | (cmat<<8);
                *wvptr++ = w;
                wv++;

            }
        }
        if (obj->rowoffset) {
            WORD i, j;
            j = 0;
            for (i = 0; i < obj->npoints; i++) {
                obj->rowoffset[i] = j;
                j += obj->npoints;
            }
        }
        Close(file_ptr);
    }
    printf("object Id: %i\n",objectId);
    obj->tX = obj->tY = obj->tZ = 0;
    obj->rX = obj->rY = obj->rZ = 0;
    objectId++;
    return obj;
}

void loadObject3DPalette(Object3D *obj) {
    UBYTE i,j;
    for (i = 0; i < obj->nmaterials; i++) {
        UBYTE red = obj->material[i]->red,
            green = obj->material[i]->green,
             blue = obj->material[i]->blue;
        FLOAT delta = 0.475;
        for (j = 0; j < 8; j++) {
            UBYTE lred = red  * delta,
                lgreen = green * delta,
                lblue  = blue  * delta;
                cop1[79 +((j+(i<<3)+32)<<1)+2] = ((lred&0xf0)<<4) | (lgreen&0xf0)      | ((lblue&0xf0)>>4);
                cop1[211+((j+(i<<3)+32)<<1)+2] = ((lred&0x0f)<<8) | ((lgreen&0x0f)<<4) | (lblue&0x0f);
                cop2[79 +((j+(i<<3)+32)<<1)+2] = ((lred&0xf0)<<4) | (lgreen&0xf0)      | ((lblue&0xf0)>>4);
                cop2[211+((j+(i<<3)+32)<<1)+2] = ((lred&0x0f)<<8) | ((lgreen&0x0f)<<4) | (lblue&0x0f);
            delta += 0.075;
        }
    }
}

void update(Object3D *object, Object2D *obj2d, LONG m[4][4]) {
    UWORD i, j, k;
    UBYTE id = object->vectors[4]>>12;
    WORD *points2d = obj2d->points[id];
    LONG tpoints[3];
    /*
    object->dirty[0] = 20416;
    object->dirty[1] = 12736;
    object->dirty[2] = 0;
    object->dirty[3] = 0;
    */
    for (j = 0, k = 0, i = 0; j < object->npoints_size; j+=3, k+=2, i++) {
        tpoints[0] = object->points[j];
        tpoints[1] = object->points[j+1];
        tpoints[2] = object->points[j+2];
        //tpoints[3] = FP_UNIT;

        matrixMultiply31(tpoints, m);
        matrixTranslate(tpoints, object->pX, object->pY, object->pZ);

        wpoints[j]   = tpoints[0];
        wpoints[j+1] = tpoints[1];
        wpoints[j+2] = tpoints[2];

        medlight[i] = 0;
        cpoint[i]   = 0;

        if (wpoints[j+2] > 0) {
            points2d[k]   = (CWIDTH2_FP2D)  + (object->tX<<FP_2D) + ((wpoints[j]<<14)   / wpoints[j+2]);
            points2d[k+1] = (CHEIGHT2_FP2D) + (object->tY<<FP_2D) + ((wpoints[j+1]<<14) / wpoints[j+2]);

            if (wpoints[j+2] > maxZ)
                maxZ = wpoints[j+2];
            if (wpoints[j+2] < minZ)
                minZ = wpoints[j+2];
/*
            if (points2d[k] < object->dirty[0])
                object->dirty[0] = points2d[k];
            else if (points2d[k] > object->dirty[2])
                object->dirty[2] = points2d[k];

            if (points2d[k+1] < object->dirty[1])
                object->dirty[1] = points2d[k+1];
            else if (points2d[k+1] > object->dirty[3])
                object->dirty[3] = points2d[k+1];
  */

        }
    }

    for (k = 0, i = 0; k < object->nvectors_size; k+=5, i+=3) {
        WORD ix1 = (object->vectors[k]   << 1),
             ix2 = (object->vectors[k+1] << 1),
             ix3 = (object->vectors[k+2] << 1);
        WORD tx1 = ix1 + object->vectors[k],
             tx2 = ix2 + object->vectors[k+1],
             tx3 = ix3 + object->vectors[k+2];
        WORD nx = object->normals[i],
             ny = object->normals[i+1],
             nz = object->normals[i+2];

        LONG x1 = wpoints[tx1],
             y1 = wpoints[tx1 + 1],
             z1 = wpoints[tx1 + 2],
             z2 = wpoints[tx2 + 2],
             z3 = wpoints[tx3 + 2];

        LONG dx1 = points2d[ix3]   - points2d[ix1];
        LONG dy1 = points2d[ix3+1] - points2d[ix1+1];
        LONG dx2 = points2d[ix3]   - points2d[ix2];
        LONG dy2 = points2d[ix3+1] - points2d[ix2+1];

        LONG zrange = maxZ-minZ;

        if (zrange != 0 && ((dx1*(dy1-dy2) - (dx1-dx2)*dy1) >> FP) > 0) {
            WORD pol;
            WORD lint;

            LONG z = (z1 + z2 + z3) >> 2;

            z = z + (z >> 2) + (z >> 4) + (z >> 6);

            lint =  (((light[0] * nx)>>FP) + ((light[1] * ny)>>FP) + ((light[2] * nz)>>FP));
            if (lint < 0)
                lint = 0;
            if (lint > 7)
                lint = 7;

            object->vectors[k+3] = lint;


            pol = (((maxZ - z) << 10) / zrange);

            if (obj2d->vectors[pol] == NULL) {
                obj2d->vectors[pol] = &object->vectors[k];
                nVectorsPaint ++;
            }
            else {
                for (j = pol + 1; j < ZBUFFER_LEN; j++) {
                    if (obj2d->vectors[j] == NULL) {
                        obj2d->vectors[j] = &object->vectors[k];
                        nVectorsPaint ++;
                        break;
                    }
                }
            }
        }
        else {
            object->vectors[k+3] = -1;
        }

    }

    for (i = 0; i < object->npoints; i++) {
        for (k = 0; k < object->nvectors_size; k+=5) {
            if (object->vectors[k+3] != -1) {
                if (object->vectors[k] == i) {
                    medlight[i] += object->vectors[k+3];
                    cpoint[i]++;
                }
                else if (object->vectors[k+1] == i) {
                    medlight[i] += object->vectors[k+3];
                    cpoint[i]++;
                }
                else if (object->vectors[k+2] == i) {
                    medlight[i] += object->vectors[k+3];
                    cpoint[i]++;
                }
            }
        }
        if (cpoint[i] > 0)
            medlight[i] /= cpoint[i];
    }

    for (k = 0; k < object->nvectors_size; k+=5) {
        if (object->vectors[k+3] != -1) {
            WORD ix1 = object->vectors[k],
                 ix2 = object->vectors[k+1],
                 ix3 = object->vectors[k+2];
            UBYTE color = (object->vectors[k+4] >> 8)&0xf;
            color = 0x20 + ((color)<<3);
            object->vectors[k+3]  = (color + (medlight[ix1])) << 8;
            object->vectors[k+3] |= (color + (medlight[ix2])) & 0xff;
            object->vectors[k+4] &= 0xff00;
            object->vectors[k+4] |= (color + (medlight[ix3])) & 0xff;
        }
    }
}

WORD render(UWORD nvectors, Scene3D *scene, Object2D *obj2d, UBYTE *chunky) {
    UWORD i;
    WORD count = 0;
    WORD **vectors = obj2d->vectors;
    UBYTE *ptr;
    for (i = 0; i < ZBUFFER_LEN; i++) {
        if (vectors[i] != NULL) {
            WORD *vec = vectors[i];
            WORD a = vec[0],
                 b = vec[1],
                 c = vec[2];
            UBYTE id = vec[4]>>12;
            Object3D *obj = scene->objects[id];
            WORD *points = obj2d->points[id];
            WORD x1 = points[(a<<1)],
                 y1 = points[(a<<1)+1],
                 x2 = points[(b<<1)],
                 y2 = points[(b<<1)+1],
                 x3 = points[(c<<1)],
                 y3 = points[(c<<1)+1],
                 x1d,y1d,x2d,y2d,x3d,y3d,ix1,ix2,ix3,ix4,ix5,ix6,
                 dx1 = 0, dx2 = 0, dx3 = 0, dx1b, dx2b, dx3b,
                tmpx, tmpy, y, x, scx1, scx2, scolleft, scolright, swap = 0, coldelta = 0, scol1;
            WORD dxt, scanleft, scanright;
            BYTE calc = 0;
            WORD color1 = vec[3]>>8,
                 color2 = vec[3]&0xff,
                 color3 = vec[4]&0xff,
                 dc1 = 0, dc2 = 0, dc3 = 0;
            color1 <<= FP_2D;
            color2 <<= FP_2D;
            color3 <<= FP_2D;

            if (x1 < 0) {
                x1 = 0;
                calc |= 1;
            }

            if (x1 > WIDTH_FP) {
                x1 = WIDTH_FP;
                calc |= 1;
            }

            if (x2 < 0) {
                x2 = 0;
                calc |= 2;
            }
            if (x2 > WIDTH_FP) {
                x2 = WIDTH_FP;
                calc |= 2;
            }

            if (x3 < 0) {
                x3 = 0;
                calc |= 4;
            }
            if (x3 > WIDTH_FP) {
                x3 = WIDTH_FP;
                calc |= 4;
            }

            if (y1 < 0) {
                y1 = 0;
                calc |= 1;
            }

            if (y1 > HEIGHT_FP) {
                y1 = HEIGHT_FP;
                calc |= 1;
            }

            if (y2 < 0) {
                y2 = 0;
                calc |= 2;

            }
            if (y2 > HEIGHT_FP) {
                y2 = HEIGHT_FP;
                calc |= 2;
            }

            if (y3 < 0) {
                y3 = 0;
                calc |= 4;
            }
            if (y3 > HEIGHT_FP) {
                y3 = HEIGHT_FP;
                calc |= 4;
            }

            if (calc != 7) {

                if (y2 > y3)
                {
                    tmpx = x2;
                    tmpy = y2;
                    x2 = x3;
                    y2 = y3;
                    x3 = tmpx;
                    y3 = tmpy;

                    tmpx = color2;
                    color2 = color3;
                    color3 = tmpx;

                    tmpx = b;
                    b = c;
                    c = tmpx;
                }

                if (y1 > y2)
                {
                    tmpx = x1;
                    tmpy = y1;
                    x1 = x2;
                    y1 = y2;
                    x2 = tmpx;
                    y2 = tmpy;

                    tmpx = color1;
                    color1 = color2;
                    color2 = tmpx;

                    tmpx = a;
                    a = b;
                    b = tmpx;
                }

                if (y2 > y3)
                {
                    tmpx = x2;
                    tmpy = y2;
                    x2 = x3;
                    y2 = y3;
                    x3 = tmpx;
                    y3 = tmpy;

                    tmpx = color2;
                    color2 = color3;
                    color3 = tmpx;

                    tmpx = b;
                    b = c;
                    c = tmpx;
                }
                x1d = x1>>FP_2D;
                y1d = y1>>FP_2D;
                x2d = x2>>FP_2D;
                y2d = y2>>FP_2D;
                x3d = x3>>FP_2D;
                y3d = y3>>FP_2D;



                if (y2d != y1d)
                    dx1 = dx1b = (x2 - x1) / (y2d - y1d);

                if (y3d != y1d)
                    dx2 = dx2b = (x3 - x1) / (y3d - y1d);

                if (y3d != y2d)
                    dx3 = dx3b = (x3 - x2) / (y3d - y2d);

                if (y2d != y1d)
                    dc1 = (color2 - color1) / (y2d - y1d);

                if (y3d != y1d)
                    dc2 = (color3 - color1) / (y3d - y1d);

                if (y3d != y2d)
                    dc3 = (color3 - color2) / (y3d - y2d);


                scanleft  = x1;
                scanright = x1;

                scolleft  = color1;
                scolright = color1;

                ptr = &chunky[rowchunky[y1d]];

                if (dx2 < dx1) {
                    dxt = dx2;
                    dx2 = dx1;
                    dx1 = dxt;
                    swap = 1;

                    dxt = dc2;
                    dc2 = dc1;
                    dc1 = dxt;
                }
                for (y = y1d; y < y2d; y++) {
                    scx1 = (scanleft>>FP_2D)+((scanleft&0x3f)>>5);
                    scx2 = (scanright>>FP_2D)+((scanright&0x3f)>>5);
                    if ((scx2-scx1+1) > 0) {
                        coldelta = (scolright-scolleft) / (scx2-scx1+1);
                        scol1    = scolleft;
                        ptr+=scx1;
                        for (x = scx1; x <= scx2; x++) {
                            *ptr++ = scol1>>FP_2D;
                            scol1 += coldelta;
                        }
                    }
                    else
                        ptr+=scx2;
                    scolleft  += dc1;
                    scolright += dc2;

                    scanleft  += dx1;
                    scanright += dx2;

                    ptr += 319-scx2;
                }
                if (swap!=0) {
                    dxt = dx2;
                    dx2 = dx1;
                    dx1 = dxt;

                    dxt = dc2;
                    dc2 = dc1;
                    dc1 = dxt;
                }

                if (dx2 < dx3) {
                    dx1 = dx3;
                    dc1 = dc3;
                    if (y1d == y2d) {
                        scanleft  = x2;
                        scanright = x1;

                        scolleft  = color2;
                        scolright = color1;
                    }
                }
                else
                {
                    dx1 = dx2;
                    dx2 = dx3;

                    dc1 = dc2;
                    dc2 = dc3;
                    if (y1d == y2d) {
                        scanleft  = x1;
                        scanright = x2;

                        scolleft  = color1;
                        scolright = color2;
                    }
                }
                for (y = y2d; y < y3d; y++) {
                    scx1 = (scanleft>>FP_2D)  + ((scanleft&0x3f)>>5);
                    scx2 = (scanright>>FP_2D) + ((scanright&0x3f)>>5);
                    if ((scx2-scx1+1) > 0)
                        coldelta = (scolright-scolleft) / (scx2-scx1+1);
                        scol1    = scolleft;
                        ptr+=scx1;
                        for (x = scx1; x <= scx2; x++) {
                            *ptr++ = scol1>>FP_2D;
                            scol1 += coldelta;
                        }

                    scolleft  += dc1;
                    scolright += dc2;

                    scanleft  += dx1;
                    scanright += dx2;

                    ptr += 319-scx2;
                }


            }
            vectors[i] = NULL;
            count++;
        }

        if (count == nvectors)
            break;
    }
    return count;
}


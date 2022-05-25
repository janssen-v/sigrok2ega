#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>

#include <io.h>
#include <fcntl.h>

#define IMG_WIDTH 640
#define IMG_HEIGHT 400

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
static const Uint32 rmask = 0xff000000;
static const Uint32 gmask = 0x00ff0000;
static const Uint32 bmask = 0x0000ff00;
static const Uint32 amask = 0x000000ff;
#else
static const Uint32 rmask = 0x00ff0000;
static const Uint32 gmask = 0x0000ff00;
static const Uint32 bmask = 0x000000ff;
static const Uint32 amask = 0xff000000;
#endif

static Uint32 egapal[64];

void init_pal()
{
    int i;
    for (i = 0; i < 64; ++i)
    {
        Uint32 color =
            (((i & (1 << 0)) << 1) + ((i & (1 << 3)) >> 3)) * (0x55555555 & bmask) + (((i & (1 << 1)) >> 0) + ((i & (1 << 3)) >> 3)) * (0x55555555 & gmask) + (((i & (1 << 2)) >> 1) + ((i & (1 << 3)) >> 3)) * (0x55555555 & rmask) + amask;
        printf("%xd ", color);
        egapal[i] = color;
    }
}

void pset(
    SDL_Surface *surface,
    unsigned int x,
    unsigned int y,
    unsigned char color)
{
    if (x >= IMG_WIDTH || y >= IMG_HEIGHT)
    {
        return;
    }

    Uint32 *target_pixel = surface->pixels + y * surface->pitch +
                           x * sizeof(*target_pixel);
    *target_pixel = egapal[color];
}

int main()
{
    _setmode(_fileno(stdin), _O_BINARY);

    int x_scaled;
    int filter = 0;
    int mode = 320;
    int vsynccount = 0;
    int hsynccount = 0;
    int new_frame = 0;
    unsigned char value, color1;
    int hsync = 0, vsync = 0;
    int frame = 0;
    unsigned int vsyncc = 0, ref_len = 0;
    unsigned int x = 0, y = 0;
    SDL_Window *window;

    init_pal();

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(
        "SIGROK2EGA",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        IMG_WIDTH,
        IMG_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (window == NULL)
    {
        SDL_Log("Could not create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL)
    {
        SDL_DestroyWindow(window);
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *surface =
        SDL_CreateRGBSurface(
            0,
            IMG_WIDTH,
            IMG_HEIGHT,
            32,
            rmask,
            gmask,
            bmask,
            amask);
    if (surface == NULL)
    {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surface);
    if (tex == NULL)
    {
        SDL_Log("SDL_CreateTextueFromSurface() failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // SDL_SetWindowSize(window,640,480);
    // SDL_RenderSetLogicalSize(ren, 640, 400);
    // SDL_RenderSetScale(ren, 2, 2);

    while (!feof(stdin))
    {

        value = fgetc(stdin);
        vsync = value & 128;
        hsync = value & 64;
        vsynccount = vsynccount + vsync;
        // hsynccount=hsynccount+hsync;

        if (mode == 640)
        {
            if (vsync != 0)
            {

                vsyncc = 0;
                new_frame = 0;
            }
            else
            {
                vsyncc++;
                if (vsyncc > 20)
                {
                    vsyncc = 0;
                    y = 0;
                    // this means a new frame?
                    if (!new_frame)
                    {
                        new_frame = 1;
                        frame++;
                        if (vsynccount < 400000)
                        {
                            if (vsynccount > 380000)
                            {
                                mode = 320;
                            }
                        }
                        else
                        {
                            mode = 640;
                        }
                        printf(" vsynccount %u\n", vsynccount);
                        // printf(" hsynccount %u\n", hsynccount);
                        vsynccount = 0;
                        // hsynccount=0;

                        SDL_UpdateTexture(tex, NULL, surface->pixels, surface->pitch);
                        // SDL_RenderClear(ren);
                        SDL_RenderCopy(ren, tex, NULL, NULL);
                        SDL_RenderPresent(ren);

                        SDL_Event e;
                        if (SDL_PollEvent(&e))
                        {
                            if (e.type == SDL_QUIT)
                            {
                                break;
                            }
                            else if (e.type == SDL_KEYDOWN)
                            {

                                switch (e.key.keysym.sym)
                                {

                                case SDLK_UP:
                                    filter = 1;
                                    printf("Filter on\n");
                                    break;
                                case SDLK_DOWN:
                                    filter = 0;
                                    printf("Filter off\n");
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            color1 = value & 0x3F;
            int x_scaled = x - 75;
            int y_scaled = y - 3;

            // if (y>32){
            // if (y<235){

            pset(surface, x_scaled, y_scaled, color1);

            x++;
            if (hsync)
            {
                ref_len++;
                if (ref_len > 30 && x > 700)
                {
                    //	pset(surface, x_scaled, y, 5);
                    y++;
                    x = 0;
                }
            }
            else
            {
                ref_len = 0;
            }
        }

        if (mode == 320)
        {
            if (vsync == 0)
            {

                vsyncc = 0;
                new_frame = 0;
            }
            else
            {
                vsyncc++;
                if (vsyncc > 20)
                {
                    vsyncc = 0;
                    y = 0;
                    // this means a new frame?
                    if (!new_frame)
                    {
                        new_frame = 1;
                        //	  frame++;
                        if (vsynccount < 400000)
                        {
                            if (vsynccount > 380000)
                            {
                                mode = 320;
                            }
                        }
                        else
                        {
                            mode = 640;
                        }
                        printf(" vsynccount %u\n", vsynccount);
                        vsynccount = 0;
                        // printf(" hsynccount %u\n", hsynccount);
                        // hsynccount=0;
                        SDL_UpdateTexture(tex, NULL, surface->pixels, surface->pitch);
                        // SDL_RenderClear(ren);
                        SDL_RenderCopy(ren, tex, NULL, NULL);
                        SDL_RenderPresent(ren);

                        SDL_Event e;
                        if (SDL_PollEvent(&e))
                        {
                            if (e.type == SDL_QUIT)
                            {
                                break;
                            }
                            else if (e.type == SDL_KEYDOWN)
                            {

                                switch (e.key.keysym.sym)
                                {

                                case SDLK_UP:
                                    filter = 1;
                                    printf("Filter on\n");
                                    break;
                                case SDLK_DOWN:
                                    filter = 0;
                                    printf("Filter off\n");
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            color1 = value & 0x3F;
            // fast perfekt int x_scaled = round(319.5/714.5*(x-126));
            // int x_scaled = round(319.5/714.5*(x-126));
            // if (x==174) {int x_scaled = 300;}
            //  int x_scaled=x    ;
            int y_scaled = y - 34;

            // unfiltered: data is from x=168 and 719 pixels long. Needs to be scaled to 640 pixels to fit in window
            if (filter == 0)
            {
                x_scaled = x - 168;
                x_scaled = x_scaled * 0.9;
                pset(surface, x_scaled, y_scaled * 2, color1);
                pset(surface, x_scaled, (y_scaled * 2) - 1, color1);
            }

            else
            {

                x_scaled = 330;
                // if (y>32){
                if (y < 235)
                {
                    if (x < 889)
                    {

                        if (x < 550)
                        {
                            if (x > 174)
                            {

                                if (x < 359)
                                {

                                    if (x < 194)
                                    {
                                        if (x > 174)
                                        {
                                            if (x == 175)
                                            {
                                                x_scaled = 0;
                                            }
                                            if (x == 177)
                                            {
                                                x_scaled = 1;
                                            }
                                            if (x == 179)
                                            {
                                                x_scaled = 2;
                                            }
                                            if (x == 182)
                                            {
                                                x_scaled = 3;
                                            }
                                            if (x == 184)
                                            {
                                                x_scaled = 4;
                                            }
                                            if (x == 186)
                                            {
                                                x_scaled = 5;
                                            }
                                            if (x == 188)
                                            {
                                                x_scaled = 6;
                                            }
                                            if (x == 191)
                                            {
                                                x_scaled = 7;
                                            }
                                            if (x == 193)
                                            {
                                                x_scaled = 8;
                                            }
                                        }
                                    }
                                    if (x < 218)
                                    {
                                        if (x > 194)
                                        {
                                            if (x == 195)
                                            {
                                                x_scaled = 9;
                                            }
                                            if (x == 197)
                                            {
                                                x_scaled = 10;
                                            }
                                            if (x == 200)
                                            {
                                                x_scaled = 11;
                                            }
                                            if (x == 202)
                                            {
                                                x_scaled = 12;
                                            }
                                            if (x == 204)
                                            {
                                                x_scaled = 13;
                                            }
                                            if (x == 206)
                                            {
                                                x_scaled = 14;
                                            }
                                            if (x == 209)
                                            {
                                                x_scaled = 15;
                                            }
                                            if (x == 211)
                                            {
                                                x_scaled = 16;
                                            }
                                            if (x == 213)
                                            {
                                                x_scaled = 17;
                                            }
                                            if (x == 215)
                                            {
                                                x_scaled = 18;
                                            }
                                            if (x == 217)
                                            {
                                                x_scaled = 19;
                                            }
                                        }
                                    }
                                    if (x > 219)
                                    {
                                        if (x < 236)
                                        {
                                            if (x == 220)
                                            {
                                                x_scaled = 20;
                                            }
                                            if (x == 222)
                                            {
                                                x_scaled = 21;
                                            }
                                            if (x == 224)
                                            {
                                                x_scaled = 22;
                                            }
                                            if (x == 226)
                                            {
                                                x_scaled = 23;
                                            }
                                            if (x == 229)
                                            {
                                                x_scaled = 24;
                                            }
                                            if (x == 231)
                                            {
                                                x_scaled = 25;
                                            }
                                            if (x == 233)
                                            {
                                                x_scaled = 26;
                                            }
                                            if (x == 235)
                                            {
                                                x_scaled = 27;
                                            }
                                        }
                                    }
                                    if (x > 237)
                                    {
                                        if (x < 261)
                                        {

                                            if (x == 238)
                                            {
                                                x_scaled = 28;
                                            }
                                            if (x == 240)
                                            {
                                                x_scaled = 29;
                                            }
                                            if (x == 242)
                                            {
                                                x_scaled = 30;
                                            }
                                            if (x == 244)
                                            {
                                                x_scaled = 31;
                                            }
                                            if (x == 247)
                                            {
                                                x_scaled = 32;
                                            }
                                            if (x == 249)
                                            {
                                                x_scaled = 33;
                                            }
                                            if (x == 251)
                                            {
                                                x_scaled = 34;
                                            }
                                            if (x == 253)
                                            {
                                                x_scaled = 35;
                                            }
                                            if (x == 255)
                                            {
                                                x_scaled = 36;
                                            }
                                            if (x == 258)
                                            {
                                                x_scaled = 37;
                                            }
                                            if (x == 260)
                                            {
                                                x_scaled = 38;
                                            }
                                        }
                                    }
                                    if (x < 283)
                                    {
                                        if (x > 261)
                                        {
                                            if (x == 262)
                                            {
                                                x_scaled = 39;
                                            }
                                            if (x == 264)
                                            {
                                                x_scaled = 40;
                                            }
                                            if (x == 267)
                                            {
                                                x_scaled = 41;
                                            }
                                            if (x == 269)
                                            {
                                                x_scaled = 42;
                                            }
                                            if (x == 271)
                                            {
                                                x_scaled = 43;
                                            }
                                            if (x == 273)
                                            {
                                                x_scaled = 44;
                                            }
                                            if (x == 276)
                                            {
                                                x_scaled = 45;
                                            }
                                            if (x == 278)
                                            {
                                                x_scaled = 46;
                                            }
                                            if (x == 280)
                                            {
                                                x_scaled = 47;
                                            }
                                            if (x == 282)
                                            {
                                                x_scaled = 48;
                                            }
                                        }
                                    }
                                    if (x < 310)
                                    {
                                        if (x > 284)
                                        {
                                            if (x == 285)
                                            {
                                                x_scaled = 49;
                                            }
                                            if (x == 287)
                                            {
                                                x_scaled = 50;
                                            }
                                            if (x == 289)
                                            {
                                                x_scaled = 51;
                                            }
                                            if (x == 291)
                                            {
                                                x_scaled = 52;
                                            }
                                            if (x == 293)
                                            {
                                                x_scaled = 53;
                                            }
                                            if (x == 296)
                                            {
                                                x_scaled = 54;
                                            }
                                            if (x == 298)
                                            {
                                                x_scaled = 55;
                                            }
                                            if (x == 300)
                                            {
                                                x_scaled = 56;
                                            }
                                            if (x == 302)
                                            {
                                                x_scaled = 57;
                                            }
                                            if (x == 305)
                                            {
                                                x_scaled = 58;
                                            }
                                            if (x == 307)
                                            {
                                                x_scaled = 59;
                                            }
                                            if (x == 309)
                                            {
                                                x_scaled = 60;
                                            }
                                        }
                                    }
                                    if (x > 310)
                                    {
                                        if (x < 332)
                                        {
                                            if (x == 311)
                                            {
                                                x_scaled = 61;
                                            }
                                            if (x == 314)
                                            {
                                                x_scaled = 62;
                                            }
                                            if (x == 316)
                                            {
                                                x_scaled = 63;
                                            }
                                            if (x == 318)
                                            {
                                                x_scaled = 64;
                                            }
                                            if (x == 320)
                                            {
                                                x_scaled = 65;
                                            }
                                            if (x == 323)
                                            {
                                                x_scaled = 66;
                                            }
                                            if (x == 325)
                                            {
                                                x_scaled = 67;
                                            }
                                            if (x == 327)
                                            {
                                                x_scaled = 68;
                                            }
                                            if (x == 329)
                                            {
                                                x_scaled = 69;
                                            }
                                            if (x == 331)
                                            {
                                                x_scaled = 70;
                                            }
                                        }
                                    }
                                    if (x > 333)
                                    {
                                        if (x < 359)
                                        {

                                            if (x == 334)
                                            {
                                                x_scaled = 71;
                                            }
                                            if (x == 336)
                                            {
                                                x_scaled = 72;
                                            }
                                            if (x == 338)
                                            {
                                                x_scaled = 73;
                                            }
                                            if (x == 340)
                                            {
                                                x_scaled = 74;
                                            }
                                            if (x == 343)
                                            {
                                                x_scaled = 75;
                                            }
                                            if (x == 345)
                                            {
                                                x_scaled = 76;
                                            }
                                            if (x == 347)
                                            {
                                                x_scaled = 77;
                                            }
                                            if (x == 349)
                                            {
                                                x_scaled = 78;
                                            }
                                            if (x == 352)
                                            {
                                                x_scaled = 79;
                                            }
                                            if (x == 354)
                                            {
                                                x_scaled = 80;
                                            }
                                            if (x == 356)
                                            {
                                                x_scaled = 81;
                                            }
                                            if (x == 358)
                                            {
                                                x_scaled = 82;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    if (x > 360)
                                    {
                                        if (x < 400)
                                        {
                                            if (x == 361)
                                            {
                                                x_scaled = 83;
                                            }
                                            if (x == 363)
                                            {
                                                x_scaled = 84;
                                            }
                                            if (x == 365)
                                            {
                                                x_scaled = 85;
                                            }
                                            if (x == 367)
                                            {
                                                x_scaled = 86;
                                            }
                                            if (x == 369)
                                            {
                                                x_scaled = 87;
                                            }
                                            if (x == 372)
                                            {
                                                x_scaled = 88;
                                            }
                                            if (x == 374)
                                            {
                                                x_scaled = 89;
                                            }
                                            if (x == 376)
                                            {
                                                x_scaled = 90;
                                            }
                                            if (x == 378)
                                            {
                                                x_scaled = 91;
                                            }
                                            if (x == 381)
                                            {
                                                x_scaled = 92;
                                            }
                                            if (x == 383)
                                            {
                                                x_scaled = 93;
                                            }
                                            if (x == 385)
                                            {
                                                x_scaled = 94;
                                            }
                                            if (x == 387)
                                            {
                                                x_scaled = 95;
                                            }
                                            if (x == 390)
                                            {
                                                x_scaled = 96;
                                            }
                                            if (x == 392)
                                            {
                                                x_scaled = 97;
                                            }
                                            if (x == 394)
                                            {
                                                x_scaled = 98;
                                            }
                                            if (x == 396)
                                            {
                                                x_scaled = 99;
                                            }
                                            if (x == 399)
                                            {
                                                x_scaled = 100;
                                            }
                                        }
                                    }

                                    if (x > 400)
                                    {
                                        if (x < 422)
                                        {
                                            if (x == 401)
                                            {
                                                x_scaled = 101;
                                            }
                                            if (x == 403)
                                            {
                                                x_scaled = 102;
                                            }
                                            if (x == 405)
                                            {
                                                x_scaled = 103;
                                            }
                                            if (x == 407)
                                            {
                                                x_scaled = 104;
                                            }
                                            if (x == 410)
                                            {
                                                x_scaled = 105;
                                            }
                                            if (x == 412)
                                            {
                                                x_scaled = 106;
                                            }
                                            if (x == 414)
                                            {
                                                x_scaled = 107;
                                            }
                                            if (x == 416)
                                            {
                                                x_scaled = 108;
                                            }
                                            if (x == 419)
                                            {
                                                x_scaled = 109;
                                            }
                                            if (x == 421)
                                            {
                                                x_scaled = 110;
                                            }
                                        }
                                    }

                                    if (x > 422)
                                    {
                                        if (x < 449)
                                        {

                                            if (x == 423)
                                            {
                                                x_scaled = 111;
                                            }
                                            if (x == 425)
                                            {
                                                x_scaled = 112;
                                            }
                                            if (x == 428)
                                            {
                                                x_scaled = 113;
                                            }
                                            if (x == 430)
                                            {
                                                x_scaled = 114;
                                            }
                                            if (x == 432)
                                            {
                                                x_scaled = 115;
                                            }
                                            if (x == 434)
                                            {
                                                x_scaled = 116;
                                            }
                                            if (x == 437)
                                            {
                                                x_scaled = 117;
                                            }
                                            if (x == 439)
                                            {
                                                x_scaled = 118;
                                            }
                                            if (x == 441)
                                            {
                                                x_scaled = 119;
                                            }
                                            if (x == 443)
                                            {
                                                x_scaled = 120;
                                            }
                                            if (x == 445)
                                            {
                                                x_scaled = 121;
                                            }
                                            if (x == 448)
                                            {
                                                x_scaled = 122;
                                            }
                                        }
                                    }
                                    if (x < 549)
                                    {
                                        if (x > 449)
                                        {
                                            if (x < 471)
                                            {
                                                if (x == 450)
                                                {
                                                    x_scaled = 123;
                                                }
                                                if (x == 452)
                                                {
                                                    x_scaled = 124;
                                                }
                                                if (x == 454)
                                                {
                                                    x_scaled = 125;
                                                }
                                                if (x == 457)
                                                {
                                                    x_scaled = 126;
                                                }
                                                if (x == 459)
                                                {
                                                    x_scaled = 127;
                                                }
                                                if (x == 461)
                                                {
                                                    x_scaled = 128;
                                                }
                                                if (x == 463)
                                                {
                                                    x_scaled = 129;
                                                }
                                                if (x == 466)
                                                {
                                                    x_scaled = 130;
                                                }
                                                if (x == 468)
                                                {
                                                    x_scaled = 131;
                                                }
                                                if (x == 470)
                                                {
                                                    x_scaled = 132;
                                                }
                                            }
                                        }
                                        if (x > 471)
                                        {
                                            if (x < 500)
                                            {
                                                if (x == 472)
                                                {
                                                    x_scaled = 133;
                                                }
                                                if (x == 475)
                                                {
                                                    x_scaled = 134;
                                                }
                                                if (x == 477)
                                                {
                                                    x_scaled = 135;
                                                }
                                                if (x == 479)
                                                {
                                                    x_scaled = 136;
                                                }
                                                if (x == 481)
                                                {
                                                    x_scaled = 137;
                                                }
                                                if (x == 483)
                                                {
                                                    x_scaled = 138;
                                                }
                                                if (x == 486)
                                                {
                                                    x_scaled = 139;
                                                }
                                                if (x == 488)
                                                {
                                                    x_scaled = 140;
                                                }
                                                if (x == 490)
                                                {
                                                    x_scaled = 141;
                                                }
                                                if (x == 492)
                                                {
                                                    x_scaled = 142;
                                                }
                                                if (x == 495)
                                                {
                                                    x_scaled = 143;
                                                }
                                                if (x == 497)
                                                {
                                                    x_scaled = 144;
                                                }
                                                if (x == 499)
                                                {
                                                    x_scaled = 145;
                                                }
                                            }
                                        }
                                        if (x > 500)
                                        {
                                            if (x < 522)
                                            {
                                                if (x == 501)
                                                {
                                                    x_scaled = 146;
                                                }
                                                if (x == 504)
                                                {
                                                    x_scaled = 147;
                                                }
                                                if (x == 506)
                                                {
                                                    x_scaled = 148;
                                                }
                                                if (x == 508)
                                                {
                                                    x_scaled = 149;
                                                }
                                                if (x == 510)
                                                {
                                                    x_scaled = 150;
                                                }
                                                if (x == 513)
                                                {
                                                    x_scaled = 151;
                                                }
                                                if (x == 515)
                                                {
                                                    x_scaled = 152;
                                                }
                                                if (x == 517)
                                                {
                                                    x_scaled = 153;
                                                }
                                                if (x == 519)
                                                {
                                                    x_scaled = 154;
                                                }
                                                if (x == 521)
                                                {
                                                    x_scaled = 155;
                                                }
                                            }
                                        }
                                        if (x > 523)
                                        {
                                            if (x < 550)
                                            {
                                                if (x == 524)
                                                {
                                                    x_scaled = 156;
                                                }
                                                if (x == 526)
                                                {
                                                    x_scaled = 157;
                                                }
                                                if (x == 528)
                                                {
                                                    x_scaled = 158;
                                                }
                                                if (x == 530)
                                                {
                                                    x_scaled = 159;
                                                }
                                                if (x == 533)
                                                {
                                                    x_scaled = 160;
                                                }
                                                if (x == 535)
                                                {
                                                    x_scaled = 161;
                                                }
                                                if (x == 537)
                                                {
                                                    x_scaled = 162;
                                                }
                                                if (x == 539)
                                                {
                                                    x_scaled = 163;
                                                }
                                                if (x == 542)
                                                {
                                                    x_scaled = 164;
                                                }
                                                if (x == 544)
                                                {
                                                    x_scaled = 165;
                                                }
                                                if (x == 546)
                                                {
                                                    x_scaled = 166;
                                                }
                                                if (x == 548)
                                                {
                                                    x_scaled = 167;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (x < 750)
                            {
                                if (x < 674)
                                {
                                    if (x > 550)
                                    {
                                        if (x < 572)
                                        {
                                            if (x == 551)
                                            {
                                                x_scaled = 168;
                                            }
                                            if (x == 553)
                                            {
                                                x_scaled = 169;
                                            }
                                            if (x == 555)
                                            {
                                                x_scaled = 170;
                                            }
                                            if (x == 557)
                                            {
                                                x_scaled = 171;
                                            }
                                            if (x == 559)
                                            {
                                                x_scaled = 172;
                                            }
                                            if (x == 562)
                                            {
                                                x_scaled = 173;
                                            }
                                            if (x == 564)
                                            {
                                                x_scaled = 174;
                                            }
                                            if (x == 566)
                                            {
                                                x_scaled = 175;
                                            }
                                            if (x == 568)
                                            {
                                                x_scaled = 176;
                                            }
                                            if (x == 571)
                                            {
                                                x_scaled = 177;
                                            }
                                        }
                                    }
                                    if (x > 572)
                                    {
                                        if (x < 598)
                                        {

                                            if (x == 573)
                                            {
                                                x_scaled = 178;
                                            }
                                            if (x == 575)
                                            {
                                                x_scaled = 179;
                                            }
                                            if (x == 577)
                                            {
                                                x_scaled = 180;
                                            }
                                            if (x == 580)
                                            {
                                                x_scaled = 181;
                                            }
                                            if (x == 582)
                                            {
                                                x_scaled = 182;
                                            }
                                            if (x == 584)
                                            {
                                                x_scaled = 183;
                                            }
                                            if (x == 586)
                                            {
                                                x_scaled = 184;
                                            }
                                            if (x == 589)
                                            {
                                                x_scaled = 185;
                                            }
                                            if (x == 591)
                                            {
                                                x_scaled = 186;
                                            }
                                            if (x == 593)
                                            {
                                                x_scaled = 187;
                                            }
                                            if (x == 595)
                                            {
                                                x_scaled = 188;
                                            }
                                            if (x == 597)
                                            {
                                                x_scaled = 189;
                                            }
                                        }
                                    }
                                    if (x > 599)
                                    {
                                        if (x < 623)
                                        {
                                            if (x == 600)
                                            {
                                                x_scaled = 190;
                                            }
                                            if (x == 602)
                                            {
                                                x_scaled = 191;
                                            }
                                            if (x == 604)
                                            {
                                                x_scaled = 192;
                                            }
                                            if (x == 606)
                                            {
                                                x_scaled = 193;
                                            }
                                            if (x == 609)
                                            {
                                                x_scaled = 194;
                                            }
                                            if (x == 611)
                                            {
                                                x_scaled = 195;
                                            }
                                            if (x == 613)
                                            {
                                                x_scaled = 196;
                                            }
                                            if (x == 615)
                                            {
                                                x_scaled = 197;
                                            }
                                            if (x == 618)
                                            {
                                                x_scaled = 198;
                                            }
                                            if (x == 620)
                                            {
                                                x_scaled = 199;
                                            }
                                            if (x == 622)
                                            {
                                                x_scaled = 200;
                                            }
                                        }
                                    }
                                    if (x > 623)
                                    {
                                        if (x < 650)
                                        {
                                            if (x == 624)
                                            {
                                                x_scaled = 201;
                                            }
                                            if (x == 627)
                                            {
                                                x_scaled = 202;
                                            }
                                            if (x == 629)
                                            {
                                                x_scaled = 203;
                                            }
                                            if (x == 631)
                                            {
                                                x_scaled = 204;
                                            }
                                            if (x == 633)
                                            {
                                                x_scaled = 205;
                                            }
                                            if (x == 635)
                                            {
                                                x_scaled = 206;
                                            }
                                            if (x == 638)
                                            {
                                                x_scaled = 207;
                                            }
                                            if (x == 640)
                                            {
                                                x_scaled = 208;
                                            }
                                            if (x == 642)
                                            {
                                                x_scaled = 209;
                                            }
                                            if (x == 644)
                                            {
                                                x_scaled = 210;
                                            }
                                            if (x == 647)
                                            {
                                                x_scaled = 211;
                                            }
                                            if (x == 649)
                                            {
                                                x_scaled = 212;
                                            }
                                        }
                                    }
                                    if (x > 650)
                                    {
                                        if (x < 674)
                                        {

                                            if (x == 651)
                                            {
                                                x_scaled = 213;
                                            }
                                            if (x == 653)
                                            {
                                                x_scaled = 214;
                                            }
                                            if (x == 656)
                                            {
                                                x_scaled = 215;
                                            }
                                            if (x == 658)
                                            {
                                                x_scaled = 216;
                                            }
                                            if (x == 660)
                                            {
                                                x_scaled = 217;
                                            }
                                            if (x == 663)
                                            {
                                                x_scaled = 218;
                                            }
                                            if (x == 665)
                                            {
                                                x_scaled = 219;
                                            }
                                            if (x == 667)
                                            {
                                                x_scaled = 220;
                                            }
                                            if (x == 669)
                                            {
                                                x_scaled = 221;
                                            }
                                            if (x == 671)
                                            {
                                                x_scaled = 222;
                                            }
                                            if (x == 673)
                                            {
                                                x_scaled = 223;
                                            }
                                        }
                                    }
                                }

                                if (x > 675)
                                {
                                    if (x < 699)
                                    {

                                        if (x == 676)
                                        {
                                            x_scaled = 224;
                                        }
                                        if (x == 678)
                                        {
                                            x_scaled = 225;
                                        }
                                        if (x == 680)
                                        {
                                            x_scaled = 226;
                                        }
                                        if (x == 682)
                                        {
                                            x_scaled = 227;
                                        }
                                        if (x == 685)
                                        {
                                            x_scaled = 228;
                                        }
                                        if (x == 687)
                                        {
                                            x_scaled = 229;
                                        }
                                        if (x == 689)
                                        {
                                            x_scaled = 230;
                                        }
                                        if (x == 691)
                                        {
                                            x_scaled = 231;
                                        }
                                        if (x == 694)
                                        {
                                            x_scaled = 232;
                                        }
                                        if (x == 696)
                                        {
                                            x_scaled = 233;
                                        }
                                        if (x == 698)
                                        {
                                            x_scaled = 234;
                                        }
                                    }
                                }
                                if (x > 699)
                                {
                                    if (x < 724)
                                    {
                                        if (x == 700)
                                        {
                                            x_scaled = 235;
                                        }
                                        if (x == 703)
                                        {
                                            x_scaled = 236;
                                        }
                                        if (x == 705)
                                        {
                                            x_scaled = 237;
                                        }
                                        if (x == 707)
                                        {
                                            x_scaled = 238;
                                        }
                                        if (x == 709)
                                        {
                                            x_scaled = 239;
                                        }
                                        if (x == 711)
                                        {
                                            x_scaled = 240;
                                        }
                                        if (x == 714)
                                        {
                                            x_scaled = 241;
                                        }
                                        if (x == 716)
                                        {
                                            x_scaled = 242;
                                        }
                                        if (x == 718)
                                        {
                                            x_scaled = 243;
                                        }
                                        if (x == 720)
                                        {
                                            x_scaled = 244;
                                        }
                                        if (x == 723)
                                        {
                                            x_scaled = 245;
                                        }
                                    }
                                }
                                if (x > 724)
                                {
                                    if (x < 750)
                                    {
                                        if (x == 725)
                                        {
                                            x_scaled = 246;
                                        }
                                        if (x == 727)
                                        {
                                            x_scaled = 247;
                                        }
                                        if (x == 729)
                                        {
                                            x_scaled = 248;
                                        }
                                        if (x == 732)
                                        {
                                            x_scaled = 249;
                                        }
                                        if (x == 734)
                                        {
                                            x_scaled = 250;
                                        }
                                        if (x == 736)
                                        {
                                            x_scaled = 251;
                                        }
                                        if (x == 738)
                                        {
                                            x_scaled = 252;
                                        }
                                        if (x == 741)
                                        {
                                            x_scaled = 253;
                                        }
                                        if (x == 743)
                                        {
                                            x_scaled = 254;
                                        }
                                        if (x == 745)
                                        {
                                            x_scaled = 255;
                                        }
                                        if (x == 747)
                                        {
                                            x_scaled = 256;
                                        }
                                        if (x == 749)
                                        {
                                            x_scaled = 257;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (x > 751)
                                {
                                    if (x < 775)
                                    {
                                        if (x == 752)
                                        {
                                            x_scaled = 258;
                                        }
                                        if (x == 754)
                                        {
                                            x_scaled = 259;
                                        }
                                        if (x == 756)
                                        {
                                            x_scaled = 260;
                                        }
                                        if (x == 758)
                                        {
                                            x_scaled = 261;
                                        }
                                        if (x == 761)
                                        {
                                            x_scaled = 262;
                                        }
                                        if (x == 763)
                                        {
                                            x_scaled = 263;
                                        }
                                        if (x == 765)
                                        {
                                            x_scaled = 264;
                                        }
                                        if (x == 767)
                                        {
                                            x_scaled = 265;
                                        }
                                        if (x == 770)
                                        {
                                            x_scaled = 266;
                                        }
                                        if (x == 772)
                                        {
                                            x_scaled = 267;
                                        }
                                        if (x == 774)
                                        {
                                            x_scaled = 268;
                                        }
                                    }
                                }
                                if (x > 775)
                                {
                                    if (x > 775)
                                    {
                                        if (x < 800)
                                        {

                                            if (x == 776)
                                            {
                                                x_scaled = 269;
                                            }
                                            if (x == 778)
                                            {
                                                x_scaled = 270;
                                            }
                                            if (x == 781)
                                            {
                                                x_scaled = 271;
                                            }
                                            if (x == 783)
                                            {
                                                x_scaled = 272;
                                            }
                                            if (x == 785)
                                            {
                                                x_scaled = 273;
                                            }
                                            if (x == 787)
                                            {
                                                x_scaled = 274;
                                            }
                                            if (x == 790)
                                            {
                                                x_scaled = 275;
                                            }
                                            if (x == 792)
                                            {
                                                x_scaled = 276;
                                            }
                                            if (x == 794)
                                            {
                                                x_scaled = 277;
                                            }
                                            if (x == 796)
                                            {
                                                x_scaled = 278;
                                            }
                                            if (x == 799)
                                            {
                                                x_scaled = 279;
                                            }
                                        }
                                    }
                                    if (x > 800)
                                    {
                                        if (x < 822)
                                        {
                                            if (x == 801)
                                            {
                                                x_scaled = 280;
                                            }
                                            if (x == 803)
                                            {
                                                x_scaled = 281;
                                            }
                                            if (x == 805)
                                            {
                                                x_scaled = 282;
                                            }
                                            if (x == 808)
                                            {
                                                x_scaled = 283;
                                            }
                                            if (x == 810)
                                            {
                                                x_scaled = 284;
                                            }
                                            if (x == 812)
                                            {
                                                x_scaled = 285;
                                            }
                                            if (x == 814)
                                            {
                                                x_scaled = 286;
                                            }
                                            if (x == 817)
                                            {
                                                x_scaled = 287;
                                            }
                                            if (x == 819)
                                            {
                                                x_scaled = 288;
                                            }
                                            if (x == 821)
                                            {
                                                x_scaled = 289;
                                            }
                                        }
                                    }
                                    if (x > 822)
                                    {
                                        if (x < 849)
                                        {
                                            if (x == 823)
                                            {
                                                x_scaled = 290;
                                            }
                                            if (x == 825)
                                            {
                                                x_scaled = 291;
                                            }
                                            if (x == 828)
                                            {
                                                x_scaled = 292;
                                            }
                                            if (x == 830)
                                            {
                                                x_scaled = 293;
                                            }
                                            if (x == 832)
                                            {
                                                x_scaled = 294;
                                            }
                                            if (x == 834)
                                            {
                                                x_scaled = 295;
                                            }
                                            if (x == 837)
                                            {
                                                x_scaled = 296;
                                            }
                                            if (x == 839)
                                            {
                                                x_scaled = 297;
                                            }
                                            if (x == 841)
                                            {
                                                x_scaled = 298;
                                            }
                                            if (x == 843)
                                            {
                                                x_scaled = 299;
                                            }
                                            if (x == 846)
                                            {
                                                x_scaled = 300;
                                            }
                                            if (x == 848)
                                            {
                                                x_scaled = 301;
                                            }
                                        }
                                    }
                                    if (x > 849)
                                    {
                                        if (x < 864)
                                        {
                                            if (x == 850)
                                            {
                                                x_scaled = 302;
                                            }
                                            if (x == 852)
                                            {
                                                x_scaled = 303;
                                            }
                                            if (x == 854)
                                            {
                                                x_scaled = 304;
                                            }
                                            if (x == 857)
                                            {
                                                x_scaled = 305;
                                            }
                                            if (x == 859)
                                            {
                                                x_scaled = 306;
                                            }
                                            if (x == 861)
                                            {
                                                x_scaled = 307;
                                            }
                                            if (x == 863)
                                            {
                                                x_scaled = 308;
                                            }
                                        }
                                    }
                                    if (x > 865)
                                    {
                                        if (x < 889)
                                        {

                                            if (x == 866)
                                            {
                                                x_scaled = 309;
                                            }
                                            if (x == 868)
                                            {
                                                x_scaled = 310;
                                            }
                                            if (x == 870)
                                            {
                                                x_scaled = 311;
                                            }
                                            if (x == 872)
                                            {
                                                x_scaled = 312;
                                            }
                                            if (x == 875)
                                            {
                                                x_scaled = 313;
                                            }
                                            if (x == 877)
                                            {
                                                x_scaled = 314;
                                            }
                                            if (x == 879)
                                            {
                                                x_scaled = 315;
                                            }
                                            if (x == 881)
                                            {
                                                x_scaled = 316;
                                            }
                                            if (x == 884)
                                            {
                                                x_scaled = 317;
                                            }
                                            if (x == 886)
                                            {
                                                x_scaled = 318;
                                            }
                                            if (x == 888)
                                            {
                                                x_scaled = 319;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                pset(surface, x_scaled, y_scaled, color1);
            }
            x++;
            if (hsync)
            {
                ref_len++;
                if (ref_len > 30 && x > 700)
                {
                    //	pset(surface, x_scaled, y, 5);
                    y++;
                    x = 0;
                }
            }
            else
            {
                ref_len = 0;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <complex>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#define FONT "/usr/share/fonts/X11/TTF/luximb.ttf"
#define FONT_SIZE 12

using namespace std;

typedef enum type
{
	MANDELBROT = 0,
	JULIA,
	LAMBDA
} type;

//This struct contains all the data needed by the fractal thread
typedef struct Fractal_s
{
	SDL_Surface * screen;
	uint8_t * status; //When is 0, the thread is dead, when is 1 is running
	double iterations; //Maximum number of iterations to do to determine if the number considered is part of the Mandelbrot|Julia set, the higher is the number, higher will be the precision
	double min_real; //Lower bound of the real axis with which calculate the fractal
	double max_real; //Upper bound of the real axis with which calculate the fractal
	double min_im; //Lower bound of the imaginary axis with which calculate the fractal
	double max_im; //Upper bound of the imaginary axis with which calculate the fractal
	double rx; //Real part of the red C constant in the succession to be analyzed, needed only for the Julia set
	double ry; //Imaginary part of the red C constant in the succession to be analyzed, needed only for the Julia set
	double gx; //Real part of the green C constant in the succession to be analyzed, needed only for the Julia set
	double gy; //Imaginary part of the green C constant in the succession to be analyzed, needed only for the Julia set
	double bx; //Real part of the blue C constant in the succession to be analyzed, needed only for the Julia set
	double by; //Imaginary part of the blue C constant in the succession to be analyzed, needed only for the Julia set
	double exp; //Exponent of the Z in the succession
	type ty;
	bool metric;
}Fractal_s;

bool alive; //When false the fractal thread and the application will close

int fractal(void *); //Thread which shows the fractal
void putpixel(SDL_Surface *, uint16_t, uint16_t, uint8_t, uint8_t, uint8_t); //Put a single pixel on screen
void drawrectangle(SDL_Surface *, uint16_t, uint16_t, uint16_t, uint16_t, uint8_t, uint8_t, uint8_t); //Draws a rectangle
void putstring(SDL_Surface *, TTF_Font *, const char *, uint16_t, uint16_t, uint8_t, uint8_t, uint8_t); //Write something with SDL_ttf

int main(int argc, char ** argv)
{
	SDL_Surface * screen = NULL;
	SDL_Thread * thr = NULL;
	SDL_Event e;
	uint8_t status = 0;
	int pressed = 0;
	bool fullscreen = false;
	int c;
	Fractal_s t;
	int xres = 800, yres = 600;
	string res;

	//Default parameters
	t.iterations = 256;
	t.min_real = -2.0;
	t.max_real = 2.0;
	t.min_im = -2.0;
	t.max_im = 2.0;
	t.rx = -0.52;
	t.ry = 0.06;
	t.gx = -0.51;
	t.gy = 0.06;
	t.bx = -0.50;
	t.by = 0.06;
	t.exp = 2;
	t.ty = MANDELBROT;
	t.metric = false;

	for(int i = 0; i < argc; i++)
		if(!(string(argv[i]).compare("--help")))
		{
			cout << "Usage: " << argv[0] << " [options] | --help" << endl;
			cout << "Valid options are:" << endl;
			cout << "\t-v\t\tset the resolution" << endl;
			cout << "\t-F\t\tstart fullscreen" << endl;
			cout << "\t-i\t\tset the number of iterations" << endl;
			cout << "\t-X\t\tset the maximum real number" << endl;
			cout << "\t-x\t\tset the minimum real number" << endl;
			cout << "\t-Y\t\tset the maximum imaginary number" << endl;
			cout << "\t-y\t\tset the minimum imaginary number" << endl;
			cout << "\t-M\t\tshows the Mandelbrot set" << endl;
			cout << "\t-J\t\tshows the Julia set" << endl;
			cout << "\t-L\t\tshows the Lambda set" << endl;
			cout << "\t-m\t\tshows metrics" << endl;
			cout << "\t-r\t\tset the real part of the red c constant(Julia set)" << endl;
			cout << "\t-R\t\tset the imaginary part of the red c constant(Julia set)" << endl;
			cout << "\t-g\t\tset the real part of the green c constant(Julia set)" << endl;
			cout << "\t-G\t\tset the imaginary part of the green c constant(Julia set)" << endl;
			cout << "\t-b\t\tset the real part of the blue c constant(Julia set)" << endl;
			cout << "\t-B\t\tset the imaginary part of the blue c constant(Julia set)" << endl;
			cout << "\t-e\t\tset the exp of the z in the succession" << endl;

			return 0;
		}

	while ((c = getopt (argc, argv, "v:Fi:X:x:Y:y:MJLmr:R:g:G:b:B:e:")) != -1)
        {       
                switch(c)
                {       
                        case 'i':
                                t.iterations = atoi(optarg);
                                break;
                        case 'v':
				size_t index;
				//This part of code splits the string given as argument to obtain the x and the y resolution
                                res = optarg;
				index = res.find('x');
				xres = atoi(res.substr(0, index).c_str());
				yres = atoi(res.substr(index + 1).c_str());
                                break;
			case 'F':
				fullscreen = true;
				break;
			case 'X':
				t.max_real = atof(optarg);
				break;
			case 'x':
				t.min_real = atof(optarg);
				break;
			case 'Y':
				t.max_im = atof(optarg);
				break;
			case 'y':
				t.min_im = atof(optarg);
				break;
			case 'M':
				t.ty = MANDELBROT;
				break;
			case 'J':
				t.ty = JULIA;
				break;
			case 'L':
				t.ty = LAMBDA;
				break;
			case 'm':
				t.metric = true;
				break;
			case 'r':
				t.rx = atof(optarg);
				break;
			case 'R':
				t.ry = atof(optarg);
				break;
			case 'g':
				t.gx = atof(optarg);
				break;
			case 'G':
				t.gy = atof(optarg);
				break;
			case 'b':
				t.bx = atof(optarg);
				break;
			case 'B':
				t.by = atof(optarg);
				break;
			case 'e':
				t.exp = atof(optarg);
				break;
                        case '?':
				return -1;
                                break;
                }
        }

	if(SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		cout << "Impossibile inizializzare SDL: " << SDL_GetError() << endl;
        	exit(1);
    	}

	atexit(SDL_Quit);

	if(fullscreen == true)
		screen = SDL_SetVideoMode(xres, yres, 32, SDL_FULLSCREEN | SDL_HWSURFACE);
	else
		screen = SDL_SetVideoMode(xres, yres, 32, SDL_HWSURFACE);

	if(screen == NULL)
	{
		cout << "Impossibile settare il video: " << SDL_GetError() << endl;
		exit(1);
	}

	alive = true;
	t.screen = screen;
	status = 1;
	t.status = &status;
	thr = SDL_CreateThread(fractal, static_cast<void *>(&t));

	while(!pressed)
	{
		while(SDL_PollEvent(&e))
		{
			switch(e.type)
			{
				case SDL_KEYDOWN:
					switch(e.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						{
							pressed = 1;
							alive = false;
							break;
						}

						case SDLK_f:
						{
							SDL_WM_ToggleFullScreen(screen);
							break;
						}

						case SDLK_w:
						{
							if(*(t.status) == 0)
								SDL_SaveBMP(screen, "file.bmp");
							break;
						}

						case SDLK_m:
						{
							if(*(t.status) == 0)
							{
								t.metric = (t.metric == true) ? false : true;
								*(t.status) = 1;
								thr = SDL_CreateThread(fractal, static_cast<void *>(&t));
							}
							break;
						}
					}
					break;
				case SDL_QUIT:
					pressed = 1;
					alive = false;
					break;

				case SDL_MOUSEBUTTONDOWN:
					if(*(t.status) == 0)
					{
						switch(e.button.button)
						{
							case SDL_BUTTON_LEFT:
								//In this part of code, all the parameters are recalculated in function of the position of the mouse at the click, pratically, the minimum and the maximum are calculated to be proportional to the screen resolution and the mouse position
								t.min_real = (static_cast<double>(static_cast<int>(e.button.x) * abs(t.min_real - t.max_real)) / static_cast<double>(screen->w)) + (t.min_real) - (abs(t.min_real - t.max_real) / 2);
								t.max_real = (static_cast<double>(static_cast<int>(e.button.x) * abs(t.min_real - t.max_real)) / static_cast<double>(screen->w)) + (t.min_real) + (abs(t.min_real - t.max_real) / 2);
								t.min_im = abs(t.min_im - t.max_im) - (static_cast<double>(static_cast<int>(e.button.y) * abs(t.min_im - t.max_im)) / static_cast<double>(screen->h)) + (t.min_im) - (abs(t.min_im - t.max_im) / 2);
								t.max_im = abs(t.min_im - t.max_im) - (static_cast<double>(static_cast<int>(e.button.y) * abs(t.min_im - t.max_im)) / static_cast<double>(screen->h)) + (t.min_im) + (abs(t.min_im - t.max_im) / 2);

								*(t.status) = 1;
								thr = SDL_CreateThread(fractal, static_cast<void *>(&t));
								break;
							case SDL_BUTTON_WHEELUP:
								t.min_real = (static_cast<double>(static_cast<int>(e.button.x) * abs(t.min_real - t.max_real)) / static_cast<double>(screen->w)) + (t.min_real) - (abs(t.min_real - t.max_real) / 4);
								t.max_real = (static_cast<double>(static_cast<int>(e.button.x) * abs(t.min_real - t.max_real)) / static_cast<double>(screen->w)) + (t.min_real) + (abs(t.min_real - t.max_real) / 4);
								t.min_im = abs(t.min_im - t.max_im) - (static_cast<double>(static_cast<int>(e.button.y) * abs(t.min_im - t.max_im)) / static_cast<double>(screen->h)) + (t.min_im) - (abs(t.min_im - t.max_im) / 4);
								t.max_im = abs(t.min_im - t.max_im) - (static_cast<double>(static_cast<int>(e.button.y) * abs(t.min_im - t.max_im)) / static_cast<double>(screen->h)) + (t.min_im) + (abs(t.min_im - t.max_im) / 4);

								*(t.status) = 1;
								thr = SDL_CreateThread(fractal, static_cast<void *>(&t));
								break;

							case SDL_BUTTON_WHEELDOWN:
								t.min_real = (static_cast<double>(static_cast<int>(e.button.x) * abs(t.min_real - t.max_real)) / static_cast<double>(screen->w)) + (t.min_real) - abs(t.min_real - t.max_real);
								t.max_real = (static_cast<double>(static_cast<int>(e.button.x) * abs(t.min_real - t.max_real)) / static_cast<double>(screen->w)) + (t.min_real) + abs(t.min_real - t.max_real);
								t.min_im = abs(t.min_im - t.max_im) - (static_cast<double>(static_cast<int>(e.button.y) * abs(t.min_im - t.max_im)) / static_cast<double>(screen->h)) + (t.min_im) - abs(t.min_im - t.max_im);
								t.max_im = abs(t.min_im - t.max_im) - (static_cast<double>(static_cast<int>(e.button.y) * abs(t.min_im - t.max_im)) / static_cast<double>(screen->h)) + (t.min_im) + abs(t.min_im - t.max_im);

								*(t.status) = 1;
								thr = SDL_CreateThread(fractal, static_cast<void *>(&t));
								break;
						}
					}
					break;

			}
		}
		SDL_Delay(50);
	}

	SDL_WaitThread(thr, NULL);
	SDL_FreeSurface(screen);

	return 0;
}

int fractal(void * s)
{
	Fractal_s * t = static_cast<Fractal_s *>(s);
	TTF_Font* font;
	double X, Y;
	double marginx = (t->screen->w / 30), marginy = (t->screen->h / 30);
	stringstream ss;

	//Max 4 numbers to be used in a double
	ss.setf(ios::fixed,ios::floatfield);
	ss.precision(4);

	TTF_Init();

	if(t->metric == false)
		marginx = marginy = 0;

	if((font = TTF_OpenFont(FONT, FONT_SIZE)) == NULL)
	{
		cout << "Impossibile caricare il font: " << SDL_GetError() << endl;
		return -1;
	}

	SDL_FillRect(t->screen, NULL, SDL_MapRGB(t->screen->format, 0xFF, 0xFF, 0xFF)); //White background
	SDL_Flip(t->screen);

	for(uint16_t x = marginx; x < (t->screen->w - marginx); x++)
	{
		for(uint16_t y = marginy; y < (t->screen->h - marginy); y++)
		{
			if(alive == false)
				return -1;

			//X and Y are the real and the imaginary coords calculated proportionally to the screen coords
			X = (static_cast<double>(x * abs(t->min_real - t->max_real)) / static_cast<double>(t->screen->w)) + (t->min_real);
			Y = abs(t->min_im - t->max_im) - (static_cast<double>(y * abs(t->min_im - t->max_im)) / static_cast<double>(t->screen->h)) + (t->min_im);
			complex<double> z;
			complex<double> c;
			int countR = 0;
			int countG = 0;
			int countB = 0;

			switch(t->ty)
			{
				case JULIA:
				{
					c = complex<double>(t->rx, t->ry);
					z = complex<double>(X, Y);

					while((abs(z) <= 2) && (countR < t->iterations))
					{
						z = pow(z, t->exp) + c;
						++countR;
					}
	
					c = complex<double>(t->gx, t->gy);
					z = complex<double>(X, Y);
					while((abs(z) <= 2) && (countG < t->iterations))
					{
						z = pow(z, t->exp) + c;
						++countG;
					}
	
					c = complex<double>(t->bx, t->by);
					z = complex<double>(X, Y);
					while((abs(z) <= 2) && (countB < t->iterations))
					{
						z = pow(z, t->exp) + c;
						++countB;
					}
					break;
				}
				case MANDELBROT:
				{
					c = complex<double>(X, Y);
					z = complex<double>(0, 0);
					while((abs(z) <= 2) && (countR < t->iterations))
					{
						z = pow(z, t->exp) + c;
						++countR;
					}
	
					c = complex<double>(X, Y);
					z = complex<double>(0, 0);
					while((abs(z) <= 2) && (countG < t->iterations))
					{
						z = pow(z, t->exp) + c;
						++countG;
					}

					c = complex<double>(X, Y);
					z = complex<double>(0, 0);
					while((abs(z) <= 2) && (countB < t->iterations))
					{
						z = pow(z, t->exp) + c;
						++countB;
					}
					break;
				}
				case LAMBDA:
				{
					z = complex<double>(X, Y);
					while((abs(z) <= 2) && (countR < t->iterations))
					{
						z = complex<double>(0.85, 0.6)*z*(complex<double>(1, 0) - z);
						++countR;
					}
	
					z = complex<double>(X, Y);
					while((abs(z) <= 2) && (countG < t->iterations))
					{
						z = complex<double>(0.86, 0.6)*z*(complex<double>(1, 0) - z);
						++countG;
					}

					z = complex<double>(X, Y);
					while((abs(z) <= 2) && (countB < t->iterations))
					{
						z = complex<double>(0.87, 0.6)*z*(complex<double>(1, 0) - z);
						++countB;
					}
					break;
				}
			}

			//Colors are based on the number of iterations
			putpixel(t->screen, x, y, countR, countG, countB);
		}

		//Every (t->screen->w / 80) the screen is updated
		if(!(x % (t->screen->w / 80)))
			SDL_Flip(t->screen);
	}

	if(t->metric == true)
	{
		//Draws the outer rectangle
		drawrectangle(t->screen, t->screen->w/2, t->screen->h/2, t->screen->w - (2*marginx), t->screen->h - (2*marginy), 0x00, 0x00, 0x00);

		TTF_CloseFont(font);
		font = TTF_OpenFont(FONT, FONT_SIZE / 1.4); //Font is reopen to be more accurate

		//These cicles draws lines around the outer rectangle
		for(uint16_t x = marginx; x < (t->screen->w - marginx); x++)
			if(!(x % (t->screen->w / 10)))
			{
				drawrectangle(t->screen, x, marginy, 1, 5, 0x00, 0x00, 0x00);
				drawrectangle(t->screen, x, t->screen->h - marginy, 1, 5, 0x00, 0x00, 0x00);
			}

		for(uint16_t y = marginx; y < (t->screen->h - marginy); y++)
			if(!(y % (t->screen->h / 10)))
			{
				drawrectangle(t->screen, marginx, y, 5, 1, 0x00, 0x00, 0x00);
				drawrectangle(t->screen, t->screen->w - marginx, y, 5, 1, 0x00, 0x00, 0x00);
			}

		for(uint16_t x = marginx; x < (t->screen->w - marginx); x++)
		{
			X = (static_cast<double>(x * abs(t->min_real - t->max_real)) / static_cast<double>(t->screen->w)) + (t->min_real);

			if(!(x % (t->screen->w / 10)))
			{
				ss << X;
				putstring(t->screen, font, ss.str().c_str(), x, t->screen->h - (FONT_SIZE/2), 0x00, 0x00, 0x00);
				ss.str("");
			}
		}

		for(uint16_t y = marginy; y < (t->screen->h - marginy); y++)
		{
			Y = abs(t->min_im - t->max_im) - (static_cast<double>(y * abs(t->min_im - t->max_im)) / static_cast<double>(t->screen->h)) + (t->min_im);

			if(!(y % (t->screen->h / 10)))
			{
				ss << Y;
				putstring(t->screen, font, ss.str().c_str(), marginx/2, y, 0x00, 0x00, 0x00);
				ss.str("");
			}
		}
	}

	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Flip(t->screen);
	*(t->status) = 0;
}

void putpixel(SDL_Surface * screen, uint16_t x, uint16_t y, uint8_t R, uint8_t G, uint8_t B)
{
	int bpp = screen->format->BytesPerPixel;
	uint32_t * ptr = NULL;

	if((x <= 0) || (y <= 0))
		return;

	if((x >= screen->w) || (y >= screen->h))
		return;

	ptr = (uint32_t *)screen->pixels + y*screen->pitch/bpp + x;
	*ptr = SDL_MapRGB(screen->format, R, G, B);
}

void putstring(SDL_Surface * screen, TTF_Font * font, const char * string, uint16_t x, uint16_t y, uint8_t R, uint8_t G, uint8_t B)
{
	SDL_Surface * fontSurface;
	SDL_Color color;
	SDL_Rect fontRect;

	color.r = R;
	color.g = G;
	color.b = B;

	fontSurface = TTF_RenderText_Solid(font, string, color);
        fontRect.x = x - (fontSurface->w / 2);
        fontRect.y = y - (fontSurface->h / 2);

        SDL_BlitSurface(fontSurface, NULL, screen, &fontRect);
	SDL_FreeSurface(fontSurface);
}

void drawrectangle(SDL_Surface * screen, uint16_t x, uint16_t y, uint16_t base, uint16_t height, uint8_t R, uint8_t G, uint8_t B)
{
	int i;
	SDL_Rect rect;

	rect.x = x - (base / 2);
	rect.y = y - (height / 2);
	rect.w = base;
	rect.h = height;

	for(i = 0; i < base; i++)
		putpixel(screen, (rect.x + i), rect.y, R, G, B);

	for(i = 0; i < base; i++)
		putpixel(screen, (rect.x + i), (rect.y + height - 1), R, G, B);

	for(i = 0; i < height; i++)
		putpixel(screen, rect.x, (rect.y + i), R, G, B);

	for(i = 0; i < height; i++)
		putpixel(screen, (rect.x + base - 1), (rect.y + i), R, G, B);


	SDL_UpdateRects(screen, 1, &rect);
}

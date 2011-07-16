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
	double cx; //Real part of the C constant in the succession to be analyzed, needed only for the Julia set
	double cy; //Imaginary part of the C constant in the succession to be analyzed, needed only for the Julia set
	bool isJulia; //If true, associated Julia set will be shown, instead if false, will be shown the associated Mandelbrot set
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
	t.cx = 0;
	t.cy = 1;
	t.isJulia = false;

	for(int i = 0; i < argc; i++)
		if(!(string(argv[i]).compare("--help")))
		{
			cout << "Usage: " << argv[0] << " [options] | --help" << endl;
			cout << "Valid options are:" << endl;
			cout << "\t-r\t\tset the resolution" << endl;
			cout << "\t-i\t\tset the number of iterations" << endl;
			cout << "\t-X\t\tset the maximum real number" << endl;
			cout << "\t-x\t\tset the minimum real number" << endl;
			cout << "\t-Y\t\tset the maximum imaginary number" << endl;
			cout << "\t-y\t\tset the minimum imaginary number" << endl;
			cout << "\t-J\t\tshows the Julia set" << endl;
			cout << "\t-c\t\tset the real part of the c constant(Julia set)" << endl;
			cout << "\t-C\t\tset the imaginary part of the c constant(Julia set)" << endl;

			return 0;
		}

	while ((c = getopt (argc, argv, "r:i:X:x:Y:y:Jc:C:")) != -1)
        {       
                switch(c)
                {       
                        case 'i':
                                t.iterations = atoi(optarg);
                                break;
                        case 'r':
				size_t index;
				//This part of code splits the string given as argument to obtain the x and the y resolution
                                res = optarg;
				index = res.find('x');
				xres = atoi(res.substr(0, index).c_str());
				yres = atoi(res.substr(index + 1).c_str());
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
			case 'J':
				t.isJulia = true;
				break;
			case 'c':
				t.cx = atof(optarg);
				break;
			case 'C':
				t.cy = atof(optarg);
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
	stringstream ss;

	//Max 4 numbers to be used in a double
	ss.setf(ios::fixed,ios::floatfield);
	ss.precision(4);

	TTF_Init();

	if((font = TTF_OpenFont(FONT, FONT_SIZE)) == NULL)
	{
		cout << "Impossibile caricare il font: " << SDL_GetError() << endl;
		return -1;
	}

	SDL_FillRect(t->screen, NULL, SDL_MapRGB(t->screen->format, 0xFF, 0xFF, 0xFF)); //White background
	SDL_Flip(t->screen);

	//(t->screen->w / 30) represents the margin, on left and on right to be shown for the "metric"
	for(uint16_t x = (t->screen->w / 30); x < (t->screen->w - (t->screen->w / 30)); x++)
	{
		for(uint16_t y = (t->screen->h / 30); y < (t->screen->h - (t->screen->h / 30)); y++)
		{
			if(alive == false)
				return -1;

			//X and Y are the real and the imaginary coords calculated proportionally to the screen coords
			X = (static_cast<double>(x * abs(t->min_real - t->max_real)) / static_cast<double>(t->screen->w)) + (t->min_real);
			Y = abs(t->min_im - t->max_im) - (static_cast<double>(y * abs(t->min_im - t->max_im)) / static_cast<double>(t->screen->h)) + (t->min_im);
			complex<double> z;
			complex<double> c;
			int count = 1;

			if(t->isJulia == true)
			{
				c = complex<double>(t->cx, t->cy);
				z = complex<double>(X, Y);
			}
			else
				z = c = complex<double>(X, Y);

			while((abs(z) <= 2) && (count < t->iterations))
			{
				z = (z*z) + c;
				++count;
			}

			//Colors are based on the number of iterations
			putpixel(t->screen, x, y, count, count, 2*count);
		}

		//Every (t->screen->w / 80) the screen is updated
		if(!(x % (t->screen->w / 80)))
			SDL_Flip(t->screen);
	}

	//Draws the outer rectangle
	drawrectangle(t->screen, t->screen->w/2, t->screen->h/2, t->screen->w - (2*(t->screen->w/30)), t->screen->h - (2*(t->screen->h/30)), 0x00, 0x00, 0x00);

	TTF_CloseFont(font);
	font = TTF_OpenFont(FONT, FONT_SIZE / 1.4); //Font is reopen to be more accurate

	//These cicles draws lines around the outer rectangle
	for(uint16_t x = (t->screen->w / 30); x < (t->screen->w - (t->screen->w / 30)); x++)
		if(!(x % (t->screen->w / 10)))
		{
			drawrectangle(t->screen, x, (t->screen->h/30), 1, 5, 0x00, 0x00, 0x00);
			drawrectangle(t->screen, x, t->screen->h - (t->screen->h/30), 1, 5, 0x00, 0x00, 0x00);
		}

	for(uint16_t y = (t->screen->h / 30); y < (t->screen->h - (t->screen->h / 30)); y++)
		if(!(y % (t->screen->h / 10)))
		{
			drawrectangle(t->screen, (t->screen->w/30), y, 5, 1, 0x00, 0x00, 0x00);
			drawrectangle(t->screen, t->screen->w - (t->screen->w/30), y, 5, 1, 0x00, 0x00, 0x00);
		}

	for(uint16_t x = (t->screen->w / 30); x < (t->screen->w - (t->screen->w / 30)); x++)
	{
		X = (static_cast<double>(x * abs(t->min_real - t->max_real)) / static_cast<double>(t->screen->w)) + (t->min_real);

		if(!(x % (t->screen->w / 10)))
		{
			ss << X;
			putstring(t->screen, font, ss.str().c_str(), x, t->screen->h - (FONT_SIZE/2), 0x00, 0x00, 0x00);
			ss.str("");
		}
	}

	for(uint16_t y = (t->screen->h / 30); y < (t->screen->h - (t->screen->h / 30)); y++)
	{
		Y = abs(t->min_im - t->max_im) - (static_cast<double>(y * abs(t->min_im - t->max_im)) / static_cast<double>(t->screen->h)) + (t->min_im);

		if(!(y % (t->screen->h / 10)))
		{
			ss << Y;
			putstring(t->screen, font, ss.str().c_str(), (t->screen->w/30)/2, y, 0x00, 0x00, 0x00);
			ss.str("");
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

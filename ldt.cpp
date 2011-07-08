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

//Struttura da passare al thread con i dati necessari
typedef struct Thread
{
	SDL_Surface * screen;
	double iterations; //Numero massimo di iterazioni da compiere per determinare l'appartenenza della successione all'insieme di Mandelbrot
	double min_real; //Estremo inferiore dell'asse reale su cui calcolare il frattale
	double max_real; //Estremo superiore dell'asse reale su cui calcolare il frattale
	double min_im; //Estremo inferiore dell'asse immaginario su cui calcolare il frattale
	double max_im; //Estremo superiore dell'asse immaginario su cui calcolare il frattale
}Thread;

bool alive; //False quando si chiude l'applicazione, il thread termina in caso di false

int mandelbrot(void *); //Thread che genera il frattale
void putpixel(SDL_Surface *, uint16_t, uint16_t, uint8_t, uint8_t, uint8_t); //Funzione che mette un singolo pixel a schermo
void drawrectangle(SDL_Surface *, uint16_t, uint16_t, uint16_t, uint16_t, uint8_t, uint8_t, uint8_t); //Disegna un rettangolo
void putstring(SDL_Surface *, TTF_Font *, const char *, uint16_t, uint16_t, uint8_t, uint8_t, uint8_t); //Scrive qualcosa con le SDL_ttf

int main(int argc, char ** argv)
{
	SDL_Surface * screen = NULL;
	SDL_Thread * thr = NULL;
	SDL_Event e; //Gestore eventi
	int pressed = 0;
	int c;
	Thread t;
	int xres = 800, yres = 600;
	string res;

	//Parametri di default
	t.iterations = 256;
	t.min_real = -2.0;
	t.max_real = 0.8;
	t.min_im = -1.4;
	t.max_im = 1.4;

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
			return 0;
		}

	while ((c = getopt (argc, argv, "r:i:X:x:Y:y:")) != -1)
        {       
                switch(c)
                {       
                        case 'i':
                                t.iterations = atoi(optarg);
                                break;
                        case 'r':
				size_t index;
				//Split della stringa relativa alla risoluzione in modo da ottenere la larghezza e l'altezza
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
	thr = SDL_CreateThread(mandelbrot, static_cast<void *>(&t));

	while(!pressed)
	{
		while(SDL_PollEvent(&e))
		{
			switch(e.type)
			{
				case SDL_QUIT:
					pressed = 1;
					alive = false; //Comunica al thread la chiusura dell'applicazione
					break;

			}
		}
		SDL_Delay(50);
	}

	SDL_WaitThread(thr, NULL);
	SDL_FreeSurface(screen);

	return 0;
}

int mandelbrot(void * s)
{
	Thread * t = static_cast<Thread *>(s);
	TTF_Font* font;
	double X, Y;
	stringstream ss;

	//Max 4 numeri da introdurre da un double
	ss.setf(ios::fixed,ios::floatfield);
	ss.precision(4);

	TTF_Init();

	if((font = TTF_OpenFont(FONT, FONT_SIZE)) == NULL)
	{
		cout << "Impossibile caricare il font: " << SDL_GetError() << endl;
		return -1;
	}

	SDL_FillRect(t->screen, NULL, SDL_MapRGB(t->screen->format, 0xFF, 0xFF, 0xFF)); //Coloro lo schermo di bianco
	SDL_Flip(t->screen);

	//(t->screen->w / 30) rappresenta il margine da lasciare a sinistra e a destra per la metrica, idem per (t->screen->h / 30)[in alto e in basso]
	for(uint16_t x = (t->screen->w / 30); x < (t->screen->w - (t->screen->w / 30)); x++)
	{
		for(uint16_t y = (t->screen->h / 30); y < (t->screen->h - (t->screen->h / 30)); y++)
		{
			if(alive == false)
				return -1;

			//X e Y sono le coordinate reali e immaginarie calcolate in modo da essere proporzionali alla dimensione dello schermo
			X = (static_cast<double>(x * abs(t->min_real - t->max_real)) / static_cast<double>(t->screen->w)) + (t->min_real);
			Y = abs(t->min_im - t->max_im) - (static_cast<double>(y * abs(t->min_im - t->max_im)) / static_cast<double>(t->screen->h)) + (t->min_im);
			complex<double> z;
			complex<double> c = complex<double>(X, Y);
			int count = 1;
			
			//Successione di Mandelbrot z(n + 1) = z(n)² + c
			//Se il modulo del numero complesso z(n + 1) non diverge, ovvero rimane minore di 2, z(n + 1) appartiene all'insieme
			//Count rappresenta il numero di iterazioni dopo il quale la successione diverge
			z = c;
			while((abs(z) <= 2) && (count < t->iterations))
			{
				z = (z*z) + c;
				++count;
			}

			//Colori basati su count
			putpixel(t->screen, x, y, count, count, 2*count);
		}

		//Fa in modo che ogni (t->screen->w / 80) lo schermo venga aggiornato
		if(!(x % (t->screen->w / 80)))
			SDL_Flip(t->screen);
	}

	//Disegna il rettangolo per la metrica
	drawrectangle(t->screen, t->screen->w/2, t->screen->h/2, t->screen->w - (2*(t->screen->w/30)), t->screen->h - (2*(t->screen->h/30)), 0x00, 0x00, 0x00);

	TTF_CloseFont(font);
	font = TTF_OpenFont(FONT, FONT_SIZE / 1.4); //Font ridotto per comodità

	//Questo ciclo disegna le linee di misurazione in alto e in basso
	for(uint16_t x = (t->screen->w / 30); x < (t->screen->w - (t->screen->w / 30)); x++)
		if(!(x % (t->screen->w / 10)))
		{
			//drawrectangle(t->screen, x - (t->screen->w/40), (t->screen->h/40), 1, 5, 0x00, 0x00, 0x00);
			//drawrectangle(t->screen, x - (t->screen->w/40), t->screen->h - (t->screen->h/40), 1, 5, 0x00, 0x00, 0x00);
			drawrectangle(t->screen, x, (t->screen->h/30), 1, 5, 0x00, 0x00, 0x00);
			drawrectangle(t->screen, x, t->screen->h - (t->screen->h/30), 1, 5, 0x00, 0x00, 0x00);
		}

	//Questo ciclo disegna le linee di misurazione a sinistra e a destra
	for(uint16_t y = (t->screen->h / 30); y < (t->screen->h - (t->screen->h / 30)); y++)
		if(!(y % (t->screen->h / 10)))
		{
			//drawrectangle(t->screen, (t->screen->w/40), y - (t->screen->h/40), 5, 1, 0x00, 0x00, 0x00);
			//drawrectangle(t->screen, t->screen->w - (t->screen->w/40), y - (t->screen->h/40), 5, 1, 0x00, 0x00, 0x00);
			drawrectangle(t->screen, (t->screen->w/30), y, 5, 1, 0x00, 0x00, 0x00);
			drawrectangle(t->screen, t->screen->w - (t->screen->w/30), y, 5, 1, 0x00, 0x00, 0x00);
		}

	//A fianco alle linee di misurazione disegnate in precedenza viene posto il valore della X
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

	//A fianco alle linee di misurazione disegnate in precedenza viene posto il valore della Y
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

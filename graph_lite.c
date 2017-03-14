#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>
#include <termios.h>
#include <pthread.h>
#include <string.h>

int fbfd = 0;                       // Filebuffer Filedescriptor
struct fb_var_screeninfo vinfo;     // Struct Variable Screeninfo
struct fb_fix_screeninfo finfo;     // Struct Fixed Screeninfo
long int screensize = 0;            // Ukuran data framebuffer
char *fbp = 0;                      // Framebuffer di memori internal

int borderUp = 10;        	       // The border width, distance from the actual screenBorder
int borderDown = 10;               // The border width, distance from the actual screenBorder
int borderLeft = 10;               // The border width, distance from the actual screenBorder
int borderRight = 10;              // The border width, distance from the actual screenBorder

int xmiddle;
int ymiddle;

void drawScreenBorder();
void moveAll(int dx, int dy);
void drawRotateWindmills();
void fireMonster();

// Warna background layar
int rground=0, gground=0, bground=0, aground=0;

// UTILITY PROCEDURE----------------------------------------------------------------------------------------- //
int isOverflow(int _x , int _y) {
//Cek apakah kooordinat (x,y) sudah melewati batas layar
    int result;
    if ( _x > vinfo.xres-borderDown || _y > vinfo.yres-borderRight || _x < borderUp-1 || _y < borderLeft-1 ) {
        result = 1;
    }
    else {
        result = 0;
    }
    return result;
}

void terminate() {
	//Pengakhiran program.
     munmap(fbp, screensize);
     close(fbfd);
}

/*char getKeyPress() {
    char buf = 0;
    struct termios old = {0};
    struct termios new = {0};
    if (tcgetattr(0, &old) < 0)
            perror("tcsetattr()");
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_lflag &= ~ECHO;
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &new) < 0)
            perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
            perror ("read()");
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
            perror ("tcsetattr ~ICANON");
    return (buf);
}*/

//KEYPRESS HANDLER==============================
static struct termios old, news;

/*Init Termios*/
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  news = old; /* make new settings same as old settings */
  news.c_lflag &= ~ICANON; /* disable buffered i/o */
  news.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &news); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}
//=============================================


// PROSEDUR RESET LAYAR DAN PEWARNAAN PIXEL----------------------------------------------------------------- //
int plotPixelRGBA(int _x, int _y, int r, int g, int b, int a) {
	//Plot pixel (x,y) dengan warna (r,g,b,a)
	if(!isOverflow(_x,_y)) {
		long int location = (_x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (_y+vinfo.yoffset) * finfo.line_length;
		if(vinfo.bits_per_pixel == 32) {
			*(fbp + location) = b;        //blue
			*(fbp + location + 1) = g;    //green
			*(fbp + location + 2) = r;    //red
			*(fbp + location + 3) = a;      //
		} else {
            *(fbp + location) = b;        //blue
			*(fbp + location + 1) = g;    //green
			*(fbp + location + 2) = r;    //red
		}
	}
}

int isPixelColor(int _x, int _y, int r, int g, int b, int a) {
	if(!isOverflow(_x,_y)) {
		long int location = (_x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (_y+vinfo.yoffset) * finfo.line_length;
		if(vinfo.bits_per_pixel == 32) {
			if((*((unsigned char *)(fbp + location)) == b)&&(*((unsigned char *)(fbp + location + 1)) == g)&&(*((unsigned char *)(fbp + location + 2)) == r)&&(*((unsigned char *)(fbp + location + 3)) == a)) {
				return 1;
			} else return 0;
		} else {
			if((*((unsigned char *)(fbp + location)) == b)&&(*((unsigned char *)(fbp + location + 1)) == g)&&(*((unsigned char *)(fbp + location + 2)) == r)) {
				return 1;
			} else return 0;
		}
	} else return 0;
}

void initScreen() {
	 // Mapping framebuffer ke memori internal
     // Buka framebuffer
     fbfd = open("/dev/fb0", O_RDWR);
     if (fbfd == -1) {
         perror("Error: cannot open framebuffer device");
         exit(1);
     }
     printf("The framebuffer device was opened successfully.\n");

     // Ambil data informasi screen
     if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         perror("Error reading fixed information");
         exit(2);
     }
     if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         perror("Error reading variable information");
         exit(3);
     }

     // Informasi resolusi, dan bit per pixel
     printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

     // Hitung ukuran memori untuk menyimpan framebuffer
     screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

     // Masukkan framebuffer ke memory, untuk kita ubah-ubah
     fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fbfd, 0);
     if ((int)fbp == -1) {
         perror("Error: failed to map framebuffer device to memory");
         exit(4);
     }
     printf("The framebuffer device was mapped to memory successfully.\n");

     if ((!vinfo.bits_per_pixel == 32)||(!vinfo.bits_per_pixel == 24)) {
		perror("bpp NOT SUPPORTED");
		exit(0);
	 }

     xmiddle = vinfo.xres/2;
     ymiddle = vinfo.yres/2;
     //borderDown = vinfo.xres - (borderUp + 600);
     //borderRight = vinfo.yres - (borderLeft + 600);

}

void clearScreen() {

	//Mewarnai latar belakang screen dengan warna putih
    int x = 0;
    int y = 0;
    for (y = 0; y < vinfo.yres; y++) {
		for (x = 0; x < vinfo.xres; x++) {
			plotPixelRGBA(x,y,0,0,0,0);
        }
	}
	drawScreenBorder();

}

// STRUKTUR DATA DAN METODE PENGGAMBARAN GARIS-------------------------------------------------------------- //
typedef struct {

  int x1; int y1; // Koordinat titik pertama garis
  int x2; int y2; // Koordinat titik kedua garis

  // Encoding warna garis dalam RGBA / RGB
  int r; int g; int b; int a;

} Line;

void initLine(Line* l, int xa, int ya, int xb, int yb, int rx, int gx, int bx, int ax) {
	(*l).x1 = xa; (*l).y1 = ya;
	(*l).x2 = xb; (*l).y2 = yb;
	(*l).r = rx; (*l).g = gx; (*l).b = bx; (*l).a = ax;
}

int getClipCode(int x, int y) {
	int code = 0;

	int xmin = borderUp-1, xmax = vinfo.xres-borderDown;
	int ymin = borderLeft-1, ymax = vinfo.yres-borderRight;

	if (x < xmin)           // to the left of clip window 0001
		code |= 1;
	else if (x > xmax)      // to the right of clip window 0010
		code |= 2;
	if (y < ymin)           // below the clip window 0100
		code |= 4;
	else if (y > ymax)      // above the clip window 1000
		code |= 8;

	return code;
}

int drawLine(Line* l) {
	int x1 = (*l).x1, y1 = (*l).y1;
	int x2 = (*l).x2, y2 = (*l).y2;
	int xmin = borderUp-1, xmax = vinfo.xres-borderDown;
	int ymin = borderLeft-1, ymax = vinfo.yres-borderRight;
	int x, y;

	int code1 = getClipCode(x1, y1);
	int code2 = getClipCode(x2, y2);

	int accept = 0;
	int valid = 1;

	if(isOverflow((*l).x1, (*l).y1) || isOverflow((*l).x2, (*l).y2)) {
		while(1) {
			if (!(code1 | code2)) { // Kedua endpoint di dalam batas, keluar loop & print
				accept = 1;
				break;
			} else if (code1 & code2) { // Kedua endpoint di region yang sama diluar batas
				break;
			} else {

				//Salah satu endpoint ada di luar batas
				int code = code1 ? code1 : code2;

				//Cara perpotongan menggunakan persamaan garis
				if (code & 8) {           // endpoint di atas area clip
					x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
					y = ymax;
				} else if (code & 4) { // endpoint di bawah area clip
					x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
					y = ymin;
				} else if (code & 2) {  // endpoint di sebelah kanan area clip
					y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
					x = xmax;
				} else if (code & 1) {   // endpoint di sebelah kiri area clip
					y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
					x = xmin;
				}

				//Pindahkan point yang ada di luar area ke dalam
				if (code == code1) {
					x1 = x;
					y1 = y;
					code1 = getClipCode(x1, y1);
				} else {
					x2 = x;
					y2 = y;
					code2 = getClipCode(x2, y2);
				}
			}
		}

		if(accept) {
			(*l).x1 = x1;
			(*l).y1 = y1;
			(*l).x2 = x2;
			(*l).y2 = y2;
			return drawLine(l);
		}
	}



	int col = 0;

	// Coord. of the next point to be displayed
	x = (*l).x1;
	y = (*l).y1;

	// Calculate initial error factor
	int dx = abs((*l).x2 - (*l).x1);
	int dy = abs((*l).y2 - (*l).y1);
	int p = 0;

	// If the absolute gradien is less than 1
	if(dx >= dy) {

		// Memastikan x1 < x2
		if((*l).x1 > (*l).x2) {
			(*l).x1 = (*l).x2;
			(*l).y1 = (*l).y2;
			(*l).x2 = x;
			(*l).y2 = y;
			x = (*l).x1;
			y = (*l).y1;
		}

		// Repeat printing the next pixel until the line is painted
		while(x <= (*l).x2) {

			// Draw the next pixel
			if (!isOverflow(x,y)) {
				plotPixelRGBA(x,y,(*l).r,(*l).g,(*l).b,(*l).a);
			}

			// Calculate the next pixel
			if(p < 0) {
				p = p + 2*dy;
			} else {
				p = p + 2*(dy-dx);
				if((*l).y2 - (*l).y1 > 0) y++;
				else y--;
			}
			x++;

		}

	// If the absolute gradien is more than 1
	} else {

		// Memastikan y1 < y2
		if((*l).y1 > (*l).y2) {
			(*l).x1 = (*l).x2;
			(*l).y1 = (*l).y2;
			(*l).x2 = x;
			(*l).y2 = y;
			x = (*l).x1;
			y = (*l).y1;
		}

		// Repeat printing the next pixel until the line is painted
		while(y <= (*l).y2) {

			// Draw the next pixel
			if (!isOverflow(x,y)) {
				plotPixelRGBA(x,y,(*l).r,(*l).g,(*l).b,(*l).a);
			}

			// Calculate the next pixel
			if(p < 0) {
				p = p + 2*dx;
			} else {
				p = p + 2*(dx-dy);
				if((*l).x2 - (*l).x1 > 0) x++;
				else x--;
			}
			y++;

		}

	}

	return col;

}

// METODE PEWARNAAN UMUM------------------------------------------------------------------------------------ //
void floodFill(int x,int y, int r,int g,int b,int a, int rb,int gb,int bb,int ab) {
// rgba is the color of the fill, rbgbbbab is the color of the border
	if((!isOverflow(x,y)) ) {
		if(!isPixelColor(x,y, r,g,b,a)) {
			if(!isPixelColor(x,y, rb,gb,bb,ab)) {

				plotPixelRGBA(x,y, r,g,b,a);
				floodFill(x+1,y, r,g,b,a, rb,gb,bb,ab);
				floodFill(x-1,y, r,g,b,a, rb,gb,bb,ab);
				floodFill(x,y+1, r,g,b,a, rb,gb,bb,ab);
				floodFill(x,y-1, r,g,b,a, rb,gb,bb,ab);

			}
		}
	}

}

// STRUKTUR DATA DAN METODE PENGGAMBARAN POLYLINE----------------------------------------------------------- //
typedef struct {

	int x[100];
	int y[100];

	// The coord. of this Polyline firepoint
	int xp; int yp;

	int PointCount; // Jumlah end-point dalam polyline, selalu >1

	// Encoding warna outline dalam RGBA / RGB
	int r; int g; int b; int a;

} PolyLine;

void initPolyline(PolyLine* p, int rx, int gx, int bx, int ax) {
// Reset Polyline dan men-set warna garis batas
	(*p).PointCount = 0;
	(*p).r = rx; (*p).g = gx; (*p).b = bx; (*p).a = ax;
}

void addEndPoint(PolyLine* p, int _x, int _y) {
	(*p).x[(*p).PointCount] = _x;
	(*p).y[(*p).PointCount] = _y;
	(*p).PointCount++;
}

void setFirePoint(PolyLine* p, int x, int y) {
	(*p).xp = x;
	(*p).yp = y;
}

void boxPolyline(PolyLine* p, int x1, int y1, int x2, int y2) {
	addEndPoint(p, x1, y1);
	addEndPoint(p, x1, y2);
	addEndPoint(p, x2, y2);
	addEndPoint(p, x2, y1);
	int xp = (x1+x2)/2;
	int yp = (y1+y2)/2;
	setFirePoint(p, xp, yp);
}

int drawPolylineOutline(PolyLine* p) {
	int col = 0;
	int i; Line l;
	for(i=1; i<(*p).PointCount; i++) {
		initLine(&l, (*p).x[i-1], (*p).y[i-1], (*p).x[i], (*p).y[i] ,(*p).r, (*p).g, (*p).b, (*p).a);
		col |= drawLine(&l);
	}

	initLine(&l, (*p).x[i-1], (*p).y[i-1], (*p).x[0], (*p).y[0] ,(*p).r, (*p).g, (*p).b, (*p).a);
	col |= drawLine(&l);
	return col;
}

void fillPolyline(PolyLine* p, int rx, int gx, int bx, int ax) {
	floodFill((*p).xp,(*p).yp, rx,gx,bx,ax, ((*p).r),((*p).g),((*p).b),((*p).a));
}

void drawScreenBorder() {
	PolyLine p;
	initPolyline(&p,0,0,255,0);
	addEndPoint(&p, borderUp,borderLeft);
	addEndPoint(&p, vinfo.xres-borderDown,borderLeft);
	addEndPoint(&p, vinfo.xres-borderDown,vinfo.yres-borderRight);
	addEndPoint(&p, borderUp,vinfo.yres-borderRight);
	drawPolylineOutline(&p);

}

// METODE ANIMASI POLYLINE---------------------------------------------------------------------------------- //
void deletePolyline(PolyLine* p) {

	fillPolyline(p, rground,gground,bground,aground);
	int r = (*p).r;
	int g = (*p).g;
	int b = (*p).b;
	int a = (*p).a;
	(*p).r = rground;
	(*p).g = gground;
	(*p).b = bground;
	(*p).a = aground;
	drawPolylineOutline(p);
	(*p).r = r;
	(*p).g = g;
	(*p).b = b;
	(*p).a = a;
}

// Warning, will remove fill color and only redraw outline
void movePolyline(PolyLine* p, int dx, int dy) {

	deletePolyline(p);
	int tempx;
	int tempy;

	tempx = (*p).xp + dx;
	tempy = (*p).yp + dy;
	(*p).xp = tempx;
	(*p).yp = tempy;

	int i;
	for(i=0; i<(*p).PointCount; i++) {
		tempx = (*p).x[i] + dx;
		tempy = (*p).y[i] + dy;
		(*p).x[i] = tempx;
		(*p).y[i] = tempy;
	}
	drawPolylineOutline(p);
}

void rotatePolyline(PolyLine* p, int xr, int yr, double degrees) {

	deletePolyline(p);
	double cosr = cos((22*degrees)/(180*7));
	double sinr = sin((22*degrees)/(180*7));
	double tempx;
	double tempy;

	tempx = xr + (((*p).xp - xr) * cosr) - (((*p).yp - yr) * sinr);
	tempy = yr + (((*p).xp - xr) * sinr) + (((*p).yp - yr) * cosr);
	(*p).xp = round(tempx);
	(*p).yp = round(tempy);

	int i;
	for(i=0; i<(*p).PointCount; i++) {
		tempx = xr + (((*p).x[i] - xr) * cosr) - (((*p).y[i] - yr) * sinr);
		tempy = yr + (((*p).x[i] - xr) * sinr) + (((*p).y[i] - yr) * cosr);
		(*p).x[i] = round(tempx);
		(*p).y[i] = round(tempy);
	}

	drawPolylineOutline(p);
}

void scalePolyline(PolyLine* p, int xa, int ya, float ratio) {

	deletePolyline(p);
	double tempx;
	double tempy;

	tempx = xa + (((*p).xp - xa) * ratio);
	tempy = ya + (((*p).yp - ya) * ratio);
	(*p).xp = round(tempx);
	(*p).yp = round(tempy);

	int i;
	for(i=0; i<(*p).PointCount; i++) {
		tempx = xa + (((*p).x[i] - xa) * ratio);
		tempy = ya + (((*p).y[i] - ya) * ratio);
		(*p).x[i] = round(tempx);
		(*p).y[i] = round(tempy);
	}

	drawPolylineOutline(p);
}

// PROSEDUR ARRAY OF POLYLINE------------------------------------------------------------------------------- //
typedef struct {

	PolyLine* arr;
	int PolyCount;

} PolyLineArray;

void initPolyLineArray(PolyLineArray* p, int size) {
	(*p).arr = (PolyLine *)malloc(size * sizeof(PolyLine));
	(*p).PolyCount = 0;
}

void addPolyline(PolyLineArray* parr, PolyLine* p) {

	PolyLine* temp = &((*parr).arr[(*parr).PolyCount]);
	(*temp).xp = (*p).xp;
	(*temp).yp = (*p).yp;
	(*temp).PointCount = (*p).PointCount;

	(*temp).r = (*p).r;
	(*temp).g = (*p).g;
	(*temp).b = (*p).b;
	(*temp).a = (*p).a;

	int i;
	for(i=0; i<(*p).PointCount; i++) {
		(*temp).x[i] = (*p).x[i];
		(*temp).y[i] = (*p).y[i];
	}

	(*parr).PolyCount++;

}

void drawPolylineArrayOutline(PolyLineArray* parr) {
	int i;
	for(i=0; i<(*parr).PolyCount;i++) {
		drawPolylineOutline(&((*parr).arr[i]));
	}
}

void movePolylineArray(PolyLineArray* parr, int dx, int dy) {
	int i;
	for(i=0; i<(*parr).PolyCount;i++) {
		movePolyline(&((*parr).arr[i]), dx, dy);
	}
}

void scalePolylineArray(PolyLineArray* parr, int ax, int ay, float scale) {
	int i;
	for(i=0; i<(*parr).PolyCount;i++) {
		scalePolyline(&((*parr).arr[i]), ax, ay, scale);
	}
}

void colorPolylineArray(PolyLineArray* parr, int r, int g, int b, int a) {
	int i;
	for(i=0; i<(*parr).PolyCount;i++) {
		fillPolyline(&((*parr).arr[i]), r,g,b,a);
	}
}

void rotatePolylineArray(PolyLineArray* parr, int xr, int yr, double degrees) {
	int i;
	for(i=0; i<(*parr).PolyCount;i++) {
		rotatePolyline(&((*parr).arr[i]), xr, yr, degrees);
	}
}

	//-----PERBATASAN API <-> KODE TUGAS UTS-----/					/-----PERBATASAN API <-> KODE TUGAS UTS-----//
	//-----PERBATASAN API <-> KODE TUGAS UTS-----/					/-----PERBATASAN API <-> KODE TUGAS UTS-----//
	//-----PERBATASAN API <-> KODE TUGAS UTS-----/					/-----PERBATASAN API <-> KODE TUGAS UTS-----//

// Poly Line Array Stage
PolyLineArray stage;
int rstage=0, gstage=0, bstage=255, astage=0;

// Laser Config
int rlaser=255, glaser=0, blaser=0, alaser=0;
int shooted = 0;
int playerLaserLength = 100;
int monsterLaserLength = 200;

// Poly Line Array Player
PolyLineArray player;
int rplayer=255, gplayer=255, bplayer=255, aplayer=0;
int xshoot, yshoot;	// The point the player will shoot
int orient = 1;		// 1 = atas, 2 = kanan, 3 = bawah, 4 = kiri

// Poly Line Array dari Monster
PolyLineArray monster1, monster2;
int nMonster = 2;
int xShootMonster[2], yShootMonster[2];
int rmonster=255, gmonster=126, bmonster=0, amonster=0;


//----------------------------------PLAYER--------------------------------------------------//

// Draw player
void initPlayer(){
	
	PolyLine body, moncong, turret;
	initPolyLineArray(&player, 11);

	initPolyline(&body, rplayer, gplayer, bplayer, aplayer);
	boxPolyline(&body, xmiddle - 25, ymiddle - 20, xmiddle + 25, ymiddle + 60);

	initPolyline(&moncong, rplayer, gplayer, bplayer, aplayer);
	boxPolyline(&moncong, xmiddle - 5, ymiddle - 20, xmiddle + 5, ymiddle - 60);
	
	initPolyline(&turret, rplayer, gplayer, bplayer, aplayer);
	boxPolyline(&turret, xmiddle - 15, ymiddle - 20, xmiddle + 15, ymiddle + 20);

	xshoot = xmiddle;
	yshoot = ymiddle - 62;

	addPolyline(&player, &body);
	addPolyline(&player, &moncong);
	addPolyline(&player, &turret);

	drawPolylineArrayOutline(&player);
}

int canPlayerMove(int dist) {
	
	int x = xshoot;
	int y = yshoot;	
	int i;
	
	if(dist > 0) {
		
		if((orient==1)||(orient==3)) {
			for(i=1; i<=dist; i++) {
				if(!isPixelColor(x-2,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x-1,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x+1,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x+2,y, rground,gground,bground,aground)) break;
				if(orient==1) y--;
				else y++;
			}
			
		} else {
			for(i=1; i<=dist; i++) {
				if(!isPixelColor(x,y-2, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y-1, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y+1, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y+2, rground,gground,bground,aground)) break;
				if(orient==2) x++;
				else x--;
			}
			
		}
		
	} else {
		
		if(orient==1) y += 124;
		else if(orient==2) x -= 124;
		else if(orient==3) y -= 124;
		else x += 124;
		
		dist = -dist;
		
		if((orient==1)||(orient==3)) {
			for(i=1; i<=dist; i++) {
				if(!isPixelColor(x-2,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x-1,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x+1,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x+2,y, rground,gground,bground,aground)) break;
				if(orient==1) y++;
				else y--;
			}
			
		} else {
			for(i=1; i<=dist; i++) {
				if(!isPixelColor(x,y-2, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y-1, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y+1, rground,gground,bground,aground)) break;
				if(!isPixelColor(x,y+2, rground,gground,bground,aground)) break;
				if(orient==2) x--;
				else x++;
			}
			
		}
	
	}
	
	return i>=dist;	
}

/* Akan menggerakan model player sesuai orientasi dan distance
 * */
void movePlayer(int dist) {
	
	int x=0, y=0;
	if(orient==1) y--;
	else if(orient==2) x++;
	else if(orient==3) y++;
	else x--;
	
	movePolylineArray(&player, x*dist, y*dist);
	moveAll(-x*dist, -y*dist);
}

/* Akan memutar model player dan xyshoot sebesar degree mengelangi xymiddle
 * */
void rotatePlayer(float degree) {
	rotatePolylineArray(&player, xmiddle, ymiddle, degree);
	
	double cosr = cos((22*degree)/(180*7));
	double sinr = sin((22*degree)/(180*7));
	double tempx = xmiddle + ((xshoot - xmiddle) * cosr) - ((yshoot - ymiddle) * sinr);
	double tempy = ymiddle + ((xshoot - xmiddle) * sinr) + ((yshoot - ymiddle) * cosr);
	xshoot = round(tempx);
	yshoot = round(tempy);
}

/* Akan mulai mewarnai titik dari *x, *y sepanjang length ke satu arah sesuai mode yang diberikan
 * 1 = atas, 2 = kanan, 3 = bawah, 4 = kiri
 * Apabila menemui titik yang tidak berwarna background akan berhenti. Nilai x dan y adalah nilai
 * titik terakhir yang akan diwarnai sebelum berhenti. 
 * */
void drawLaser(int* x, int* y, int mode, int length) {
	int i;
	for(int i=1; i<=length; i++) {
		if(isPixelColor(*x,*y, rground,gground,bground,aground)) {
			plotPixelRGBA(*x,*y, rlaser,glaser,blaser,alaser);
		} else break;
		if(mode==1) (*y)--;
		else if(mode==2) (*x)++;
		else if(mode==3) (*y)++;
		else (*x)--;
	}
}

int isHit_Player(int xshoot, int yshoot, int mode, int length){
	int x = xshoot, y = yshoot;
	for(int i=1; i<=length; i++) {
		if(isPixelColor(x,y, rmonster,gmonster,bmonster,amonster)) {
			return 1;
		}
		if(mode==1) (y)--;
		else if(mode==2) (x)++;
		else if(mode==3) (y)++;
		else (x)--;
	}
	return 0;
}


void shootPlayerLaser() {
	int x = xshoot;
	int y = yshoot;
	drawLaser(&x,&y,orient,playerLaserLength);
	shooted = 1;
	
	 if (isHit_Player(xshoot,yshoot,orient,playerLaserLength)){
		 // SHOOTING ACTION HERE
		 printf("HIT\n");
	 }
	
}

void removePlayerLaser() {
	int x = xshoot;
	int y = yshoot;
	
	int i;
	for(int i=1; i<=playerLaserLength; i++) {
		if(isPixelColor(x,y, rlaser,glaser,blaser,alaser)) {
			plotPixelRGBA(x,y, rground,gground,bground,aground);
		} else break;
		if(orient==1) y--;
		else if(orient==2) x++;
		else if(orient==3) y++;
		else x--;
	}
	
	shooted = 0;
}

//----------------------------------Monster--------------------------------------------------//

// Draw monster
void initMonster(int posX, int posY, PolyLineArray* monster, int n_th){
	PolyLine body, head, barrel;
	initPolyLineArray(&(*monster), 11);

	initPolyline(&body, rmonster, gmonster, bmonster, amonster);
	boxPolyline(&body, posX - 40, posY - 20 , posX + 40, posY + 20);

	initPolyline(&head, rmonster, gmonster, bmonster, amonster);
	boxPolyline(&head, posX - 5, posY - 20, posX + 5, posY - 30);
	
	initPolyline(&barrel, rmonster, gmonster, bmonster, amonster);
	boxPolyline(&barrel, posX - 15, posY - 40, posX + 15, posY);

	addPolyline(&(*monster), &body);
	addPolyline(&(*monster), &head);
	addPolyline(&(*monster), &barrel);

	xShootMonster[n_th-1] = posX;
	yShootMonster[n_th-1] = posY-42;
}

int isHit_Monster(int xshoot, int yshoot, int mode, int length){
	int x = xshoot, y = yshoot;
	for(int i=1; i<=length; i++) {
		if(isPixelColor(x,y, rplayer,gplayer,bplayer,aplayer)) {
			return 1;
		}
		if(mode==1) (y)--;
		else if(mode==2) (x)++;
		else if(mode==3) (y)++;
		else (x)--;
	}
	return 0;
}

void shootMonsterLaser(int xmonster, int ymonster, int orient_monster) {
	int x = xmonster;
	int y = ymonster;
	drawLaser(&x,&y,orient_monster,monsterLaserLength);
	
	 if (isHit_Monster(xmonster,ymonster,orient_monster,monsterLaserLength)){
		 // SHOOTING ACTION HERE
		 printf("HIT\n");
	 }	
}
void removeMonsterLaser(int xmonster, int ymonster, int orient_monster) {
	int x = xmonster;
	int y = ymonster;
	
	int i;
	for(int i=1; i<=monsterLaserLength; i++) {
		if(isPixelColor(x,y, rlaser,glaser,blaser,alaser)) {
			plotPixelRGBA(x,y, rground,gground,bground,aground);
		} else break;
		if(orient_monster==1) y--;
		else if(orient_monster==2) x++;
		else if(orient_monster==3) y++;
		else x--;
	}
}


// METODE HANDLER THREAD IO--------------------------------------------------------------------------------- //

void processPlayerInput() {
	while (1) {
		if(shooted == 1) {	
			usleep(100000);
			removePlayerLaser();
		} 
		
		char X = getch();
		if ((X == 'i') || (X == 'I')) { // Zoom in
			scalePolylineArray(&player, xmiddle, ymiddle, 1.1);
			scalePolylineArray(&stage, xmiddle, ymiddle, 1.1);
			scalePolylineArray(&monster1, xmiddle, ymiddle, 1.1);
			scalePolylineArray(&monster2, xmiddle, ymiddle, 1.1);
		} else if ((X == 'o') || (X == 'O')) { // Zoom out
			scalePolylineArray(&player, xmiddle, ymiddle, 1/1.1);
			scalePolylineArray(&stage, xmiddle, ymiddle, 1/1.1);
			scalePolylineArray(&monster1, xmiddle, ymiddle, 1.1);
			scalePolylineArray(&monster2, xmiddle, ymiddle, 1.1);
		} else if ((X == 'x') || (X == 'X')) { // Shoot laser
			shootPlayerLaser();
			
		} else {
			int rotate=0, move=0;
			
			// Right arrow
			if (X == 'a') rotate = -1;
			// Left arrow
			else if (X == 'd') rotate = 1;
			// Up arrow
			else if (X == 's') move = -1;
			// Down arrow
			else if (X == 'w') move = 1;
			
			if(move != 0) {
			
				if(canPlayerMove(move*5)) movePlayer(move*5);
			
			}
			
			if(rotate != 0) {
				rotatePlayer(rotate*90);
				orient += rotate;
				if(orient == 0) orient = 4;
				if(orient == 5) orient = 1;
			}
		}
		drawRotateWindmills();
		fireMonster();
		drawScreenBorder();	
		
	}
}

// void *keylistener(void *null) {
//     while (1) {
// 		processPlayerInput();
//     }
// }

// void *windmillspinner(void *null) {
//     while (1) {
// 		usleep(100000);
		
//     }
// }

void fireMonster(){
	// while(1) {
		for (int i=0; i<nMonster;i++){
			shootMonsterLaser(xShootMonster[i],yShootMonster[i],1);
		}
		usleep(100000);
		for (int i=0; i<nMonster;i++){
			removeMonsterLaser(xShootMonster[i],yShootMonster[i],1);
		}
		// usleep(100000);
	// }
}

void initStage() {	
	int firex, firey = 0;
	int jmlBangunan = 1;
	
	initPolyLineArray(&stage, 80);
	
	PolyLine p;
    FILE* file = fopen("palace.txt", "r"); /* should check the result */
    char line[500];
    
	int pertama = 1;
	initPolyline(&p, rstage, gstage, bstage, astage); //polyline pertama
    
    while (fgets(line, sizeof(line), file)) {
		
        //Jika merupakan baris berisi jumlah point
        if(strlen(line) <= 4 && (pertama != 1)){
			
			setFirePoint(&p, firex, firey);
			addPolyline(&stage, &p);
			initPolyline(&p, rstage, gstage, bstage, astage); //polyline selanjutnya
			
		//Jika merupakan baris berisi x y 
		} else {
			
			if (pertama == 1) pertama = 0;
			else {
				int x;
				int y;
				sscanf(line, "%d %d", &x, &y);
				//printf("%d ", x);
				//printf("%d\n", y);
				addEndPoint(&p, x, y);
			}
		}
	}
	
    //elemen terakhir
    setFirePoint(&p, firex, firey);
	addPolyline(&stage, &p);
    fclose(file);
    
    drawPolylineArrayOutline(&stage);
    scalePolylineArray(&stage, xmiddle, ymiddle, 3);
    movePolylineArray(&stage, 1400,-180);
    
}


PolyLineArray windmills;
int rbase=0, gbase=0, bbase=255, abase=0;
int rbasecolor=0, gbasecolor=0, bbasecolor=126, abasecolor=0;
int rblade=0, gblade=255, bblade=0, ablade=0;
int rbladecolor=0, gbladecolor=126, bbladecolor=0, abladecolor=0;

void addWindmill(int x, int y, float scale) {
	PolyLine base;
	initPolyline(&base, rbase,gbase,bbase,abase);
	addEndPoint(&base, x+(50*scale), y+(50*scale));
	addEndPoint(&base, x+(50*scale), y-(50*scale));
	addEndPoint(&base, x-(50*scale), y-(50*scale));
	addEndPoint(&base, x-(50*scale), y+(50*scale));
    setFirePoint(&base, x, y);
	addPolyline(&windmills,&base);
	
	PolyLine blade;
	initPolyline(&blade, rblade,gblade,bblade,ablade);
	addEndPoint(&blade, x+(25*scale), y+(25*scale));
	addEndPoint(&blade, x+(100*scale), y+(25*scale));
	addEndPoint(&blade, x+(25*scale), y-(25*scale));
	addEndPoint(&blade, x+(25*scale), y-(100*scale));
	addEndPoint(&blade, x-(25*scale), y-(25*scale));
	addEndPoint(&blade, x-(100*scale), y-(25*scale));
	addEndPoint(&blade, x-(25*scale), y+(25*scale));
	addEndPoint(&blade, x-(25*scale), y+(100*scale));
    setFirePoint(&blade, x, y);
	addPolyline(&windmills,&blade);
}

void initWindmill() {
	initPolyLineArray(&windmills, 50);
	addWindmill(550, 250, 1);
	addWindmill(750, 750, 1);
	addWindmill(2200, 600, 1);
	drawPolylineArrayOutline(&windmills);
}

void rotatePolylineWithoutDelete(PolyLine* p, int xr, int yr, double degrees) {

	double cosr = cos((22*degrees)/(180*7));
	double sinr = sin((22*degrees)/(180*7));
	double tempx;
	double tempy;

	tempx = xr + (((*p).xp - xr) * cosr) - (((*p).yp - yr) * sinr);
	tempy = yr + (((*p).xp - xr) * sinr) + (((*p).yp - yr) * cosr);
	(*p).xp = round(tempx);
	(*p).yp = round(tempy);

	int i;
	for(i=0; i<(*p).PointCount; i++) {
		tempx = xr + (((*p).x[i] - xr) * cosr) - (((*p).y[i] - yr) * sinr);
		tempy = yr + (((*p).x[i] - xr) * sinr) + (((*p).y[i] - yr) * cosr);
		(*p).x[i] = round(tempx);
		(*p).y[i] = round(tempy);
	}

	drawPolylineOutline(p);
}

void drawRotateWindmills() {
	int i=0;
	for(i=0; i<windmills.PolyCount; i+=2) {
		deletePolyline(&(windmills.arr[i+1]));
		
		drawPolylineOutline(&(windmills.arr[i]));
		fillPolyline(&(windmills.arr[i]), rbasecolor,gbasecolor,bbasecolor,abasecolor);
		
		rotatePolylineWithoutDelete(&(windmills.arr[i+1]), windmills.arr[i+1].xp, windmills.arr[i+1].yp, -15);
		fillPolyline(&(windmills.arr[i+1]), rbladecolor,gbladecolor,bbladecolor,abladecolor);
	}
}

void movePolylineNoDelete(PolyLine* p, int dx, int dy) {

	int tempx;
	int tempy;

	tempx = (*p).xp + dx;
	tempy = (*p).yp + dy;
	(*p).xp = tempx;
	(*p).yp = tempy;

	int i;
	for(i=0; i<(*p).PointCount; i++) {
		tempx = (*p).x[i] + dx;
		tempy = (*p).y[i] + dy;
		(*p).x[i] = tempx;
		(*p).y[i] = tempy;
	}
	drawPolylineOutline(p);
}

void moveWindmills(int dx, int dy) {
	int i;
	for(i=0; i<windmills.PolyCount; i+=2) {
		deletePolyline(&(windmills.arr[i+1]));
		deletePolyline(&(windmills.arr[i]));
		
		movePolylineNoDelete(&(windmills.arr[i]), dx,dy);
		movePolylineNoDelete(&(windmills.arr[i+1]), dx,dy);
	}
}

void moveShootMonster(int dx, int dy){
	for (int i=0; i<nMonster;i++){
		xShootMonster[i]+=dx;
		yShootMonster[i]+=dy;
	}
}

/* Akan menggerakan SEMUA Model sesuai dx dan dy */
void moveAll(int dx, int dy) {
	movePolylineArray(&player, dx, dy);
	movePolylineArray(&stage, dx, dy);
	movePolylineArray(&monster1, dx, dy);
	movePolylineArray(&monster2, dx, dy);
	moveWindmills(dx, dy);
	moveShootMonster(dx,dy);
}

//MAIN PROGRAM
int main(int argc, char *argv[]) {
    initScreen();
    clearScreen();
    initPlayer();
    initStage();
 	initMonster(xmiddle + 200, ymiddle + 50, &monster1,1);
 	initMonster(xmiddle - 200, ymiddle - 100, &monster2,2);

	drawPolylineArrayOutline(&player);
	drawPolylineArrayOutline(&stage);
    
    initStage();
    initPlayer();
    initWindmill();

    processPlayerInput();
	
 //    pthread_t listener, monsterRoutine, thr_windmill;

	// pthread_create(&listener, NULL, keylistener, NULL);
	// // pthread_create(&thr_windmill, NULL, windmillspinner, NULL);
 //    // pthread_create(&monsterRoutine, NULL, fireMonster, NULL);

	// pthread_join(listener, NULL);
	// // pthread_join(thr_windmill, NULL);
	// // pthread_join(monsterRoutine, NULL);
	
	clearScreen();    
    terminate();
    return 0;
}


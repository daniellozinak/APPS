#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "font8x8.cpp"
#include <string>
#include <math.h>

#define LCD_WIDTH       320
#define LCD_HEIGHT      240
#define LCD_NAME        "Virtual LCD"

#define defaultFont font8x8
#define fontHeight 8
#define fontWidth 8
#define PI 3.14159265

// LCD Simulator

// Virtual LCD
cv::Mat g_canvas( cv::Size( LCD_WIDTH, LCD_HEIGHT ), CV_8UC3 );

unsigned int counter = 0;


// Put color pixel on LCD (canvas)
void lcd_put_pixel( int t_x, int t_y, int t_rgb_565 )
{
    // Transform the color from a LCD form into the OpenCV form. 
    cv::Vec3b l_rgb_888( 
            (  t_rgb_565         & 0x1F ) << 3, 
            (( t_rgb_565 >> 5 )  & 0x3F ) << 2, 
            (( t_rgb_565 >> 11 ) & 0x1F ) << 3
            );
    g_canvas.at<cv::Vec3b>( t_y, t_x ) = l_rgb_888; // put pixel
}

// Clear LCD
void lcd_clear()
{
    cv::Vec3b l_black( 0, 0, 0 );
    g_canvas.setTo( l_black );
}

// LCD Initialization 
void lcd_init()
{
    cv::namedWindow( LCD_NAME, 0 );
    lcd_clear();
    cv::waitKey( 1 );
}

// Simple graphic interface

struct Point2D 
{
    int32_t x, y;

    Point2D(int32_t x, int32_t y) : x(x), y(y){}
    void setPos(int32_t x, int32_t y){ this->x = x; this->y = y;}

};

struct RGB
{
    uint8_t r, g, b;

    RGB(uint8_t r,uint8_t g,uint8_t b) : r(r), g(g), b(b){}

};

class GraphElement
{
public:
    // foreground and background color
    RGB fg_color, bg_color;

    // constructor
    GraphElement( RGB t_fg_color, RGB t_bg_color ) : 
        fg_color( t_fg_color ), bg_color( t_bg_color ) {}

    // ONLY ONE INTERFACE WITH LCD HARDWARE!!!
    void drawPixel( int32_t t_x, int32_t t_y ) { lcd_put_pixel( t_x, t_y, convert_RGB888_to_RGB565( fg_color ) ); }
    
    // Draw graphics element
    virtual void draw() = 0;
    
    // Hide graphics element
    virtual void hide() { swap_fg_bg_color(); draw(); swap_fg_bg_color(); }
    void setColor(RGB c){ fg_color = c;}
private:
    // swap foreground and backgroud colors
    void swap_fg_bg_color() { RGB l_tmp = fg_color; fg_color = bg_color; bg_color = l_tmp; } 
	
    // conversion of 24-bit RGB color into 16-bit color format
    int convert_RGB888_to_RGB565( RGB t_color ) { 
	int r = ((t_color.r >> 3) & 0x1F) << 11;
	int g = ((t_color.g >> 2) & 0x3F) << 5;
	int b = ((t_color.b >> 3) & 0x1F);
	return (r | g | b);
    }
};


class Pixel : public GraphElement
{
public:
    // constructor
    Pixel( Point2D t_pos, RGB t_fg_color, RGB t_bg_color ) : pos( t_pos ), GraphElement( t_fg_color, t_bg_color ) {}
    // Draw method implementation
    virtual void draw() { drawPixel( pos.x, pos.y ); }
    void delCurrent(){lcd_put_pixel(pos.x, pos.y, 0x0);}
    void setPos(int x, int y){pos.x = x; pos.y = y;}
    // Position of Pixel
    Point2D pos;
};


class Circle : public GraphElement
{
public:
    // Center of circle
    Point2D center;
    // Radius of circle
    int32_t radius;	

    Circle( Point2D t_center, int32_t t_radius, RGB t_fg, RGB t_bg ) : 
        center( t_center ), radius( t_radius ), GraphElement( t_fg, t_bg ){};

    void draw() {
	circleBres(center.x, center.y, radius);
    }
private:
    void drawCircle(int xc, int yc, int x, int y) 
    { 
    drawPixel(xc+x, yc+y);
    
    drawPixel(xc-x, yc+y); 
    
    drawPixel(xc+x, yc-y); 
    
    drawPixel(xc-x, yc-y); 
    
    drawPixel(xc+y, yc+x); 
   
    drawPixel(xc-y, yc+x); 
  
    drawPixel(xc+y, yc-x); 
    
    drawPixel(xc-y, yc-x); 
 
    }  
  

   void circleBres(int xc, int yc, int r) 
   { 
    int x = 0, y = r; 
    int d = 3 - 2 * r; 
    drawCircle(xc, yc, x, y); 
    while (y >= x) 
    { 
          
        x++; 

        if (d > 0) 
        { 
            y--;  
            d = d + 4 * (x - y) + 10; 
        } 
        else
            d = d + 4 * x + 6; 
        drawCircle(xc, yc, x, y);
    } 
  } 
};

class Character : public GraphElement 
{
public:
    // position of character
    Point2D pos;
    // character
    char character;

    Character( Point2D t_pos, char t_char, RGB t_fg, RGB t_bg ) : 
      pos( t_pos ), character( t_char ), GraphElement( t_fg, t_bg ) {};

    void draw() { 
	    for (int i=0;i<fontHeight;i++)
            {
                for(int j = 0;j<fontWidth;j++)
                {
                   if(false!=((defaultFont[character][i])>>(j)&1))
                      { drawPixel(j+pos.x,i+pos.y);}
                }
	     }
	

};
};

class String : public GraphElement{
public:
    Point2D startPos;
    std::string string;
    String(Point2D t_pos, std::string t_s, RGB t_fg, RGB t_bg) : startPos(t_pos), string(t_s), 	      GraphElement(t_fg,t_bg){};

    void draw(){
	for(int c = 0; c < string.length(); c++){
	    for (int i=0;i<fontHeight;i++)
            {
                for(int j = 0;j<fontWidth;j++)
                {
                   if(false!=((defaultFont[string[c]][i])>>(j)&1))
                      { drawPixel(j+startPos.x +c*fontWidth,i+startPos.y);}
                }
	     }
	}
    }
private:
};

class Line : public GraphElement
{
public:
    // the first and the last point of line
    Point2D pos1, pos2;

    Line( Point2D t_pos1, Point2D t_pos2, RGB t_fg, RGB t_bg ) : 
      pos1( t_pos1 ), pos2( t_pos2 ), GraphElement( t_fg, t_bg ) {
	/*if(pos1.x > pos2.x || pos1.y > pos2.y){ 
		Point2D temp = pos1;
		pos1 = pos2;
		pos2 = temp;
	    }*/
	}

    void draw() { 
	drawLine(pos1.x,pos1.y, pos2.x, pos2.y);
    };
private:
   void drawLine(int x1, int y1, int x2, int y2) {
    int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i;
    dx=x2-x1;
    dy=y2-y1;
    dx1=fabs(dx);
    dy1=fabs(dy);
    px=2*dy1-dx1;
    py=2*dx1-dy1;
    if(dy1<=dx1) {
        if(dx>=0) {
            x=x1;
            y=y1;
            xe=x2;
        } else {
            x=x2;
            y=y2;
            xe=x1;
        }
        drawPixel(x, y);
        for(i=0;x<xe;i++) {
            x=x+1;
            if(px<0) {
                px=px+2*dy1;
            } else {
                if((dx<0 && dy<0) || (dx>0 && dy>0)) {
                    y=y+1;
                } else {
                    y=y-1;
                }
                px=px+2*(dy1-dx1);
            }
            drawPixel(x, y);
        }
     } else {
         if(dy>=0) {
             x=x1;
             y=y1;
             ye=y2;
         } else {
             x=x2;
             y=y2;
             ye=y1;
         }
         drawPixel(x, y);
         for(i=0;y<ye;i++) {
             y=y+1;
             if(py<=0) {
                 py=py+2*dx1;
             } else {
                 if((dx<0 && dy<0) || (dx>0 && dy>0)) {
                     x=x+1;
                 } else {
                     x=x-1;
                 }
                 py=py+2*(dx1-dy1);
             }
             drawPixel(x, y);
         }
     }
}
};

class Watch : public Circle{

public:
    Watch(Point2D t_pos,int t_radius,RGB t_handcolor, RGB t_numbercolor,RGB t_fg, RGB t_bg) : Circle(t_pos,t_radius,t_fg,t_bg), hand(t_pos,t_pos,t_handcolor,t_bg),start(t_pos),hand2(t_pos,t_pos,t_handcolor,t_bg), radius(t_radius),numberC(t_numbercolor),black(t_bg){}

    void init(){
	hand.pos2.y = start.y - radius;
	hand2.pos2.y = start.y - (radius/2);
	RGB blue(0,0,255);
	hand2.setColor(blue);
	hand2.draw();
	hand.draw();
	this->draw();
	animate();
   }
	
  
private:
   Line hand;
   Line hand2;
   Point2D start;
   int radius;
   RGB numberC;
   RGB black;
   std::string sec1 = "12";
   std::string sec2 = "03";
   std::string sec3 = "06";
   std::string sec4 = "09";

   void animate(){
	float offset = PI / 2;
	float step = (2*PI) / 60;
	Point2D pointsec1(start.x - 8,start.y - radius - 8);
	Point2D pointsec2(start.x + radius + 3 ,start.y - 4);
	Point2D pointsec3(start.x - 6,start.y + radius + 3);
	Point2D pointsec4(start.x - radius - 16,start.y - 3);
	String s1(pointsec1,sec1,numberC,black);
	String s2(pointsec2,sec2,numberC,black);
	String s3(pointsec3,sec3,numberC,black);
	String s4(pointsec4,sec4,numberC,black);

	String arrS[4] = {s1,s2,s3,s4};
	Point2D arrP[4] = {pointsec1,pointsec2,pointsec3,pointsec4};
	
	    
	   for(float j = -1 * offset; j < 2*PI - offset; j+=step){
		Point2D temp(start.x + (radius/2)*cos(j), start.y + (radius/2)*sin(j));
		hand2.pos2.x = temp.x; hand2.pos2.y = temp.y;
		
		for(float i = -1 * offset; i < 2*PI - offset; i+=step){
		    hand2.draw();
		    hand.hide();
		    Point2D tmp(start.x + radius*cos(i), start.y + radius*sin(i));
		    hand.pos2.x = tmp.x; hand.pos2.y = tmp.y;
		    hand.draw();
		    for(int i = 0; i < 4; i++){ 
			arrS[i].draw();}
			
		    this->draw();
		    cv::waitKey(16);
		    cv::imshow(LCD_NAME, g_canvas);
		    }
		    lcd_clear();
		}
	}
		
	
};



int main()
{
    lcd_init();                     // LCD initialization

    lcd_clear();                    // LCD clear screen

    int l_color_red = 0xF800;
    int l_color_green = 0x07E0;
    int l_color_blue = 0x001F;
    int l_color_white = 0xFFFF;
    
	
    RGB watchC(12,125,56);
    RGB handC(255,255,0);
    RGB grey(127,127,127);

    RGB black(0,0,0);
    int x = LCD_WIDTH / 2;
    int y = LCD_HEIGHT / 2;
    Point2D start(x,y);
    Watch watch(start,50,handC,grey,watchC,black);
    watch.init();
    

    cv::imshow( LCD_NAME, g_canvas );   // refresh content of "LCD"
    cv::waitKey( 0 );                   // wait for key 
}



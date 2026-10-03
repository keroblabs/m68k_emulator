#include "draw.h"
#include "vga_driver.h"

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

extern const font_data_t font_8x8;

/////////////////////////////////////////////////////////////////////////////////////////

void draw_gr_character(uint8_t character, 
						uint16_t posx, uint16_t posy, 
						uint16_t fore_colour, uint16_t back_colour, 
						bool_t transparent)
{
    uint32_t index;
    
	index = (character - font_8x8.offset) * (font_8x8.size);
	draw_bitmap_1bb(posx, posy, font_8x8.width, font_8x8.height, 
                    (uint8_t *)&font_8x8.data[index], 
                     back_colour, fore_colour);
}

void draw_bitmap_1bb(uint16_t posx, uint16_t posy, uint16_t width, uint16_t heigth, 
					      uint8_t *input, uint8_t back_colour, uint8_t fore_colour)
{
	uint16_t x;
	uint16_t y;
	uint16_t bit_mask;
	uint8_t * ptr_pixel;

	for (y = 0; y < heigth; y++)
	{
		if (y > vga_get_height()) break;

		ptr_pixel = &input[((y >> 3) * width)];
		bit_mask = 0x01 << (y & 0x07);
		
		for (x = 0; x < width; x++)
		{
			if (x > vga_get_width()) break;

			if ((ptr_pixel[x] & bit_mask) != 0)
			{
				vga_set_pixel(posx + x, posy + y, fore_colour);
			}
			else
			{
                vga_set_pixel(posx + x, posy + y, back_colour);
			}		
		}
	}
}

void draw_text(uint16_t posx, uint16_t posy, char * text, uint16_t fore_colour, uint16_t back_colour)
{
	uint32_t i;
	uint16_t x, y;

	x = posx;
	y = posy;

	for (i = 0; i < strlen(text); i++)
	{
        if ((x + font_8x8.width) >= vga_get_width())
        {
            break;
        }

		if (text[i] == 0x0a)
		{
			y += font_8x8.height;
		}
		else if (text[i] == 0x0D)
		{
			x = 0;
		}
		else
		{
			draw_gr_character((uint8_t)text[i], x, y, fore_colour, back_colour, FALSE);	
			x += font_8x8.width;
		}
	}
}

void draw_clear_screen(void)
{
    vga_fill_colour(0, 0, vga_get_width(), vga_get_height(), 0);
}

void draw_fill_colour(uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint8_t colour)
{
    vga_fill_colour(posx, posy, width, height, colour);
}

void draw_do_scroll(uint16_t height)
{
    vga_do_scroll(height);
}

uint32_t draw_get_text_height(char * text)
{
	uint32_t i;
	uint32_t len;

	len = 0;

	for (i = 0; i < strlen(text); i++)
	{
		if (len == 0) len = font_8x8.height;

		if (text[i] == 0x0a)
		{
			len += font_8x8.height;
		}
	}

	return len;
}

uint32_t draw_get_text_width(char * text)
{
	uint32_t i;
	uint32_t len;
	uint32_t max_len;

	len = 0;
	max_len = 0;

	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == 0x0D)
		{
			if (max_len < len) max_len = len;
			len = 0;
		}
		else
		{
			len += font_8x8.width;
		}
	}

	if (max_len < len) max_len = len;
	return max_len;
}

void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t colour)
{
    // draw function
    int x,y,end,inc=0,p;
    int dx=(int)x2-(int)x1;
    int dy=(int)y2-(int)y1;
    int c=0,current=0;

    // last_x = x2;
    // last_y = y2;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

     if(dx>dy)
     {
         p=2*dy-dx;
          if(x1<x2)
          {
               x=x1;y=y1;end=x2;
               if(y1<y2)inc=1;
               if(y1>y2)inc=-1;
          }
          else
          {
               x=x2;y=y2;end=x1;
               if(y2<y1)inc=1;
              if(y2>y1)inc=-1;
          }
          while(x<=end)
          {
              //lcd_set_pixel(x,y,colour);
			  vga_set_pixel(x, y, colour);

              if(p<0) p=p+2*dy;
              else
              {
                  y=y+inc;p=p+2*(dy-dx);
               }
              x++;
              if(current==0&&c==10)
              {
                  current=1;c=-1;
              }
             if(current==1&&c==6)
              {
                  current=0;c=-1;
             }
             c++;
          }
     }
     else
    {
        p=2*dx-dy;
        if(y1<y2)
        {
            y=y1;x=x1;end=y2;
            if(x1<x2)inc=1;
            if(x1>x2)inc=-1;
        }
        else
        {
            y=y2;x=x2;end=y1;
            if(x2<x1)inc=1;
            if(x2>x1)inc=-1;
        }
        while(y<=end)
          {
             //lcd_set_pixel(x,y,colour);
			  vga_set_pixel(x, y, colour);

             if(p<0)p=p+2*dx;
             else
             {
                 x=x+inc;p=p+2*(dx-dy);
               }
               y++;
               if(current==0&&c==10)
               {
                current=1;c=-1;
               }
               if(current==1&&c==6)
               {
                   current=0;c=-1;
               }
               c++;
          }
     }
}

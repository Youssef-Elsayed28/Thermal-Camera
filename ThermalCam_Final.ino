#include <SPI.h>
#include <Adafruit_MLX90640.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>


//TFT Screen Pins 
int TFT_CS = 4;
int TFT_RES = 5;
int TFT_DC = 6; 

//SPI pins
int TFT_SCK = 15;
int TFT_MOSI = 7;
int TFT_MISO = 17;

//I2C Pins
int I2C_SDA = 8;
int I2C_SCL = 9;

//The array that holds the temps
float frame[32*24];

//The Max and Min Temp Variables
float T_max;
float T_min;
//RGB565 Variables 
int R,G,B;
float t;

//Creating the objects for the tft screen and the mlx cam
Adafruit_MLX90640 mlx;
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RES);


void setup() 
{

  Serial.begin(115200);

  // Specifying which pins the SPI communication will use  
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
  //Specifying which pins the I2C communication will use
  Wire.begin(I2C_SDA,I2C_SCL);
  Wire.setClock(400000);   // 400 kHz

  // Starting the TFT screen and setting the clock speed for the SPI
  tft.begin(40000000);

  //Specifying the I2C Address for the cam (0x33)
  mlx.begin(0x33, &Wire);

  //Settings for the cam
  mlx.setMode(MLX90640_CHESS); //Setting the subimage mode 
  mlx.setRefreshRate(MLX90640_2_HZ); //Setting how often it gets a frame array
  mlx.setResolution(MLX90640_ADC_18BIT); // Setting the accuracy 

  // Setting the orientation of the screen
  tft.setRotation(3);

}

void loop() 
{
  int j = 0;
  float T; // Calculated/measured temp at each pixel

  T_max = 1e-9; // Initializing the maximum temp
  T_min = 1e9; // Initializing the minimum temp

  // This if statement fills the frame array with temperatures and prints a message if it fails.
  if (mlx.getFrame(frame) != 0)
  {
    Serial.print("Getting Temps Failed");
    delay(50);
    return;
  }


  //This for loop is used to determine the maximum and minimum temps
  for (int i = 0; i<768; i++)
  {
   
    // Pixel 111 is broken, so this averages the surrounding pixels and assigns it the average value.
    if (i==111)
    {
      frame [i] = (frame[i+1]+frame[i-1]+frame[i+32]+frame[i-32])/4;
    }

    t = frame[i];
  
    //Finding Max and Min Temp in the Frame
    if (T_max<t) {T_max = t;}
    if (T_min>t) {T_min = t;}
  }

  float T1,T2,T3,T4,Tx1,Tx2,fx,fy;

  frame[111] = (frame[112]+frame[110]+frame[79]+frame[143])/4; // ensures that the broken pixel is an avg of the pixels around it

  tft.startWrite();

  for (int y=0; y<240; y++) // y represents the rows of the camera resolution 
  {
    fy = (y % 10)/10.0f; // Using the remainder fy is calculated --> fy = 0.1-0.9 fy is used to calculate the interpolated temps 

    for (int x=0; x<320; x++) 
    {
      
      fx = (x % 10)/10.0f; // Using the remainder fx is calculated --> fx = 0.1-0.9. fx is used to calculate the interpolated temps 
      int Y = y/10.0; // This calculation will always return an integer; Y = 0,1,2,3. Y represents the columns. 
      int X = x/10.0; // This calculation will always return an integer; X = 0,1,2,3. X represents the rows.

      t = frame[Y*32 + X]; 

      if (X < 31 && Y <23) // If not currently at the last row and column then run this code - we will interpolate both along the x and y 
      {
        T1 = frame[Y*32+X]; // Non interpolated temp
        T2 = frame[Y*32+X+1]; // Non interpolated temp
        T3 = frame[(Y+1)*32+X]; // Non interpolated temp
        T4 = frame[(Y+1)*32+X+1]; // Non interpolated temp
        Tx1 = T2*fx + T1*(1-fx); // Interpolated temp along the initial row
        Tx2 = T4*fx + T3*(1-fx); // Interpolated temp along the final row

        T = Tx2*fy + Tx1*(1-fy); // Interpolated temp along both x and y.

        // Color Calcs
        if (T<(T_max+T_min)/2)
        {
          R = 0;
          G = 510*(T/(T_max-T_min))-510*(T_min/(T_max-T_min));
          B = 255;
        }
           
        else
        {
          R = 255;
          G = -510*(T/(T_max-T_min))+510*(T_max/(T_max-T_min));
          B = 0;
        }
          
        tft.writePixel(319-x, y, tft.color565(R,G,B)); // Draw the color at the specified location and the calculated colors
      }

      else if (X == 31 && Y != 23) // If at the last column but not the last row, interpolate only along y.
      {
          
        Tx1 = frame[Y*32+X]; 
        Tx2 = frame[(Y+1)*32+X];
        T = Tx2*fy + Tx1*(1-fy);  

        // Color Calcs
        if (T<(T_max+T_min)/2)
        {
          R = 0;
          G = 510*(T/(T_max-T_min))-510*(T_min/(T_max-T_min));
          B = 255;
        }
           
        else
        {
          R = 255;
          G = -510*(T/(T_max-T_min))+510*(T_max/(T_max-T_min));
          B = 0;
        }
        tft.writePixel(319-x, y, tft.color565(R,G,B)); // Draw the color at the specified location using the calculated color values
      }

      else if (Y == 23 && X != 31 ) // if at the last row but not the last column - will only interpolate along the x
      {
        T1 = frame[Y*32+X];
        T2 = frame[Y*32+X+1];
        T = T2*fx + T1*(1-fx); 

        // Color Calcs
        if (T<(T_max+T_min)/2)
        {
          R = 0;
          G = 510*(T/(T_max-T_min))-510*(T_min/(T_max-T_min));
          B = 255;
        }
           
        else
        {
          R = 255;
          G = -510*(T/(T_max-T_min))+510*(T_max/(T_max-T_min));
          B = 0;
        }
        tft.writePixel(319-x, y, tft.color565(R,G,B)); // if at the last row but not the last column - will only interpolate along the x
      }

      else // if at the last column and the last row - dont interpolate along either the x or y.
      {
        T = frame[Y*32 + X];

        // Color Calcs
        if (T<(T_max+T_min)/2)
        {
          R = 0;
          G = 510*(T/(T_max-T_min))-510*(T_min/(T_max-T_min));
          B = 255;
        }
           
        else
        {
          R = 255;
          G = -510*(T/(T_max-T_min))+510*(T_max/(T_max-T_min));
          B = 0;
        }
        tft.writePixel(319-x, y, tft.color565(R,G,B)); // if at the last row but not the last column - will only interpolate along the x
      }
    }
  }
  
  tft.endWrite();
}

  
  


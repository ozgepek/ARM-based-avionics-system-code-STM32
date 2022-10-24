//KÜTÜPHANELER.
#include <EnvironmentCalculations.h>
#include <BME280I2C.h>
//#include <SD.h> //Load SD card library
//#include<SPI.h> //Load SPI Library
#include<Wire.h>
#include<Math.h>
#include <TinyGPS.h>

TinyGPS gps;
#include "SoftSerialSTM32.h"
SoftSerialSTM32 ss(PB11, PB10); //rx tx
#define ss Serial3

#define SERIAL_BAUD 9600
// Assumed environmental values:
float referencePressure = 1016.3;  // yerel basnç(ben burada istanbul küçükçekmeceye göre ayarladım)
float outdoorTemp = 23;           // dışarının sıcaklığı (atış yapılacak bölgeye göre girilmelidir)
float barometerAltitude = 1650.3;  // meters ... map readings + barometer position

BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76
);

BME280I2C bme(settings);


const int MPU_addr = 0x68; //MPU adresi

double ang_x, ang_y , ang_z , derece_x, derece_y, deger_y, deger_x;// Açı değerleri İçin tanımlanan değişkenler.
int birinci_ayrilma = 0, ikinci_ayrilma = 0;

unsigned long yeni_zaman, eski_zaman = 0; //millis zaman fonksiyonu kullanımı için değişkenler.
int m = 0;
double altitude_deger, altitude , eski_altitude, onceki_altitude; //Bmp 280 basınç sensöründen irtifa değeri okumak için tanımlanan değişkenler.

double altitude_d[4] = {}; //farklı irtifa değerleri için dizi tanımlama
// Açı değerleri İçin tanımlanan diziler
float x_dizi[7] = {};
float y_dizi[7] = {};

//int chipSelect = 10; //SD kartın CS pini için tanımlanan D0 pini
//File mySensorData; //SD kart İçin oluşturulan nesne.


int durum = 0;

//*************************************************************************************************************************************************************************************

void setup() {
 // pinMode(5, OUTPUT);
 //pinMode(6, OUTPUT);
 //pinMode(9, OUTPUT);

 // SPI.begin();
  Serial.begin(SERIAL_BAUD);

  Wire.begin();

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  Serial.begin(9600);//Seri haberlşeme başlatılır
  ss.begin(9600); // GPS İçin SPI başlatma
  Serial.begin(9600);
  Wire.begin();   // İvme sensörü İçin I2C Başlatılır
 // SD.begin();  // SD kart başlatma

  // Mpu 6050 İvme sensörü bağlangıç ayarları.
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);


  eski_altitude = altitude; // Başlangıç irtifasını sıfır yapmak İçin Başlangıç İrtifası kaydetme.
  delay(10);

  int i = 0; // İrtifa dizisine değerlerin atanması
  while (i < 4) {
    altitude_d[i] = altitude;
    i++;
    delay(1);
  }


  int k = 0;  // Açı Dizilerine değerlerin atanması
  while (k < 7) {
    x_dizi[k] = x_aci();
    y_dizi[k] = y_aci();
    k++;
    delay(1);

  }
}
//*****************************************************************************************************************************************************************************
void loop() {

bas:

  // GPS KONUM FALAN xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//  durum = 0;
  smartdelay(500);// GPS değerlerinin çekilmesi için gerekli

  // GPS 'den enlem boylam değerlerinin çekilmesi
  float enlem , boylam;
  unsigned long age;
  gps.f_get_position(&enlem, &boylam, &age);
 Serial.print("Enlem: "); Serial.println(enlem, 8);
 Serial.print("Boylam: "); Serial.println(boylam, 8);

 int hiz = gps.f_speed_kmph();// GPS Sensöründen hız değerinin akınması
 Serial.print("Hız: "); Serial.println(hiz);

  // GPS 'den zaman değerlerinin Çekilmesi
 int year;
 byte month, day, hour, minute, second, hundredths;
 unsigned long age2;
 gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age2);
 Serial.print("Saat: "); Serial.println(hour + 3);//Utc cinsinden olan saati yerel saate çevirmek için (+3)yaptık
 Serial.print("Dakika: "); Serial.println(minute);
 Serial.print("Saniye: "); Serial.println(second);


  // İRTİFA xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  // İrtifa değerlerinin ortalamasının alınması
  altitude = (altitude_d[0] + altitude_d[1] + altitude_d[2] + altitude_d[3]) / 4;
  altitude = altitude - eski_altitude;// Bulununan irtifayı fıfır alma işlemi

  altitude_deger = altitude;// Kaydırma İşlemi İçin yeni irtifa değerinin çekilmesi




  //AÇI xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  // Açı değerlerinin ortalamasının alınması
  derece_x = (x_dizi[0] + x_dizi[1] + x_dizi[2] + x_dizi[3] + x_dizi[4] + x_dizi[5] + x_dizi[6]) / 7;

  derece_y = (y_dizi[0] + y_dizi[1] + y_dizi[2] + y_dizi[3] + y_dizi[4] + y_dizi[5] + y_dizi[6]) / 7;

  //Açı ve ivme değerinin Ekrana Yazdırılması






  Serial.print("X:"); //X ve Y eksenleri için açı değerleri seri porttan bastım
  Serial.print(derece_x);
  Serial.print("\t");
  Serial.print("Y:");
  Serial.print(derece_y);
  Serial.print("\t");
  Serial.print("X_İvme:");
  Serial.println(x_ivme());


  // Dizi kaydırma İşlemi için yeni Açı değerlerinin çekilmesi
  deger_x = x_aci();
  deger_y = y_aci();

  //Açı dizilerinin kaydırma işlemi

  x_dizi[0] = x_dizi[1];
  x_dizi[1] = x_dizi[2];
  x_dizi[2] = x_dizi[3];
  x_dizi[3] = x_dizi[4];
  x_dizi[4] = x_dizi[5];
  x_dizi[5] = x_dizi[6];
  x_dizi[6] = deger_x;

  y_dizi[0] = y_dizi[1];
  y_dizi[1] = y_dizi[2];
  y_dizi[2] = y_dizi[3];
  y_dizi[3] = y_dizi[4];
  y_dizi[4] = y_dizi[5];
  y_dizi[5] = y_dizi[6];
  y_dizi[6] = deger_y;

  // İrtifa dizisinin kaydırma İşlemi
  altitude_d[0] = altitude_d[1];
  altitude_d[1] = altitude_d[2];
  altitude_d[2] = altitude_d[3];
  altitude_d[3] = altitude_deger;


//  Serial.print("hız");
//  Serial.println(hiz);
   printBME280Data(&Serial);
   delay(500);
}

void printBME280Data
(
   Stream* client
)
{
   float temp(NAN), hum(NAN), pres(NAN);

   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_hPa);

   bme.read(pres, temp, hum, tempUnit, presUnit);

   client->print("Temp: ");
   client->print(temp);
   client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? "C" :"F"));
   client->print("\t\tPressure: ");
   client->print(pres);
   client->print(String(presUnit == BME280::PresUnit_hPa ? "hPa" : "Pa")); // expected hPa and Pa only
  

   EnvironmentCalculations::AltitudeUnit envAltUnit  =  EnvironmentCalculations::AltitudeUnit_Meters;
   EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;
   float altitude = EnvironmentCalculations::Altitude(pres, envAltUnit, referencePressure, outdoorTemp, envTempUnit);
   client->print("\t\tAltitude: ");
   client->println(altitude);

   delay(1000);
}








//*************************************************************************************************************************************************************************

// FONKSİYONLAR  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

//X FONKSİYONU**************************************************************

float x_aci() {

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);

  int16_t XAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t YAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t ZAxisFull =  (Wire.read() << 8 | Wire.read());


  ang_x = (atan(YAxisFull / (sqrt(pow(XAxisFull, 2) + pow(ZAxisFull, 2)))) * 57296 / 1000) + 1.35; //Euler Açı formülüne göre açı hesabı+sensör hatasu (X-Ekseni)

  return ang_x;
}
//****************************************************************************************************************************
//Y FONKSİYONU

float y_aci() {
  Wire.beginTransmission(MPU_addr);                                         //MPU6050 ile I2C haberleşme başlatılır
  Wire.write(0x3B);                                                         //İvme bilgisinin olduğu 0x3B-0x40 için request gönderilir
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);

  int16_t XAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t YAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t ZAxisFull =  (Wire.read() << 8 | Wire.read());

  ang_y = (atan(-1 * XAxisFull / (sqrt(pow(YAxisFull, 2) + pow(ZAxisFull, 2)))) * 57296 / 1000) + 1.64; //Euler Açı formülüne göre açı hesabı+sensör hatası (Y-Ekseni)
  return ang_y;
}


// İVME *******************************************************************************************************************************************************
float x_ivme() {
  Wire.beginTransmission(MPU_addr);                                         //MPU6050 ile I2C haberleşme başlatılır
  Wire.write(0x3B);                                                         //İvme bilgisinin olduğu 0x3B-0x40 için request gönderilir
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);

  int16_t XAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t YAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t ZAxisFull =  (Wire.read() << 8 | Wire.read());
  float XAxisFinal = (float) XAxisFull / 16384.0;                  //Datasheet'te yazan değerlere göre "g" cinsinden ivme buldum. (X ekseni için)
  float YAxisFinal = (float) YAxisFull / 16384.0;
  float ZAxisFinal = (float) ZAxisFull / 16384.0;

  if (XAxisFinal > 0.99) XAxisFinal = 1; //0.99 olan değerler 1'e tamamladım
  if (YAxisFinal > 0.99) YAxisFinal = 1;
  if (ZAxisFinal > 0.99) ZAxisFinal = 1;

  if (XAxisFinal < -0.99) XAxisFinal = -1; //-0.99 olan değerler 1'e tamamladım.
  if (YAxisFinal < -0.99) YAxisFinal = -1;
  if (ZAxisFinal < -0.99) ZAxisFinal = -1;

  return ZAxisFinal;



}


 static void smartdelay(unsigned long ms) {
 unsigned long start = millis();
  do {
    while (ss.available())
     gps.encode(ss.read());
  } while (millis() - start < ms);
}

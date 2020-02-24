/*
    Proje Adı       : TermostatDS18B20
    Proje Sahibi    : Selçuk TERZİOĞLU
    Proje Tarihi    : 19.02.2020
    Açıklama        : Nu çalışma eğitim amaçlı olarak öğrencilere yaptırmak amacıyla tasarlanmıştır. İnsan hayatını etkilyecek yerlerde kullanmak yasaktır.
                      İnsan hayatını etkilyecek yerlerde kullanılması durumunda sorumluluk kullanan kişiye aittir. 
                      Devre Arduino + DS18B20 sensörü kullanılarak tasarlanmıştır. Butonlar yardımıyla set değeri ve tolerans ayarlanarak istenilen sıcaklık
                      değerinde ve istenilen tolerans aralığında kontrol sağlanabilir. DS18B20 ile 0.1 derece hassasiyetle ölçüm yapılabilr. Devre şeması ve
                      pcb dosylarına https://github/selcukterzioglu adresinden ulaşailirsiniz.
    Devre Çalışması : Enerji verildiğinde sıcaklık kontrolü otomatik olarak başlar. İstediğiniz dereceyei ayaralamak için iki butona beraber basarsanız ekranda
                      C simgesi ve yanında rakamlar çıkacaktır. Bu dereceSet değeridir. Yukarı/Aşağı butonları ile istediğiniz dereceye ayarlayıp tekrar iki
                      butona aynı anda bastığınızda ekranın sağında t simgesi ve yanında rakamlar görülecek. Bu tolerans değeridir. istediğiniz değere ayarlayıp
                      tekrar iki butona beraber basarsanız değerler kaydolacaktır ve derece göstermeye başlayacaktır. Eğer Derece ayarladığınız dereceSet + tolerans
                      değerini geçerse röle çalışacaktır. Eğer derece dereceSet - tolerans değerinin altına inerse röle bırakacaktır.
*/
//kütüphane dosyları mevcut değilse sağındaki linklerden indirip Arduino/libraries klasörüne koplyalayınız.
#include "EepromOkuYaz.h"           //Proje dizininde mevcut dosya
#include <OneWire.h>                //OneWire seri haberleşme kütüphanesi           https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>      //DS18B20'yi okumak için gerekli kütüphane      https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <TimerOne.h>               //Timer1'i kullanmak için gerekli kütüphane     https://code.google.com/archive/p/arduino-timerone/downloads
#include <Gosterge.h>               //Göstergeyi kullanmak için gerekli kütüphane   https://github.com/selcukterzioglu/Gosterge

#define ADRES_SISTEM            0
#define ADRES_SET               1
#define ADRES_TOLERANS          3

#define BUTON_BASILMADI         -1
#define BUTON_CIFT_BASILDI      0
#define BUTON_ARTI_BASILDI      1
#define BUTON_EKSI_BASILDI      2

#define GOSTERGE_MODU_DERECE    0
#define GOSTERGE_MODU_SET       1
#define GOSTERGE_MODU_TOLERANS  2
#define GOSTERGE_MODU_FLASH     3

//ölçüm zamanının değiştirmek için bu rakamı değiştirin. Her basamak 5msn dir.(Varsayılan 400 x 5msn = 2000 msn = 2sn)
#define DERECE_OLCUM_ZAMANI     400

#define BUTON_ARTI              11  //Artı butonunun bağlantı pini
#define BUTON_EKSI              10  //Eksi butonunun bağlantı pini
#define DS18B20_PIN             12  //DS18B20 sensörünün bağlantı pini
#define ROLE_PIN                17  //Rölenin bağlı olduğu pin sayısı

//Gösterge tipine göre aşağıdaki Gösterge tiplerinden birini seçiniz.
#define GOSTERGE_TIPI       ORTAK_ANOT
//#define GOSTERGE_TIPI ORTAK_KATOT

#define GOSTERGE_SAYISI 4 //Gösterge sayısı

#define SEG_A                   4   //Göstergenin A Segmenti
#define SEG_B                   2   //Göstergenin B Segmenti
#define SEG_C                   8   //Göstergenin C Segmenti
#define SEG_D                   6   //Göstergenin D Segmenti
#define SEG_E                   5   //Göstergenin E Segmenti
#define SEG_F                   3   //Göstergenin F Segmenti
#define SEG_G                   9   //Göstergenin G Segmenti
#define SEG_DP                  7   //Göstergenin NOKTA Segmenti


#define CA1                     13  //1.Göstergenin Anodu/Katodu
#define CA2                     15  //2.Göstergenin Anodu/Katodu
#define CA3                     16  //3.Göstergenin Anodu/Katodu
#define CA4                     14  //4.Göstergenin Anodu/Katodu

//Gösterge segmenlerinde bir dizi oluşturuluyor
int segment[] = {SEG_DP, SEG_G, SEG_F, SEG_E, SEG_D, SEG_C, SEG_B, SEG_A};
//Gösterge anotları/katotları bir dizi oluşturuluyor
int comm[] = {CA1, CA2, CA3, CA4};

/*  Gösterge kütüphanesi 4'lü ortak anot göstergenin kontrolü için gereklidir. Gösterge kütüphanesinden
    bir gösterge nesnesi oluşturuyoruz. gösterge nesnesin:
        segment         : segmentlerin bağlı olduğu pinleri,
        comm            : ortak anotların/katodların bağlı olduğu pinleri,
        GOSTERGE_SAYISI : Göstergenin ksç göstergeden oluştuğu,
        GOSTERGE_TIPI   : Göstergenin tipi ORTAK_ANOT/ORTAK_KATOT
    parametrelerini gönderiyoruz. */
// Gosterge gosterge(segment, anot, GOSTERGE_SAYISI, ORTAK_ANOT);
Gosterge gosterge(segment, comm, GOSTERGE_SAYISI, GOSTERGE_TIPI);

OneWire oneWire18b20(DS18B20_PIN);       //DS18B20 için bir OneWire nesnesi oluşturuluyor
DallasTemperature sensor(&oneWire18b20); //DS18B20 için sensör kütüphanesinde sensor nesnesi oluşturuluyor.

//EEPROM'a kaydedilecek değerler
struct config_t
{
    uint8_t eepIlkCalisma;
    int eepDereceSet;
    int eepTolerans;
} sistem;

//Göstergede gösterilecek olan derece
int derece = 0;
int dereceSet = 0;
int tolerans=0;

int gostergeModu = GOSTERGE_MODU_DERECE;

bool dereceOlcumuYap = false;

void setup()
{
    EEPROM_readAll(0, sistem);          //EEPROM'u oku
    if(sistem.eepIlkCalisma > 1){       //Eğer ilk çalışma ise değerleri kur
        sistem.eepIlkCalisma = 1;
        sistem.eepDereceSet = 250;
        sistem.eepTolerans = 10;
        EEPROM_writeAll(0, sistem);
    }
    dereceSet = sistem.eepDereceSet;
    tolerans = sistem.eepTolerans;
    Timer1.initialize(5000);                    //Timer1 periyodunu 5msn olarak ayarla
    Timer1.attachInterrupt(timer1_ISR, 5000);   //Timer1 kesmesinin kur(her 5msn'de bir -> timer1_ISR fonksiyonu çalışır)
    //Giriş-çıkış ayarları
    pinMode(BUTON_ARTI, INPUT_PULLUP);          
    pinMode(BUTON_EKSI, INPUT_PULLUP);
    pinMode(ROLE_PIN, OUTPUT);    
}

void loop()
{
    if (dereceOlcumuYap)
    {
        derece = dereceOlc();
        termostatKontrol();
        dereceOlcumuYap = false;
    }
    if(butonKontrol()==BUTON_CIFT_BASILDI){
        setData();
    }
}

/*
    Bu fonksiyon bir kesme servis fonksiyonudur. Timer1'i 5 msn değerine kurduk ve 5 msn'de kesmeye girmesi için
    ayarladık ve bu fonksiyona bağladık. Bu sebeple bu fonksiyon donanım tarafından Timer1'e bağlı olarak kontrol
    edilir. Her 5 msn'de otomatik olarak çalıştırılır. Bu fonksiyonu loop içinde çağırmaya gerek yoktur.
*/
void timer1_ISR()
{
    static unsigned int tmrSay = DERECE_OLCUM_ZAMANI;
    tmrSay++;
    if (tmrSay >= DERECE_OLCUM_ZAMANI)
    {
        dereceOlcumuYap = true;
        tmrSay = 0;
    }
    //Her 5 msn'de aktif göstergeyi değiştirir. Bu değişim göz tarafından algılanmadığı için biz tüm göstergeyi
    //çalışıyor olarak görürüz.
    gostergeYaz();
}

/*
    Gösterge moduna göre ekranda gösterilecek verileri hazırlar
*/
void gostergeYaz()
{
    static int gostergeData = 0;
    switch (gostergeModu)
    {
    case GOSTERGE_MODU_DERECE:
        gostergeData = derece;
        if (gostergeData < 0)
            gosterge.gostergeGuncelle(gostergeData, 1, EKSI, BASA_EKLE);
        else
            gosterge.gostergeGuncelle(gostergeData*10, 2, DERECE, SONA_EKLE);
        break;
    case GOSTERGE_MODU_SET:
        gostergeData = dereceSet;
        gosterge.gostergeGuncelle(gostergeData, 1, C, BASA_EKLE);
        break;
    case GOSTERGE_MODU_TOLERANS:
        gostergeData = tolerans;
        gosterge.gostergeGuncelle(gostergeData, 1, T, BASA_EKLE);
        break;
    default:
        break;
    }
}

/*
    DS18B20 sensöründen sıcaklık bilgisini okur.
    Sensörden gelen bilgiyi 10 ile çarparak ondalık haneyide tam sayı hanesine alıp
    float işlem yapmaktan kaçınıyoruz.
*/
int dereceOlc()
{
    sensor.requestTemperatures();    
    return (int)(sensor.getTempCByIndex(0) * 10);
}

/*
    Butonları kontrol eder
*/
int butonKontrol()
{
    int butonDurum = BUTON_BASILMADI;
    if (digitalRead(BUTON_ARTI) == LOW)
    {
        delay(50);
        butonDurum = BUTON_ARTI_BASILDI;
        while (digitalRead(BUTON_ARTI) == LOW){
            if (digitalRead(BUTON_EKSI) == LOW)
            {
                butonDurum = BUTON_CIFT_BASILDI;
            }
        }
        delay(25);
    }
    else if (digitalRead(BUTON_EKSI) == LOW)
    {
        delay(50);
        butonDurum = BUTON_EKSI_BASILDI;
        while (digitalRead(BUTON_EKSI) == LOW){
            if (digitalRead(BUTON_ARTI) == LOW)
            {
                butonDurum = BUTON_CIFT_BASILDI;
            }
        }
        delay(25);
    }
    return butonDurum;
}

/*
    sıcaklık kontrolünü yapar
    Eğer DERECE > SET+TOLERANS ise röleyi açar
    Eğer DERECE < SET-TOLERANS ise röleyi kapatır
*/
void termostatKontrol()
{
    static int _derece = 0;
    if (_derece == derece)
        return;
    if (derece > (dereceSet + tolerans))
        digitalWrite(ROLE_PIN, HIGH);
    if (derece < (dereceSet - tolerans))
        digitalWrite(ROLE_PIN, LOW);
    _derece = derece;
}

/*
    dereceSet ve tolerans değerlerini ayarlar
*/
void setData(){
    gostergeModu = GOSTERGE_MODU_SET;
    int btnKontrol = BUTON_BASILMADI;
    while (btnKontrol != BUTON_CIFT_BASILDI)
    {
        btnKontrol = butonKontrol();
        if(btnKontrol == BUTON_ARTI_BASILDI)
            dereceSet+=1;
        else if(btnKontrol == BUTON_EKSI_BASILDI)
            dereceSet-=1;
    }

    gostergeModu = GOSTERGE_MODU_TOLERANS;
    btnKontrol = BUTON_BASILMADI;
    while (btnKontrol != BUTON_CIFT_BASILDI)
    {
        btnKontrol = butonKontrol();
        if(btnKontrol == BUTON_ARTI_BASILDI)
            tolerans+=1;
        else if(btnKontrol == BUTON_EKSI_BASILDI){
            if(tolerans >1)
                tolerans-=1;
        }
    }
    sistem.eepDereceSet = dereceSet;
    sistem.eepTolerans = tolerans;
    EEPROM_writeAll(0, sistem);
    gostergeModu = GOSTERGE_MODU_DERECE;    
}

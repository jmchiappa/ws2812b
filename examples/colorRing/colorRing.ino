#include <ws2812.h>

int32_t i;
int32_t led;
int32_t colorIndex;
uint8_t myStripLedArray[BUF_SIZE(8)];
stripLed myStripLed(myStripLedArray,sizeof(myStripLedArray),WS_RGB);

// Décrire cette fonction,
int32_t Bleu(int index) {
int32_t couleur;
  if (((255 < index) && (index < 512))) {
    couleur = index - 256;
  } else if (index > 511) {
    couleur = 767 - index;
  } else {
    couleur = 0;
  }
  return couleur;
}


// Décrire cette fonction
int32_t Vert(int index) {
int32_t couleur;
  if (index < 256) {
    couleur = index;
  } else if (index < 512) {
    couleur = 511 - index;
  } else {
    couleur = 0;
  }
  return couleur;
}


// Décrire cette fonction
int32_t Rouge(int index) {
int32_t couleur;
  if (index < 256) {
    couleur = 255 - index;
  } else if (index > 511) {
    couleur = index - 512;
  } else {
    couleur = 0;
  }
  return couleur;
}



void setup() {
  myStripLed.begin();
  i = 0;
}

void loop() {
  for (led = 0; led <= 7; led=led+1) {
    colorIndex = i + led * 95;
    colorIndex = colorIndex % 766;
    myStripLed.setLEDcolor(led,(Rouge(colorIndex)),(Vert(colorIndex)),(Bleu(colorIndex)));
  }
  i = i + 1;
  i = i % 766;
  delay(1);
}